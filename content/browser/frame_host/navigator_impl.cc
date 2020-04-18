// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/navigator_impl.h"

#include <utility>

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/frame_host/debug_urls.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigation_controller_impl.h"
#include "content/browser/frame_host/navigation_entry_impl.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/browser/frame_host/navigation_request.h"
#include "content/browser/frame_host/navigation_request_info.h"
#include "content/browser/frame_host/navigator_delegate.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/browser/webui/web_ui_controller_factory_registry.h"
#include "content/browser/webui/web_ui_impl.h"
#include "content/common/frame_messages.h"
#include "content/common/navigation_params.h"
#include "content/common/page_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/stream_handle.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/url_utils.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"
#include "url/url_util.h"

namespace content {

namespace {

FrameMsg_Navigate_Type::Value GetNavigationType(
    const GURL& old_url,
    const GURL& new_url,
    ReloadType reload_type,
    const NavigationEntryImpl& entry,
    const FrameNavigationEntry& frame_entry,
    bool is_same_document_history_load) {
  // Reload navigations
  switch (reload_type) {
    case ReloadType::NORMAL:
      return FrameMsg_Navigate_Type::RELOAD;
    case ReloadType::BYPASSING_CACHE:
    case ReloadType::DISABLE_PREVIEWS:
      return FrameMsg_Navigate_Type::RELOAD_BYPASSING_CACHE;
    case ReloadType::ORIGINAL_REQUEST_URL:
      return FrameMsg_Navigate_Type::RELOAD_ORIGINAL_REQUEST_URL;
    case ReloadType::NONE:
      break;  // Fall through to rest of function.
  }

  // |RenderViewImpl::PopulateStateFromPendingNavigationParams| differentiates
  // between |RESTORE_WITH_POST| and |RESTORE|.
  if (entry.restore_type() == RestoreType::LAST_SESSION_EXITED_CLEANLY) {
    if (entry.GetHasPostData())
      return FrameMsg_Navigate_Type::RESTORE_WITH_POST;
    else
      return FrameMsg_Navigate_Type::RESTORE;
  }

  // History navigations.
  if (frame_entry.page_state().IsValid()) {
    if (is_same_document_history_load)
      return FrameMsg_Navigate_Type::HISTORY_SAME_DOCUMENT;
    else
      return FrameMsg_Navigate_Type::HISTORY_DIFFERENT_DOCUMENT;
  }
  DCHECK(!is_same_document_history_load);

  // A same-document fragment-navigation happens when the only part of the url
  // that is modified is after the '#' character.
  //
  // Be careful not to consider history navigations. For instance, if the
  // history is: 'A#bar' -> 'B' -> 'A#foo'. Then an history navigation from
  // 'A#foo' to 'A#bar' is not a same-document navigation, but a
  // different-document one! The two FrameNavigationEntry doesn't share the same
  // document_sequence_number.
  //
  // When modifying this condition, please take a look at:
  // FrameLoader::shouldPerformFragmentNavigation.
  if (new_url.has_ref() && old_url.EqualsIgnoringRef(new_url) &&
      frame_entry.method() == "GET") {
    return FrameMsg_Navigate_Type::SAME_DOCUMENT;
  } else {
    return FrameMsg_Navigate_Type::DIFFERENT_DOCUMENT;
  }
}

}  // namespace

struct NavigatorImpl::NavigationMetricsData {
  NavigationMetricsData(base::TimeTicks start_time,
                        GURL url,
                        RestoreType restore_type)
      : start_time_(start_time), url_(url) {
    is_restoring_from_last_session_ =
        (restore_type == RestoreType::LAST_SESSION_EXITED_CLEANLY ||
         restore_type == RestoreType::LAST_SESSION_CRASHED);
  }

  base::TimeTicks start_time_;
  GURL url_;
  bool is_restoring_from_last_session_;
  base::TimeTicks url_job_start_time_;
  base::TimeDelta before_unload_delay_;
};

NavigatorImpl::NavigatorImpl(NavigationControllerImpl* navigation_controller,
                             NavigatorDelegate* delegate)
    : controller_(navigation_controller), delegate_(delegate) {}

NavigatorImpl::~NavigatorImpl() {}

// static
void NavigatorImpl::CheckWebUIRendererDoesNotDisplayNormalURL(
    RenderFrameHostImpl* render_frame_host,
    const GURL& url) {
  int enabled_bindings = render_frame_host->GetEnabledBindings();
  bool is_allowed_in_web_ui_renderer =
      WebUIControllerFactoryRegistry::GetInstance()->IsURLAcceptableForWebUI(
          render_frame_host->frame_tree_node()
              ->navigator()
              ->GetController()
              ->GetBrowserContext(),
          url);
  if ((enabled_bindings & BINDINGS_POLICY_WEB_UI) &&
      !is_allowed_in_web_ui_renderer) {
    // Log the URL to help us diagnose any future failures of this CHECK.
    FrameTreeNode* root_node =
        render_frame_host->frame_tree_node()->frame_tree()->root();
    GetContentClient()->SetActiveURL(
        url, root_node->current_url().possibly_invalid_spec());
    CHECK(0);
  }
}

NavigatorDelegate* NavigatorImpl::GetDelegate() {
  return delegate_;
}

NavigationController* NavigatorImpl::GetController() {
  return controller_;
}

// TODO(clamy): See if we can remove this function now that PlzNavigate has
// shipped.
void NavigatorImpl::DidStartProvisionalLoad(
    RenderFrameHostImpl* render_frame_host,
    const GURL& url,
    const std::vector<GURL>& redirect_chain,
    const base::TimeTicks& navigation_start) {
  bool is_main_frame = render_frame_host->frame_tree_node()->IsMainFrame();
  bool is_error_page = (url.spec() == kUnreachableWebDataURL);
  GURL validated_url(url);
  RenderProcessHost* render_process_host = render_frame_host->GetProcess();
  render_process_host->FilterURL(false, &validated_url);

  // Do not allow browser plugin guests to navigate to non-web URLs, since they
  // cannot swap processes or grant bindings.
  ChildProcessSecurityPolicyImpl* policy =
      ChildProcessSecurityPolicyImpl::GetInstance();
  if (render_process_host->IsForGuestsOnly() &&
      !policy->IsWebSafeScheme(validated_url.scheme())) {
    validated_url = GURL(url::kAboutBlankURL);
  }

  if (is_main_frame && !is_error_page) {
    DidStartMainFrameNavigation(validated_url,
                                render_frame_host->GetSiteInstance(),
                                render_frame_host->GetNavigationHandle());
  }
}

void NavigatorImpl::DidFailProvisionalLoadWithError(
    RenderFrameHostImpl* render_frame_host,
    const FrameHostMsg_DidFailProvisionalLoadWithError_Params& params) {
  VLOG(1) << "Failed Provisional Load: " << params.url.possibly_invalid_spec()
          << ", error_code: " << params.error_code
          << ", error_description: " << params.error_description
          << ", showing_repost_interstitial: "
          << params.showing_repost_interstitial
          << ", frame_id: " << render_frame_host->GetRoutingID();
  GURL validated_url(params.url);
  RenderProcessHost* render_process_host = render_frame_host->GetProcess();
  render_process_host->FilterURL(false, &validated_url);

  if (net::ERR_ABORTED == params.error_code) {
    // EVIL HACK ALERT! Ignore failed loads when we're showing interstitials.
    // This means that the interstitial won't be torn down properly, which is
    // bad. But if we have an interstitial, go back to another tab type, and
    // then load the same interstitial again, we could end up getting the first
    // interstitial's "failed" message (as a result of the cancel) when we're on
    // the second one. We can't tell this apart, so we think we're tearing down
    // the current page which will cause a crash later on.
    //
    // http://code.google.com/p/chromium/issues/detail?id=2855
    // Because this will not tear down the interstitial properly, if "back" is
    // back to another tab type, the interstitial will still be somewhat alive
    // in the previous tab type. If you navigate somewhere that activates the
    // tab with the interstitial again, you'll see a flash before the new load
    // commits of the interstitial page.
    if (delegate_ && delegate_->ShowingInterstitialPage()) {
      LOG(WARNING) << "Discarding message during interstitial.";
      return;
    }

    // We used to cancel the pending renderer here for cross-site downloads.
    // However, it's not safe to do that because the download logic repeatedly
    // looks for this WebContents based on a render ID. Instead, we just
    // leave the pending renderer around until the next navigation event
    // (Navigate, DidNavigate, etc), which will clean it up properly.
    //
    // TODO(creis): Find a way to cancel any pending RFH here.
  }

  // Discard the pending navigation entry if needed.
  int expected_pending_entry_id =
      render_frame_host->GetNavigationHandle()
          ? render_frame_host->GetNavigationHandle()->pending_nav_entry_id()
          : 0;
  DiscardPendingEntryIfNeeded(expected_pending_entry_id);
}

void NavigatorImpl::DidFailLoadWithError(
    RenderFrameHostImpl* render_frame_host,
    const GURL& url,
    int error_code,
    const base::string16& error_description) {
  if (delegate_) {
    delegate_->DidFailLoadWithError(render_frame_host, url, error_code,
                                    error_description);
  }
}

bool NavigatorImpl::NavigateToEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    const NavigationEntryImpl& entry,
    ReloadType reload_type,
    bool is_same_document_history_load,
    bool is_history_navigation_in_new_child,
    bool is_pending_entry,
    const scoped_refptr<network::ResourceRequestBody>& post_body,
    std::unique_ptr<NavigationUIData> navigation_ui_data) {
  TRACE_EVENT0("browser,navigation", "NavigatorImpl::NavigateToEntry");

  GURL dest_url = frame_entry.url();
  Referrer dest_referrer = frame_entry.referrer();
  if ((reload_type == ReloadType::ORIGINAL_REQUEST_URL ||
       reload_type == ReloadType::DISABLE_PREVIEWS) &&
      entry.GetOriginalRequestURL().is_valid() && !entry.GetHasPostData()) {
    // We may have been redirected when navigating to the current URL.
    // Use the URL the user originally intended to visit as signaled by the
    // ReloadType, if it's valid and if a POST wasn't involved; the latter
    // case avoids issues with sending data to the wrong page. The
    // DISABLE_PREVIEWS case is triggered from a user action to view the
    // original URL without any preview intervention treatment.
    dest_url = entry.GetOriginalRequestURL();
    dest_referrer = Referrer();
  }

  // Don't attempt to navigate if the virtual URL is non-empty and invalid.
  if (frame_tree_node->IsMainFrame()) {
    const GURL& virtual_url = entry.GetVirtualURL();
    if (!virtual_url.is_valid() && !virtual_url.is_empty()) {
      LOG(WARNING) << "Refusing to load for invalid virtual URL: "
                   << virtual_url.possibly_invalid_spec();
      return false;
    }
  }

  // Don't attempt to navigate to non-empty invalid URLs.
  if (!dest_url.is_valid() && !dest_url.is_empty()) {
    LOG(WARNING) << "Refusing to load invalid URL: "
                 << dest_url.possibly_invalid_spec();
    return false;
  }

  // The renderer will reject IPC messages with URLs longer than
  // this limit, so don't attempt to navigate with a longer URL.
  if (dest_url.spec().size() > url::kMaxURLChars) {
    LOG(WARNING) << "Refusing to load URL as it exceeds " << url::kMaxURLChars
                 << " characters.";
    return false;
  }

  // This will be used to set the Navigation Timing API navigationStart
  // parameter for browser navigations in new tabs (intents, tabs opened through
  // "Open link in new tab"). We need to keep it above RFHM::Navigate() call to
  // capture the time needed for the RenderFrameHost initialization.
  base::TimeTicks navigation_start = base::TimeTicks::Now();
  base::TimeTicks tracing_navigation_start = TRACE_TIME_TICKS_NOW();
  TRACE_EVENT_INSTANT_WITH_TIMESTAMP0(
      "navigation,rail", "NavigationTiming navigationStart",
      TRACE_EVENT_SCOPE_GLOBAL, tracing_navigation_start);

  // Determine if Previews should be used for the navigation.
  PreviewsState previews_state = PREVIEWS_UNSPECIFIED;
  if (!frame_tree_node->IsMainFrame()) {
    // For subframes, use the state of the top-level frame.
    previews_state = frame_tree_node->frame_tree()
                         ->root()
                         ->current_frame_host()
                         ->last_navigation_previews_state();
  } else if (reload_type == ReloadType::DISABLE_PREVIEWS) {
    // Disable LoFi when asked for it explicitly.
    previews_state = PREVIEWS_NO_TRANSFORM;
  }

  // Give the delegate an opportunity to adjust the previews state.
  if (delegate_)
    delegate_->AdjustPreviewsStateForNavigation(&previews_state);

  navigation_data_.reset(new NavigationMetricsData(navigation_start, dest_url,
                                                   entry.restore_type()));
  RequestNavigation(frame_tree_node, dest_url, dest_referrer, frame_entry,
                    entry, reload_type, previews_state,
                    is_same_document_history_load,
                    is_history_navigation_in_new_child, post_body,
                    navigation_start, std::move(navigation_ui_data));
  if (frame_tree_node->IsMainFrame() && frame_tree_node->navigation_request()) {
    // For the trace below we're using the navigation handle as the async
    // trace id, |navigation_start| as the timestamp and reporting the
    // FrameTreeNode id as a parameter. For navigations where no network
    // request is made (data URLs, JavaScript URLs, etc) there is no handle
    // and so no tracing is done.
    TRACE_EVENT_ASYNC_BEGIN_WITH_TIMESTAMP1(
        "navigation", "Navigation timeToNetworkStack",
        frame_tree_node->navigation_request()->navigation_handle(),
        tracing_navigation_start, "FrameTreeNode id",
        frame_tree_node->frame_tree_node_id());
  }

  // Make sure no code called via RFH::Navigate clears the pending entry.
  if (is_pending_entry)
    CHECK_EQ(controller_->GetPendingEntry(), &entry);

  if (controller_->GetPendingEntryIndex() == -1 &&
      dest_url.SchemeIs(url::kJavaScriptScheme)) {
    // If the pending entry index is -1 (which means a new navigation rather
    // than a history one), and the user typed in a javascript: URL, don't add
    // it to the session history.
    //
    // This is a hack. What we really want is to avoid adding to the history
    // any URL that doesn't generate content, and what would be great would be
    // if we had a message from the renderer telling us that a new page was
    // not created. The same message could be used for mailto: URLs and the
    // like.
    return false;
  }

  // Notify observers about navigation.
  if (delegate_ && is_pending_entry)
    delegate_->DidStartNavigationToPendingEntry(dest_url, reload_type);

  return true;
}

bool NavigatorImpl::NavigateToPendingEntry(
    FrameTreeNode* frame_tree_node,
    const FrameNavigationEntry& frame_entry,
    ReloadType reload_type,
    bool is_same_document_history_load,
    std::unique_ptr<NavigationUIData> navigation_ui_data) {
  return NavigateToEntry(frame_tree_node, frame_entry,
                         *controller_->GetPendingEntry(), reload_type,
                         is_same_document_history_load, false, true, nullptr,
                         std::move(navigation_ui_data));
}

bool NavigatorImpl::StartHistoryNavigationInNewSubframe(
    RenderFrameHostImpl* render_frame_host,
    const GURL& default_url) {
  return controller_->StartHistoryNavigationInNewSubframe(render_frame_host,
                                                          default_url);
}

void NavigatorImpl::DidNavigate(
    RenderFrameHostImpl* render_frame_host,
    const FrameHostMsg_DidCommitProvisionalLoad_Params& params,
    std::unique_ptr<NavigationHandleImpl> navigation_handle,
    bool was_within_same_document) {
  FrameTreeNode* frame_tree_node = render_frame_host->frame_tree_node();
  FrameTree* frame_tree = frame_tree_node->frame_tree();

  bool is_same_document_navigation = controller_->IsURLSameDocumentNavigation(
      params.url, params.origin, was_within_same_document, render_frame_host);

  // If a frame claims the navigation was same-document, it must be the current
  // frame, not a pending one.
  if (is_same_document_navigation &&
      render_frame_host !=
          frame_tree_node->render_manager()->current_frame_host()) {
    bad_message::ReceivedBadMessage(render_frame_host->GetProcess(),
                                    bad_message::NI_IN_PAGE_NAVIGATION);
    is_same_document_navigation = false;
  }

  if (ui::PageTransitionIsMainFrame(params.transition)) {
    if (delegate_) {
      // When overscroll navigation gesture is enabled, a screenshot of the page
      // in its current state is taken so that it can be used during the
      // nav-gesture. It is necessary to take the screenshot here, before
      // calling RenderFrameHostManager::DidNavigateMainFrame, because that can
      // change WebContents::GetRenderViewHost to return the new host, instead
      // of the one that may have just been swapped out.
      if (delegate_->CanOverscrollContent()) {
        // Don't take screenshots if we are staying on the same document. We
        // want same-document navigations to be super fast, and taking a
        // screenshot currently blocks GPU for a longer time than we are willing
        // to tolerate in this use case.
        if (!was_within_same_document)
          controller_->TakeScreenshot();
      }

      // Run tasks that must execute just before the commit.
      delegate_->DidNavigateMainFramePreCommit(is_same_document_navigation);
    }
  }

  // DidNavigateFrame() must be called before replicating the new origin and
  // other properties to proxies.  This is because it destroys the subframes of
  // the frame we're navigating from, which might trigger those subframes to
  // run unload handlers.  Those unload handlers should still see the old
  // frame's origin.  See https://crbug.com/825283.
  frame_tree_node->render_manager()->DidNavigateFrame(
      render_frame_host, params.gesture == NavigationGestureUser,
      is_same_document_navigation);

  // Save the new page's origin and other properties, and replicate them to
  // proxies, including the proxy created in DidNavigateFrame() to replace the
  // old frame in cross-process navigation cases.
  frame_tree_node->SetCurrentOrigin(
      params.origin, params.has_potentially_trustworthy_unique_origin);
  frame_tree_node->SetInsecureRequestPolicy(params.insecure_request_policy);
  frame_tree_node->SetInsecureNavigationsSet(params.insecure_navigations_set);

  // Navigating to a new location means a new, fresh set of http headers and/or
  // <meta> elements - we need to reset CSP and Feature Policy.
  if (!is_same_document_navigation) {
    render_frame_host->ResetContentSecurityPolicies();
    frame_tree_node->ResetForNavigation();
  }

  // Update the site of the SiteInstance if it doesn't have one yet, unless
  // assigning a site is not necessary for this URL. In that case, the
  // SiteInstance can still be considered unused until a navigation to a real
  // page.
  SiteInstanceImpl* site_instance = render_frame_host->GetSiteInstance();
  if (!site_instance->HasSite() &&
      SiteInstanceImpl::ShouldAssignSiteForURL(params.url)) {
    site_instance->SetSite(params.url);
  }

  // Need to update MIME type here because it's referred to in
  // UpdateNavigationCommands() called by RendererDidNavigate() to
  // determine whether or not to enable the encoding menu.
  // It's updated only for the main frame. For a subframe,
  // RenderView::UpdateURL does not set params.contents_mime_type.
  // (see http://code.google.com/p/chromium/issues/detail?id=2929 )
  // TODO(jungshik): Add a test for the encoding menu to avoid
  // regressing it again.
  // TODO(nasko): Verify the correctness of the above comment, since some of the
  // code doesn't exist anymore. Also, move this code in the
  // PageTransitionIsMainFrame code block above.
  if (ui::PageTransitionIsMainFrame(params.transition) && delegate_)
    delegate_->SetMainFrameMimeType(params.contents_mime_type);

  int old_entry_count = controller_->GetEntryCount();
  LoadCommittedDetails details;
  bool did_navigate = controller_->RendererDidNavigate(
      render_frame_host, params, &details, is_same_document_navigation,
      navigation_handle.get());

  // If the history length and/or offset changed, update other renderers in the
  // FrameTree.
  if (old_entry_count != controller_->GetEntryCount() ||
      details.previous_entry_index !=
          controller_->GetLastCommittedEntryIndex()) {
    frame_tree->root()->render_manager()->SendPageMessage(
        new PageMsg_SetHistoryOffsetAndLength(
            MSG_ROUTING_NONE, controller_->GetLastCommittedEntryIndex(),
            controller_->GetEntryCount()),
        site_instance);
  }

  render_frame_host->DidNavigate(params, is_same_document_navigation);

  // Send notification about committed provisional loads. This notification is
  // different from the NAV_ENTRY_COMMITTED notification which doesn't include
  // the actual URL navigated to and isn't sent for AUTO_SUBFRAME navigations.
  if (details.type != NAVIGATION_TYPE_NAV_IGNORE && delegate_) {
    DCHECK_EQ(!render_frame_host->GetParent(),
              did_navigate ? details.is_main_frame : false);
    navigation_handle->DidCommitNavigation(
        params, did_navigate, details.did_replace_entry, details.previous_url,
        details.type, render_frame_host);
    navigation_handle.reset();
  }

  if (!did_navigate)
    return;  // No navigation happened.

  // DO NOT ADD MORE STUFF TO THIS FUNCTION! Your component should either listen
  // for the appropriate notification (best) or you can add it to
  // DidNavigateMainFramePostCommit / DidNavigateAnyFramePostCommit (only if
  // necessary, please).

  // TODO(carlosk): Move this out when PlzNavigate implementation properly calls
  // the observer methods.
  RecordNavigationMetrics(details, params, site_instance);

  // Run post-commit tasks.
  if (delegate_) {
    if (details.is_main_frame) {
      delegate_->DidNavigateMainFramePostCommit(render_frame_host, details,
                                                params);
    }

    delegate_->DidNavigateAnyFramePostCommit(render_frame_host, details,
                                             params);
  }
}

void NavigatorImpl::RequestOpenURL(
    RenderFrameHostImpl* render_frame_host,
    const GURL& url,
    bool uses_post,
    const scoped_refptr<network::ResourceRequestBody>& body,
    const std::string& extra_headers,
    const Referrer& referrer,
    WindowOpenDisposition disposition,
    bool should_replace_current_entry,
    bool user_gesture,
    blink::WebTriggeringEventInfo triggering_event_info,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory) {
  // Note: This can be called for subframes (even when OOPIFs are not possible)
  // if the disposition calls for a different window.

  // Only the current RenderFrameHost should be sending an OpenURL request.
  // Pending RenderFrameHost should know where it is navigating and pending
  // deletion RenderFrameHost shouldn't be trying to navigate.
  if (render_frame_host !=
      render_frame_host->frame_tree_node()->current_frame_host()) {
    return;
  }

  SiteInstance* current_site_instance = render_frame_host->GetSiteInstance();

  // TODO(creis): Pass the redirect_chain into this method to support client
  // redirects.  http://crbug.com/311721.
  std::vector<GURL> redirect_chain;

  // Note that unlike NavigateFromFrameProxy, this uses the navigating
  // RenderFrameHost's current SiteInstance, as that's where this navigation
  // originated.
  GURL dest_url(url);
  if (!GetContentClient()->browser()->ShouldAllowOpenURL(current_site_instance,
                                                         url)) {
    dest_url = GURL(url::kAboutBlankURL);
  }

  int frame_tree_node_id = -1;

  // Send the navigation to the current FrameTreeNode if it's destined for a
  // subframe in the current tab.  We'll assume it's for the main frame
  // (possibly of a new or different WebContents) otherwise.
  if (disposition == WindowOpenDisposition::CURRENT_TAB &&
      render_frame_host->GetParent()) {
    frame_tree_node_id =
        render_frame_host->frame_tree_node()->frame_tree_node_id();
  }

  OpenURLParams params(dest_url, referrer, frame_tree_node_id, disposition,
                       ui::PAGE_TRANSITION_LINK,
                       true /* is_renderer_initiated */);
  params.uses_post = uses_post;
  params.post_data = body;
  params.extra_headers = extra_headers;
  if (redirect_chain.size() > 0)
    params.redirect_chain = redirect_chain;
  params.should_replace_current_entry = should_replace_current_entry;
  params.user_gesture = user_gesture;
  params.triggering_event_info = triggering_event_info;

  // RequestOpenURL is used only for local frames, so we can get here only if
  // the navigation is initiated by a frame in the same SiteInstance as this
  // frame.  Note that navigations on RenderFrameProxies do not use
  // RequestOpenURL and go through NavigateFromFrameProxy instead.
  params.source_site_instance = current_site_instance;

  params.source_render_frame_id = render_frame_host->GetRoutingID();
  params.source_render_process_id = render_frame_host->GetProcess()->GetID();

  if (render_frame_host->web_ui()) {
    // Note that we hide the referrer for Web UI pages. We don't really want
    // web sites to see a referrer of "chrome://blah" (and some chrome: URLs
    // might have search terms or other stuff we don't want to send to the
    // site), so we send no referrer.
    params.referrer = Referrer();

    // Navigations in Web UI pages count as browser-initiated navigations.
    params.is_renderer_initiated = false;
  }

  params.blob_url_loader_factory = std::move(blob_url_loader_factory);

  GetContentClient()->browser()->OverrideNavigationParams(
      current_site_instance, &params.transition, &params.is_renderer_initiated,
      &params.referrer);

  if (delegate_)
    delegate_->OpenURL(params);
}

void NavigatorImpl::NavigateFromFrameProxy(
    RenderFrameHostImpl* render_frame_host,
    const GURL& url,
    SiteInstance* source_site_instance,
    const Referrer& referrer,
    ui::PageTransition page_transition,
    bool should_replace_current_entry,
    const std::string& method,
    scoped_refptr<network::ResourceRequestBody> post_body,
    const std::string& extra_headers,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory) {
  // |method != "POST"| should imply absence of |post_body|.
  if (method != "POST" && post_body) {
    NOTREACHED();
    post_body = nullptr;
  }

  // Allow the delegate to cancel the transfer.
  if (!delegate_->ShouldTransferNavigation(
          render_frame_host->frame_tree_node()->IsMainFrame()))
    return;

  Referrer referrer_to_use(referrer);
  SiteInstance* current_site_instance = render_frame_host->GetSiteInstance();
  // It is important to pass in the source_site_instance if it is available
  // (such as when navigating a proxy).  See https://crbug.com/656752.
  if (!GetContentClient()->browser()->ShouldAllowOpenURL(
          source_site_instance ? source_site_instance : current_site_instance,
          url)) {
    // It is important to return here, rather than rewrite the url to
    // about:blank.  The latter won't actually have any effect when
    // transferring, as NavigateToEntry will think that the transfer is to the
    // same RFH that started the navigation and let the existing navigation (for
    // the disallowed URL) proceed.
    return;
  }

  // TODO(creis): Determine if this transfer started as a browser-initiated
  // navigation.  See https://crbug.com/495161.
  bool is_renderer_initiated = true;
  if (render_frame_host->web_ui()) {
    // Note that we hide the referrer for Web UI pages. We don't really want
    // web sites to see a referrer of "chrome://blah" (and some chrome: URLs
    // might have search terms or other stuff we don't want to send to the
    // site), so we send no referrer.
    referrer_to_use = Referrer();

    // Navigations in Web UI pages count as browser-initiated navigations.
    is_renderer_initiated = false;
  }

  GetContentClient()->browser()->OverrideNavigationParams(
      current_site_instance, &page_transition, &is_renderer_initiated,
      &referrer_to_use);

  controller_->NavigateFromFrameProxy(
      render_frame_host, url, is_renderer_initiated, source_site_instance,
      referrer_to_use, page_transition, should_replace_current_entry, method,
      post_body, extra_headers, std::move(blob_url_loader_factory));
}

void NavigatorImpl::OnBeforeUnloadACK(FrameTreeNode* frame_tree_node,
                                      bool proceed,
                                      const base::TimeTicks& proceed_time) {
  DCHECK(frame_tree_node);

  NavigationRequest* navigation_request = frame_tree_node->navigation_request();

  // The NavigationRequest may have been canceled while the renderer was
  // executing the BeforeUnload event.
  if (!navigation_request)
    return;

  // If the user chose not to proceed, cancel the ongoing navigation.
  // Note: it might be a new navigation, and not the one that triggered the
  // sending of the BeforeUnload IPC in the first place. However, the
  // BeforeUnload where the user asked not to proceed will have taken place
  // after the navigation started. The last user input shoud be respected, and
  // the navigation cancelled anyway.
  if (!proceed) {
    CancelNavigation(frame_tree_node, true);
    return;
  }

  // The browser-initiated NavigationRequest that triggered the sending of the
  // BeforeUnload IPC might have been replaced by a renderer-initiated one while
  // the BeforeUnload event executed in the renderer. In that case, the request
  // will already have begun, so there is no need to start it again.
  if (navigation_request->state() >
      NavigationRequest::WAITING_FOR_RENDERER_RESPONSE) {
    DCHECK(navigation_request->from_begin_navigation());
    return;
  }

  // Update the navigation start: it should be when it was determined that the
  // navigation will proceed.
  navigation_request->set_navigation_start_time(proceed_time);

  DCHECK_EQ(NavigationRequest::WAITING_FOR_RENDERER_RESPONSE,
            navigation_request->state());

  // Send the request to the IO thread.
  navigation_request->BeginNavigation();
  // DO NOT USE |navigation_request| BEYOND THIS POINT. It might have been
  // destroyed in BeginNavigation().
  // See https://crbug.com/770157.
}

void NavigatorImpl::OnBeginNavigation(
    FrameTreeNode* frame_tree_node,
    const CommonNavigationParams& common_params,
    mojom::BeginNavigationParamsPtr begin_params,
    scoped_refptr<network::SharedURLLoaderFactory> blob_url_loader_factory) {
  // TODO(clamy): the url sent by the renderer should be validated with
  // FilterURL.
  // This is a renderer-initiated navigation.
  DCHECK(frame_tree_node);

  NavigationRequest* ongoing_navigation_request =
      frame_tree_node->navigation_request();

  // Client redirects during the initial history navigation of a child frame
  // should take precedence over the history navigation (despite being renderer-
  // initiated).  See https://crbug.com/348447 and https://crbug.com/691168.
  if (ongoing_navigation_request && ongoing_navigation_request->request_params()
                                        .is_history_navigation_in_new_child) {
    // Preemptively clear this local pointer before deleting the request.
    ongoing_navigation_request = nullptr;
    frame_tree_node->ResetNavigationRequest(false, true);
  }

  // The renderer-initiated navigation request is ignored iff a) there is an
  // ongoing request b) which is browser initiated and c) the renderer request
  // is not user-initiated.
  if (ongoing_navigation_request &&
      ongoing_navigation_request->browser_initiated() &&
      !common_params.has_user_gesture) {
    RenderFrameHost* current_frame_host =
        frame_tree_node->render_manager()->current_frame_host();
    current_frame_host->Send(
        new FrameMsg_DroppedNavigation(current_frame_host->GetRoutingID()));
    return;
  }

  // In all other cases the current navigation, if any, is canceled and a new
  // NavigationRequest is created for the node.
  if (frame_tree_node->IsMainFrame()) {
    // Renderer-initiated main-frame navigations that need to swap processes
    // will go to the browser via a OpenURL call, and then be handled by the
    // same code path as browser-initiated navigations. For renderer-initiated
    // main frame navigation that start via a BeginNavigation IPC, the
    // RenderFrameHost will not be swapped. Therefore it is safe to call
    // DidStartMainFrameNavigation with the SiteInstance from the current
    // RenderFrameHost.
    DidStartMainFrameNavigation(
        common_params.url,
        frame_tree_node->current_frame_host()->GetSiteInstance(), nullptr);
    navigation_data_.reset();
  }
  NavigationEntryImpl* pending_entry = controller_->GetPendingEntry();
  NavigationEntryImpl* current_entry = controller_->GetLastCommittedEntry();
  // Only consult the delegate for override state if there is no current entry,
  // since that state should only apply to newly created tabs (and not cases
  // where the NavigationEntry recorded the state).
  bool override_user_agent =
      current_entry
          ? current_entry->GetIsOverridingUserAgent()
          : delegate_ && delegate_->ShouldOverrideUserAgentInNewTabs();
  frame_tree_node->CreatedNavigationRequest(
      NavigationRequest::CreateRendererInitiated(
          frame_tree_node, pending_entry, common_params,
          std::move(begin_params), controller_->GetLastCommittedEntryIndex(),
          controller_->GetEntryCount(), override_user_agent,
          std::move(blob_url_loader_factory)));
  NavigationRequest* navigation_request = frame_tree_node->navigation_request();

  // For main frames, NavigationHandle will be created after the call to
  // |DidStartMainFrameNavigation|, so it receives the most up to date pending
  // entry from the NavigationController.
  navigation_request->BeginNavigation();
  // DO NOT USE |navigation_request| BEYOND THIS POINT. It might have been
  // destroyed in BeginNavigation().
  // See https://crbug.com/770157.
}

void NavigatorImpl::RestartNavigationAsCrossDocument(
    std::unique_ptr<NavigationRequest> navigation_request) {
  FrameTreeNode* frame_tree_node = navigation_request->frame_tree_node();
  // Don't restart the navigation if there is already another ongoing navigation
  // in the FrameTreeNode.
  if (frame_tree_node->navigation_request())
    return;

  navigation_request->ResetForCrossDocumentRestart();
  frame_tree_node->CreatedNavigationRequest(std::move(navigation_request));
  frame_tree_node->navigation_request()->BeginNavigation();
  // DO NOT USE THE NAVIGATION REQUEST BEYOND THIS POINT. It might have been
  // destroyed in BeginNavigation().
  // See https://crbug.com/770157.
}

void NavigatorImpl::OnAbortNavigation(FrameTreeNode* frame_tree_node) {
  NavigationRequest* ongoing_navigation_request =
      frame_tree_node->navigation_request();
  if (!ongoing_navigation_request ||
      ongoing_navigation_request->browser_initiated()) {
    return;
  }

  // Abort the renderer-initiated navigation request.
  CancelNavigation(frame_tree_node, false);
}

void NavigatorImpl::CancelNavigation(FrameTreeNode* frame_tree_node,
                                     bool inform_renderer) {
  if (frame_tree_node->navigation_request() &&
      frame_tree_node->navigation_request()->navigation_handle()) {
    frame_tree_node->navigation_request()
        ->navigation_handle()
        ->set_net_error_code(net::ERR_ABORTED);
  }
  frame_tree_node->ResetNavigationRequest(false, inform_renderer);
  if (frame_tree_node->IsMainFrame())
    navigation_data_.reset();
}

void NavigatorImpl::LogResourceRequestTime(base::TimeTicks timestamp,
                                           const GURL& url) {
  if (navigation_data_ && navigation_data_->url_ == url) {
    navigation_data_->url_job_start_time_ = timestamp;
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToURLJobStart",
        navigation_data_->url_job_start_time_ - navigation_data_->start_time_);
  }
}

void NavigatorImpl::LogBeforeUnloadTime(
    const base::TimeTicks& renderer_before_unload_start_time,
    const base::TimeTicks& renderer_before_unload_end_time) {
  // Only stores the beforeunload delay if we're tracking a browser initiated
  // navigation and it happened later than the navigation request.
  if (navigation_data_ &&
      renderer_before_unload_start_time > navigation_data_->start_time_) {
    navigation_data_->before_unload_delay_ =
        renderer_before_unload_end_time - renderer_before_unload_start_time;
  }
}

void NavigatorImpl::DiscardPendingEntryIfNeeded(int expected_pending_entry_id) {
  // Racy conditions can cause a fail message to arrive after its corresponding
  // pending entry has been replaced by another navigation. If
  // |DiscardPendingEntry| is called in this case, then the completely valid
  // entry for the new navigation would be discarded. See crbug.com/513742. To
  // catch this case, the current pending entry is compared against the current
  // navigation handle's entry id, which should correspond to the failed load.
  NavigationEntry* pending_entry = controller_->GetPendingEntry();
  bool pending_matches_fail_msg =
      pending_entry &&
      expected_pending_entry_id == pending_entry->GetUniqueID();
  if (!pending_matches_fail_msg)
    return;

  // We usually clear the pending entry when it fails, so that an arbitrary URL
  // isn't left visible above a committed page. This must be enforced when the
  // pending entry isn't visible (e.g., renderer-initiated navigations) to
  // prevent URL spoofs for same-document navigations that don't go through
  // DidStartProvisionalLoadForFrame.
  //
  // However, we do preserve the pending entry in some cases, such as on the
  // initial navigation of an unmodified blank tab. We also allow the delegate
  // to say when it's safe to leave aborted URLs in the omnibox, to let the
  // user edit the URL and try again. This may be useful in cases that the
  // committed page cannot be attacker-controlled. In these cases, we still
  // allow the view to clear the pending entry and typed URL if the user
  // requests (e.g., hitting Escape with focus in the address bar).
  //
  // Note: don't touch the transient entry, since an interstitial may exist.
  bool should_preserve_entry = controller_->IsUnmodifiedBlankTab() ||
                               delegate_->ShouldPreserveAbortedURLs();
  if (pending_entry != controller_->GetVisibleEntry() ||
      !should_preserve_entry) {
    controller_->DiscardPendingEntry(true);

    // Also force the UI to refresh.
    controller_->delegate()->NotifyNavigationStateChanged(INVALIDATE_TYPE_URL);
  }
}

void NavigatorImpl::RequestNavigation(
    FrameTreeNode* frame_tree_node,
    const GURL& dest_url,
    const Referrer& dest_referrer,
    const FrameNavigationEntry& frame_entry,
    const NavigationEntryImpl& entry,
    ReloadType reload_type,
    PreviewsState previews_state,
    bool is_same_document_history_load,
    bool is_history_navigation_in_new_child,
    const scoped_refptr<network::ResourceRequestBody>& post_body,
    base::TimeTicks navigation_start,
    std::unique_ptr<NavigationUIData> navigation_ui_data) {
  DCHECK(frame_tree_node);

  // This is not a real navigation. Send the URL to the renderer process
  // immediately for handling.
  // Note: this includes navigations to JavaScript URLs, which are considered
  // renderer-debug URLs.
  if (IsRendererDebugURL(dest_url)) {
    RenderFrameHostImpl* render_frame_host =
        frame_tree_node->current_frame_host();
    frame_tree_node->render_manager()->InitializeRenderFrameIfNecessary(
        render_frame_host);
    render_frame_host->HandleRendererDebugURL(dest_url);
    return;
  }

  // This value must be set here because creating a NavigationRequest might
  // change the renderer live/non-live status and change this result.
  // We don't want to dispatch a beforeunload handler if
  // is_history_navigation_in_new_child is true. This indicates a newly created
  // child frame which does not have a beforunload handler.
  FrameMsg_Navigate_Type::Value navigation_type = GetNavigationType(
      frame_tree_node->current_url(),  // old_url
      dest_url,                        // new_url
      reload_type,                     // reload_type
      entry,                           // entry
      frame_entry,                     // frame_entry
      is_same_document_history_load);  // is_same_document_history_load
  bool is_same_document =
      FrameMsg_Navigate_Type::IsSameDocument(navigation_type);
  bool should_dispatch_beforeunload =
      !is_same_document && !is_history_navigation_in_new_child &&
      frame_tree_node->current_frame_host()->ShouldDispatchBeforeUnload();
  std::unique_ptr<NavigationRequest> scoped_request =
      NavigationRequest::CreateBrowserInitiated(
          frame_tree_node, dest_url, dest_referrer, frame_entry, entry,
          navigation_type, previews_state, is_same_document_history_load,
          is_history_navigation_in_new_child, post_body, navigation_start,
          controller_, std::move(navigation_ui_data));

  frame_tree_node->CreatedNavigationRequest(std::move(scoped_request));

  NavigationRequest* navigation_request = frame_tree_node->navigation_request();
  if (!navigation_request)
    return;  // Navigation was synchronously stopped.

  // Have the current renderer execute its beforeunload event if needed. If it
  // is not needed then NavigationRequest::BeginNavigation should be directly
  // called instead.
  if (should_dispatch_beforeunload) {
    navigation_request->SetWaitingForRendererResponse();
    frame_tree_node->current_frame_host()->DispatchBeforeUnload(
        true, reload_type != ReloadType::NONE);
  } else {
    navigation_request->BeginNavigation();
    // DO NOT USE |navigation_request| BEYOND THIS POINT. It might have been
    // destroyed in BeginNavigation().
    // See https://crbug.com/770157.
    return;
  }
}

void NavigatorImpl::RecordNavigationMetrics(
    const LoadCommittedDetails& details,
    const FrameHostMsg_DidCommitProvisionalLoad_Params& params,
    SiteInstance* site_instance) {
  DCHECK(site_instance->HasProcess());

  if (!details.is_main_frame || !navigation_data_ ||
      navigation_data_->url_job_start_time_.is_null() ||
      navigation_data_->url_ != params.original_request_url) {
    return;
  }

  base::TimeDelta time_to_commit =
      base::TimeTicks::Now() - navigation_data_->start_time_;
  UMA_HISTOGRAM_TIMES("Navigation.TimeToCommit", time_to_commit);

  time_to_commit -= navigation_data_->before_unload_delay_;
  base::TimeDelta time_to_network = navigation_data_->url_job_start_time_ -
                                    navigation_data_->start_time_ -
                                    navigation_data_->before_unload_delay_;
  if (navigation_data_->is_restoring_from_last_session_) {
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToCommit_SessionRestored_BeforeUnloadDiscounted",
        time_to_commit);
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToURLJobStart_SessionRestored_BeforeUnloadDiscounted",
        time_to_network);
    navigation_data_.reset();
    return;
  }
  bool navigation_created_new_renderer_process =
      site_instance->GetProcess()->GetInitTimeForNavigationMetrics() >
      navigation_data_->start_time_;
  if (navigation_created_new_renderer_process) {
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToCommit_NewRenderer_BeforeUnloadDiscounted",
        time_to_commit);
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToURLJobStart_NewRenderer_BeforeUnloadDiscounted",
        time_to_network);
  } else {
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToCommit_ExistingRenderer_BeforeUnloadDiscounted",
        time_to_commit);
    UMA_HISTOGRAM_TIMES(
        "Navigation.TimeToURLJobStart_ExistingRenderer_BeforeUnloadDiscounted",
        time_to_network);
  }
  navigation_data_.reset();
}

void NavigatorImpl::DidStartMainFrameNavigation(
    const GURL& url,
    SiteInstanceImpl* site_instance,
    NavigationHandleImpl* navigation_handle) {
  // If there is no browser-initiated pending entry for this navigation and it
  // is not for the error URL, create a pending entry using the current
  // SiteInstance, and ensure the address bar updates accordingly.  We don't
  // know the referrer or extra headers at this point, but the referrer will
  // be set properly upon commit.
  NavigationEntryImpl* pending_entry = controller_->GetPendingEntry();
  bool has_browser_initiated_pending_entry =
      pending_entry && !pending_entry->is_renderer_initiated();

  // A pending navigation entry is created in OnBeginNavigation(). The renderer
  // sends a provisional load notification after that. We don't want to create
  // a duplicate navigation entry here.
  bool renderer_provisional_load_to_pending_url =
      pending_entry && pending_entry->is_renderer_initiated() &&
      (pending_entry->GetURL() == url);

  // If there is a transient entry, creating a new pending entry will result
  // in deleting it, which leads to inconsistent state.
  bool has_transient_entry = !!controller_->GetTransientEntry();

  if (!has_browser_initiated_pending_entry && !has_transient_entry &&
      !renderer_provisional_load_to_pending_url) {
    std::unique_ptr<NavigationEntryImpl> entry =
        NavigationEntryImpl::FromNavigationEntry(
            NavigationController::CreateNavigationEntry(
                url, content::Referrer(), ui::PAGE_TRANSITION_LINK,
                true /* is_renderer_initiated */, std::string(),
                controller_->GetBrowserContext(),
                nullptr /* blob_url_loader_factory */));
    entry->set_site_instance(site_instance);
    // TODO(creis): If there's a pending entry already, find a safe way to
    // update it instead of replacing it and copying over things like this.
    // That will allow us to skip the NavigationHandle update below as well.
    if (pending_entry) {
      entry->set_transferred_global_request_id(
          pending_entry->transferred_global_request_id());
      entry->set_should_replace_entry(pending_entry->should_replace_entry());
      entry->SetRedirectChain(pending_entry->GetRedirectChain());
    }

    controller_->SetPendingEntry(std::move(entry));
    if (delegate_)
      delegate_->NotifyChangedNavigationState(content::INVALIDATE_TYPE_URL);
  }
}

}  // namespace content
