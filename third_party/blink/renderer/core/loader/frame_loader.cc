/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
 * Copyright (C) 2011 Kris Jordan <krisjordan@gmail.com>
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/frame_loader.h"

#include <memory>
#include "base/auto_reset.h"
#include "services/network/public/mojom/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_network_provider.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/public/web/web_history_item.h"
#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/viewport_description.h"
#include "third_party/blink/renderer/core/events/gesture_event.h"
#include "third_party/blink/renderer/core/events/keyboard_event.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/events/page_transition_event.h"
#include "third_party/blink/renderer/core/frame/content_settings_client.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/visual_viewport.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/loader/appcache/application_cache_host.h"
#include "third_party/blink/renderer/core/loader/document_load_timing.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/form_submission.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/loader/link_loader.h"
#include "third_party/blink/renderer/core/loader/navigation_scheduler.h"
#include "third_party/blink/renderer/core/loader/network_hints_interface.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/create_window.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scrolling/scrolling_coordinator.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image.h"
#include "third_party/blink/renderer/core/xml/parser/xml_document_parser.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instance_counters.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"
#include "third_party/blink/renderer/platform/plugins/plugin_script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scroll/scroll_animator_base.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

using blink::WebURLRequest;

namespace blink {

using namespace HTMLNames;

bool IsBackForwardLoadType(FrameLoadType type) {
  return type == kFrameLoadTypeBackForward ||
         type == kFrameLoadTypeInitialHistoryLoad;
}

bool IsReloadLoadType(FrameLoadType type) {
  return type == kFrameLoadTypeReload ||
         type == kFrameLoadTypeReloadBypassingCache;
}

static bool NeedsHistoryItemRestore(FrameLoadType type) {
  // FrameLoadtypeInitialHistoryLoad is intentionally excluded.
  return type == kFrameLoadTypeBackForward || IsReloadLoadType(type);
}

static NavigationPolicy MaybeCheckCSP(
    const ResourceRequest& request,
    NavigationType type,
    LocalFrame* frame,
    NavigationPolicy policy,
    bool should_check_main_world_content_security_policy,
    ContentSecurityPolicy::CheckHeaderType check_header_type) {
  // TODO(arthursonzogni): 'frame-src' check is disabled on the
  // renderer side, but is enforced on the browser side.
  // See http://crbug.com/692595 for understanding why it
  // can't be enforced on both sides instead.

  // 'form-action' check in the frame that is navigating is disabled on the
  // renderer side, but is enforced on the browser side instead.
  // N.B. check in the frame that initiates the navigation stills occurs in
  // blink and is not enforced on the browser-side.
  // TODO(arthursonzogni) The 'form-action' check should be fully disabled
  // in blink, except when the form submission doesn't trigger a navigation
  // (i.e. javascript urls). Please see https://crbug.com/701749.

  // TODO(dgozman): find better place for the comments above and
  // remove this method entirely.
  return policy;
}

static SinglePageAppNavigationType CategorizeSinglePageAppNavigation(
    SameDocumentNavigationSource same_document_navigation_source,
    FrameLoadType frame_load_type) {
  // |SinglePageAppNavigationType| falls into this grid according to different
  // combinations of |FrameLoadType| and |SameDocumentNavigationSource|:
  //
  //                              HistoryApi           Default
  //  kFrameLoadTypeBackForward   illegal              otherFragmentNav
  // !kFrameLoadTypeBackForward   sameDocBack/Forward  historyPushOrReplace
  switch (same_document_navigation_source) {
    case kSameDocumentNavigationDefault:
      if (frame_load_type == kFrameLoadTypeBackForward) {
        return kSPANavTypeSameDocumentBackwardOrForward;
      }
      return kSPANavTypeOtherFragmentNavigation;
    case kSameDocumentNavigationHistoryApi:
      // It's illegal to have both kSameDocumentNavigationHistoryApi and
      // kFrameLoadTypeBackForward.
      DCHECK(frame_load_type != kFrameLoadTypeBackForward);
      return kSPANavTypeHistoryPushStateOrReplaceState;
  }
  NOTREACHED();
  return kSPANavTypeSameDocumentBackwardOrForward;
}

ResourceRequest FrameLoader::ResourceRequestForReload(
    FrameLoadType frame_load_type,
    ClientRedirectPolicy client_redirect_policy) {
  DCHECK(IsReloadLoadType(frame_load_type));
  const auto cache_mode = frame_load_type == kFrameLoadTypeReloadBypassingCache
                              ? mojom::FetchCacheMode::kBypassCache
                              : mojom::FetchCacheMode::kValidateCache;
  if (!document_loader_ || !document_loader_->GetHistoryItem())
    return ResourceRequest();
  ResourceRequest request =
      document_loader_->GetHistoryItem()->GenerateResourceRequest(cache_mode);

  // Set requestor origin to be the current URL's origin.
  request.SetRequestorOrigin(SecurityOrigin::Create(request.Url()));

  // ClientRedirectPolicy is an indication that this load was triggered by some
  // direct interaction with the page. If this reload is not a client redirect,
  // we should reuse the referrer from the original load of the current
  // document. If this reload is a client redirect (e.g., location.reload()), it
  // was initiated by something in the current document and should therefore
  // show the current document's url as the referrer.
  if (client_redirect_policy == ClientRedirectPolicy::kClientRedirect) {
    request.SetHTTPReferrer(SecurityPolicy::GenerateReferrer(
        frame_->GetDocument()->GetReferrerPolicy(),
        frame_->GetDocument()->Url(),
        frame_->GetDocument()->OutgoingReferrer()));
  }

  request.SetSkipServiceWorker(frame_load_type ==
                               kFrameLoadTypeReloadBypassingCache);
  return request;
}

FrameLoader::FrameLoader(LocalFrame* frame)
    : frame_(frame),
      progress_tracker_(ProgressTracker::Create(frame)),
      in_stop_all_loaders_(false),
      in_restore_scroll_(false),
      forced_sandbox_flags_(kSandboxNone),
      dispatching_did_clear_window_object_in_main_world_(false),
      protect_provisional_loader_(false),
      detached_(false) {
  DCHECK(frame_);

  TRACE_EVENT_OBJECT_CREATED_WITH_ID("loading", "FrameLoader", this);
  TakeObjectSnapshot();
}

FrameLoader::~FrameLoader() {
  DCHECK(detached_);
}

void FrameLoader::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_);
  visitor->Trace(progress_tracker_);
  visitor->Trace(document_loader_);
  visitor->Trace(provisional_document_loader_);
}

void FrameLoader::Init() {
  ScriptForbiddenScope forbid_scripts;

  ResourceRequest initial_request{KURL(g_empty_string)};
  initial_request.SetRequestContext(WebURLRequest::kRequestContextInternal);
  initial_request.SetFrameType(
      frame_->IsMainFrame() ? network::mojom::RequestContextFrameType::kTopLevel
                            : network::mojom::RequestContextFrameType::kNested);

  provisional_document_loader_ =
      Client()->CreateDocumentLoader(frame_, initial_request, SubstituteData(),
                                     ClientRedirectPolicy::kNotClientRedirect,
                                     base::UnguessableToken::Create());
  provisional_document_loader_->StartLoading();

  frame_->GetDocument()->CancelParsing();

  state_machine_.AdvanceTo(
      FrameLoaderStateMachine::kDisplayingInitialEmptyDocument);

  // Suppress finish notifications for initial empty documents, since they don't
  // generate start notifications.
  document_loader_->SetSentDidFinishLoad();
  if (frame_->GetPage()->Paused())
    SetDefersLoading(true);

  TakeObjectSnapshot();
}

LocalFrameClient* FrameLoader::Client() const {
  return frame_->Client();
}

void FrameLoader::SetDefersLoading(bool defers) {
  if (provisional_document_loader_)
    provisional_document_loader_->Fetcher()->SetDefersLoading(defers);

  if (Document* document = frame_->GetDocument()) {
    document->Fetcher()->SetDefersLoading(defers);
    if (defers)
      document->PauseScheduledTasks();
    else
      document->UnpauseScheduledTasks();
  }

  if (!defers)
    frame_->GetNavigationScheduler().StartTimer();
}

bool FrameLoader::ShouldSerializeScrollAnchor() {
  return frame_ && frame_->View() &&
         RuntimeEnabledFeatures::ScrollAnchorSerializationEnabled() &&
         frame_->View()->ShouldPerformScrollAnchoring();
}

void FrameLoader::SaveScrollAnchor() {
  if (!ShouldSerializeScrollAnchor())
    return;

  if (!document_loader_ || !document_loader_->GetHistoryItem() ||
      !frame_->View())
    return;

  // Shouldn't clobber anything if we might still restore later.
  if (NeedsHistoryItemRestore(document_loader_->LoadType()) &&
      !document_loader_->GetInitialScrollState().was_scrolled_by_user)
    return;

  HistoryItem* history_item = document_loader_->GetHistoryItem();
  if (ScrollableArea* layout_scrollable_area =
          frame_->View()->LayoutViewportScrollableArea()) {
    ScrollAnchor* scroll_anchor = layout_scrollable_area->GetScrollAnchor();
    DCHECK(scroll_anchor);

    const SerializedAnchor& serialized_anchor =
        scroll_anchor->GetSerializedAnchor();
    if (serialized_anchor.IsValid()) {
      history_item->SetScrollAnchorData(
          {serialized_anchor.selector,
           WebFloatPoint(serialized_anchor.relative_offset.X(),
                         serialized_anchor.relative_offset.Y()),
           serialized_anchor.simhash});
    }
  }
}

void FrameLoader::SaveScrollState() {
  if (!document_loader_ || !document_loader_->GetHistoryItem() ||
      !frame_->View())
    return;

  // Shouldn't clobber anything if we might still restore later.
  if (NeedsHistoryItemRestore(document_loader_->LoadType()) &&
      !document_loader_->GetInitialScrollState().was_scrolled_by_user)
    return;

  HistoryItem* history_item = document_loader_->GetHistoryItem();
  if (ScrollableArea* layout_scrollable_area =
          frame_->View()->LayoutViewportScrollableArea())
    history_item->SetScrollOffset(layout_scrollable_area->GetScrollOffset());
  history_item->SetVisualViewportScrollOffset(ToScrollOffset(
      frame_->GetPage()->GetVisualViewport().VisibleRect().Location()));

  if (frame_->IsMainFrame())
    history_item->SetPageScaleFactor(frame_->GetPage()->PageScaleFactor());

  Client()->DidUpdateCurrentHistoryItem();
}

void FrameLoader::DispatchUnloadEvent() {
  FrameNavigationDisabler navigation_disabler(*frame_);

  // If the frame is unloading, the provisional loader should no longer be
  // protected. It will be detached soon.
  protect_provisional_loader_ = false;
  SaveScrollState();

  if (frame_->GetDocument() && !SVGImage::IsInSVGImage(frame_->GetDocument()))
    frame_->GetDocument()->DispatchUnloadEvents();
}

void FrameLoader::DidExplicitOpen() {
  // Calling document.open counts as committing the first real document load.
  if (!state_machine_.CommittedFirstRealDocumentLoad())
    state_machine_.AdvanceTo(FrameLoaderStateMachine::kCommittedFirstRealLoad);

  // Only model a document.open() as part of a navigation if its parent is not
  // done or in the process of completing.
  if (Frame* parent = frame_->Tree().Parent()) {
    if ((parent->IsLocalFrame() &&
         ToLocalFrame(parent)->GetDocument()->LoadEventStillNeeded()) ||
        (parent->IsRemoteFrame() && parent->IsLoading())) {
      progress_tracker_->ProgressStarted(document_loader_->LoadType());
    }
  }

  // Prevent window.open(url) -- eg window.open("about:blank") -- from blowing
  // away results from a subsequent window.document.open / window.document.write
  // call. Canceling redirection here works for all cases because document.open
  // implicitly precedes document.write.
  frame_->GetNavigationScheduler().Cancel();
}

// This is only called by ScriptController::executeScriptIfJavaScriptURL and
// always contains the result of evaluating a javascript: url. This is the
// <iframe src="javascript:'html'"> case.
void FrameLoader::ReplaceDocumentWhileExecutingJavaScriptURL(
    const String& source,
    Document* owner_document) {
  Document* document = frame_->GetDocument();
  if (!document_loader_ ||
      document->PageDismissalEventBeingDispatched() != Document::kNoDismissal)
    return;

  UseCounter::Count(*document, WebFeature::kReplaceDocumentViaJavaScriptURL);

  const KURL& url = document->Url();

  // Compute this before clearing the frame, because it may need to inherit an
  // aliased security context.
  WebGlobalObjectReusePolicy global_object_reuse_policy =
      frame_->ShouldReuseDefaultView(url)
          ? WebGlobalObjectReusePolicy::kUseExisting
          : WebGlobalObjectReusePolicy::kCreateNew;

  StopAllLoaders();
  // Don't allow any new child frames to load in this frame: attaching a new
  // child frame during or after detaching children results in an attached
  // frame on a detached DOM tree, which is bad.
  SubframeLoadingDisabler disabler(document);
  frame_->DetachChildren();

  // detachChildren() potentially detaches or navigates this frame. The load
  // cannot continue in those cases.
  if (!frame_->IsAttached() || document != frame_->GetDocument())
    return;

  frame_->GetDocument()->Shutdown();
  Client()->TransitionToCommittedForNewPage();
  document_loader_->ReplaceDocumentWhileExecutingJavaScriptURL(
      url, owner_document, global_object_reuse_policy, source);
}

void FrameLoader::FinishedParsing() {
  if (state_machine_.CreatingInitialEmptyDocument())
    return;

  progress_tracker_->FinishedParsing();

  if (Client()) {
    ScriptForbiddenScope forbid_scripts;
    Client()->DispatchDidFinishDocumentLoad();
  }

  if (Client()) {
    Client()->RunScriptsAtDocumentReady(
        document_loader_ ? document_loader_->IsCommittedButEmpty() : true);
  }

  frame_->GetDocument()->CheckCompleted();

  if (!frame_->View())
    return;

  // Check if the scrollbars are really needed for the content. If not, remove
  // them, relayout, and repaint.
  frame_->View()->RestoreScrollbar();
  ProcessFragment(frame_->GetDocument()->Url(), document_loader_->LoadType(),
                  kNavigationToDifferentDocument);
}

bool FrameLoader::AllAncestorsAreComplete() const {
  for (Frame* ancestor = frame_; ancestor;
       ancestor = ancestor->Tree().Parent()) {
    if (ancestor->IsLoading())
      return false;
  }
  return true;
}

void FrameLoader::DidFinishNavigation() {
  // We should have either finished the provisional or committed navigation if
  // this is called. Only delcare the whole frame finished if neither is in
  // progress.
  DCHECK((document_loader_ && document_loader_->SentDidFinishLoad()) ||
         !HasProvisionalNavigation());
  if (!document_loader_ || !document_loader_->SentDidFinishLoad() ||
      HasProvisionalNavigation()) {
    return;
  }

  // This code in this block is meant to prepare a document for display, but
  // this code may also run on a document being unloaded. In that case, which
  // is detectable via protect_provisional_loader_, skip the display work.
  if (frame_->IsLoading() && !protect_provisional_loader_) {
    progress_tracker_->ProgressCompleted();
    // Retry restoring scroll offset since finishing loading disables content
    // size clamping.
    RestoreScrollPositionAndViewState();
    if (document_loader_)
      document_loader_->SetLoadType(kFrameLoadTypeStandard);
    frame_->DomWindow()->FinishedLoading();
  }

  Frame* parent = frame_->Tree().Parent();
  if (parent)
    parent->CheckCompleted();
}

Frame* FrameLoader::Opener() {
  return Client() ? Client()->Opener() : nullptr;
}

void FrameLoader::SetOpener(LocalFrame* opener) {
  // If the frame is already detached, the opener has already been cleared.
  if (Client())
    Client()->SetOpener(opener);
}

bool FrameLoader::AllowPlugins(ReasonForCallingAllowPlugins reason) {
  // With Oilpan, a FrameLoader might be accessed after the Page has been
  // detached. FrameClient will not be accessible, so bail early.
  if (!Client())
    return false;
  Settings* settings = frame_->GetSettings();
  bool allowed = settings && settings->GetPluginsEnabled();
  if (!allowed && reason == kAboutToInstantiatePlugin)
    frame_->GetContentSettingsClient()->DidNotAllowPlugins();
  return allowed;
}

void FrameLoader::UpdateForSameDocumentNavigation(
    const KURL& new_url,
    SameDocumentNavigationSource same_document_navigation_source,
    scoped_refptr<SerializedScriptValue> data,
    HistoryScrollRestorationType scroll_restoration_type,
    FrameLoadType type,
    Document* initiating_document) {
  SinglePageAppNavigationType single_page_app_navigation_type =
      CategorizeSinglePageAppNavigation(same_document_navigation_source, type);
  UMA_HISTOGRAM_ENUMERATION(
      "RendererScheduler.UpdateForSameDocumentNavigationCount",
      single_page_app_navigation_type, kSPANavTypeCount);

  TRACE_EVENT1("blink", "FrameLoader::updateForSameDocumentNavigation", "url",
               new_url.GetString().Ascii().data());

  // Generate start and stop notifications only when loader is completed so that
  // we don't fire them for fragment redirection that happens in window.onload
  // handler. See https://bugs.webkit.org/show_bug.cgi?id=31838
  // Do not fire the notifications if the frame is concurrently navigating away
  // from the document, since a new document is already loading.
  bool was_loading = frame_->IsLoading();
  if (!was_loading)
    Client()->DidStartLoading(kNavigationWithinSameDocument);

  // Update the data source's request with the new URL to fake the URL change
  frame_->GetDocument()->SetURL(new_url);
  GetDocumentLoader()->UpdateForSameDocumentNavigation(
      new_url, same_document_navigation_source, std::move(data),
      scroll_restoration_type, type, initiating_document);
  if (!was_loading)
    Client()->DidStopLoading();
}

void FrameLoader::DetachDocumentLoader(Member<DocumentLoader>& loader) {
  if (!loader)
    return;

  FrameNavigationDisabler navigation_disabler(*frame_);
  loader->DetachFromFrame();
  loader = nullptr;
}

void FrameLoader::ClearInitialScrollState() {
  document_loader_->GetInitialScrollState().was_scrolled_by_user = false;
  document_loader_->GetInitialScrollState().was_scrolled_by_js = false;
}

void FrameLoader::LoadInSameDocument(
    const KURL& url,
    scoped_refptr<SerializedScriptValue> state_object,
    FrameLoadType frame_load_type,
    HistoryItem* history_item,
    ClientRedirectPolicy client_redirect,
    Document* initiating_document) {
  // If we have a state object, we cannot also be a new navigation.
  DCHECK(!state_object || frame_load_type == kFrameLoadTypeBackForward);

  // If we have a provisional request for a different document, a fragment
  // scroll should cancel it.
  DetachDocumentLoader(provisional_document_loader_);

  if (!frame_->GetPage())
    return;
  SaveScrollState();

  KURL old_url = frame_->GetDocument()->Url();
  bool hash_change = EqualIgnoringFragmentIdentifier(url, old_url) &&
                     url.FragmentIdentifier() != old_url.FragmentIdentifier();
  if (hash_change) {
    // If we were in the autoscroll/middleClickAutoscroll mode we want to stop
    // it before following the link to the anchor
    frame_->GetEventHandler().StopAutoscroll();
    frame_->DomWindow()->EnqueueHashchangeEvent(old_url, url);
  }
  document_loader_->SetIsClientRedirect(client_redirect ==
                                        ClientRedirectPolicy::kClientRedirect);
  if (history_item)
    document_loader_->SetItemForHistoryNavigation(history_item);
  UpdateForSameDocumentNavigation(url, kSameDocumentNavigationDefault, nullptr,
                                  kScrollRestorationAuto, frame_load_type,
                                  initiating_document);

  ClearInitialScrollState();

  frame_->GetDocument()->CheckCompleted();

  // onpopstate might change view state, so stash for later restore.
  std::unique_ptr<HistoryItem::ViewState> view_state;
  if (history_item && history_item->GetViewState()) {
    view_state =
        std::make_unique<HistoryItem::ViewState>(*history_item->GetViewState());
  }

  frame_->DomWindow()->StatePopped(state_object
                                       ? std::move(state_object)
                                       : SerializedScriptValue::NullValue());

  if (history_item) {
    RestoreScrollPositionAndViewState(frame_load_type, kHistorySameDocumentLoad,
                                      view_state.get(),
                                      history_item->ScrollRestorationType());
  }

  // We need to scroll to the fragment whether or not a hash change occurred,
  // since the user might have scrolled since the previous navigation.
  ProcessFragment(url, frame_load_type, kNavigationWithinSameDocument);

  TakeObjectSnapshot();
}

// static
void FrameLoader::SetReferrerForFrameRequest(FrameLoadRequest& frame_request) {
  ResourceRequest& request = frame_request.GetResourceRequest();
  Document* origin_document = frame_request.OriginDocument();

  if (!origin_document)
    return;
  // Anchor elements with the 'referrerpolicy' attribute will have already set
  // the referrer on the request.
  if (request.DidSetHTTPReferrer())
    return;
  if (frame_request.GetShouldSendReferrer() == kNeverSendReferrer)
    return;

  // Always use the initiating document to generate the referrer. We need to
  // generateReferrer(), because we haven't enforced ReferrerPolicy or
  // https->http referrer suppression yet.
  Referrer referrer = SecurityPolicy::GenerateReferrer(
      origin_document->GetReferrerPolicy(), request.Url(),
      origin_document->OutgoingReferrer());

  request.SetHTTPReferrer(referrer);
  request.SetHTTPOriginToMatchReferrerIfNeeded();
}

FrameLoadType FrameLoader::DetermineFrameLoadType(
    const FrameLoadRequest& request) {
  if (frame_->Tree().Parent() &&
      !state_machine_.CommittedFirstRealDocumentLoad())
    return kFrameLoadTypeInitialInChildFrame;
  if (!frame_->Tree().Parent() && !Client()->BackForwardLength()) {
    if (Opener() && request.GetResourceRequest().Url().IsEmpty())
      return kFrameLoadTypeReplaceCurrentItem;
    return kFrameLoadTypeStandard;
  }
  if (request.GetResourceRequest().GetCacheMode() ==
      mojom::FetchCacheMode::kValidateCache)
    return kFrameLoadTypeReload;
  if (request.GetResourceRequest().GetCacheMode() ==
      mojom::FetchCacheMode::kBypassCache)
    return kFrameLoadTypeReloadBypassingCache;
  // From the HTML5 spec for location.assign():
  // "If the browsing context's session history contains only one Document,
  // and that was the about:blank Document created when the browsing context
  // was created, then the navigation must be done with replacement enabled."
  if (request.ReplacesCurrentItem() ||
      (!state_machine_.CommittedMultipleRealLoads() &&
       DeprecatedEqualIgnoringCase(frame_->GetDocument()->Url(), BlankURL())))
    return kFrameLoadTypeReplaceCurrentItem;

  if (request.GetResourceRequest().Url() == document_loader_->UrlForHistory()) {
    if (request.GetResourceRequest().HttpMethod() == HTTPNames::POST)
      return kFrameLoadTypeStandard;
    if (!request.OriginDocument())
      return kFrameLoadTypeReload;
    return kFrameLoadTypeReplaceCurrentItem;
  }

  if (request.GetSubstituteData().FailingURL() ==
          document_loader_->UrlForHistory() &&
      document_loader_->LoadType() == kFrameLoadTypeReload)
    return kFrameLoadTypeReload;

  if (request.GetResourceRequest().Url().IsEmpty() &&
      request.GetSubstituteData().FailingURL().IsEmpty()) {
    return kFrameLoadTypeReplaceCurrentItem;
  }

  if (request.OriginDocument() &&
      !request.OriginDocument()->CanCreateHistoryEntry())
    return kFrameLoadTypeReplaceCurrentItem;

  return kFrameLoadTypeStandard;
}

bool FrameLoader::PrepareRequestForThisFrame(FrameLoadRequest& request) {
  // If no origin Document* was specified, skip remaining security checks and
  // assume the caller has fully initialized the FrameLoadRequest.
  if (!request.OriginDocument())
    return true;

  KURL url = request.GetResourceRequest().Url();
  if (frame_->GetScriptController().ExecuteScriptIfJavaScriptURL(url, nullptr))
    return false;

  if (!request.OriginDocument()->GetSecurityOrigin()->CanDisplay(url)) {
    request.OriginDocument()->AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Not allowed to load local resource: " + url.ElidedString()));
    return false;
  }

  // Block renderer-initiated loads of data: and filesystem: URLs in the top
  // frame.
  //
  // If the mime type of the data URL is supported, the URL will
  // eventually be rendered, so block it here. Otherwise, the load might be
  // handled by a plugin or end up as a download, so allow it to let the
  // embedder figure out what to do with it. Navigations to filesystem URLs are
  // always blocked here.
  if (frame_->IsMainFrame() &&
      !request.GetResourceRequest().IsSameDocumentNavigation() &&
      !frame_->Client()->AllowContentInitiatedDataUrlNavigations(
          request.OriginDocument()->Url()) &&
      (url.ProtocolIs("filesystem") ||
       (url.ProtocolIsData() &&
        NetworkUtils::IsDataURLMimeTypeSupported(url)))) {
    frame_->GetDocument()->AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Not allowed to navigate top frame to " + url.Protocol() +
            " URL: " + url.ElidedString()));
    return false;
  }

  if (!request.Form() && request.FrameName().IsEmpty())
    request.SetFrameName(frame_->GetDocument()->BaseTarget());
  return true;
}

static bool ShouldNavigateTargetFrame(NavigationPolicy policy) {
  switch (policy) {
    case kNavigationPolicyCurrentTab:
      return true;

    // Navigation will target a *new* frame (e.g. because of a ctrl-click),
    // so the target frame can be ignored.
    case kNavigationPolicyNewBackgroundTab:
    case kNavigationPolicyNewForegroundTab:
    case kNavigationPolicyNewWindow:
    case kNavigationPolicyNewPopup:
      return false;

    // Navigation won't really target any specific frame,
    // so the target frame can be ignored.
    case kNavigationPolicyIgnore:
    case kNavigationPolicyDownload:
      return false;

    case kNavigationPolicyHandledByClient:
      // Impossible, because at this point we shouldn't yet have called
      // client()->decidePolicyForNavigation(...).
      NOTREACHED();
      return true;

    default:
      NOTREACHED() << policy;
      return true;
  }
}

static NavigationType DetermineNavigationType(FrameLoadType frame_load_type,
                                              bool is_form_submission,
                                              bool have_event) {
  bool is_reload = IsReloadLoadType(frame_load_type);
  bool is_back_forward = IsBackForwardLoadType(frame_load_type);
  if (is_form_submission) {
    return (is_reload || is_back_forward) ? kNavigationTypeFormResubmitted
                                          : kNavigationTypeFormSubmitted;
  }
  if (have_event)
    return kNavigationTypeLinkClicked;
  if (is_reload)
    return kNavigationTypeReload;
  if (is_back_forward)
    return kNavigationTypeBackForward;
  return kNavigationTypeOther;
}

static WebURLRequest::RequestContext DetermineRequestContextFromNavigationType(
    const NavigationType navigation_type) {
  switch (navigation_type) {
    case kNavigationTypeLinkClicked:
      return WebURLRequest::kRequestContextHyperlink;

    case kNavigationTypeOther:
      return WebURLRequest::kRequestContextLocation;

    case kNavigationTypeFormResubmitted:
    case kNavigationTypeFormSubmitted:
      return WebURLRequest::kRequestContextForm;

    case kNavigationTypeBackForward:
    case kNavigationTypeReload:
      return WebURLRequest::kRequestContextInternal;
  }
  NOTREACHED();
  return WebURLRequest::kRequestContextHyperlink;
}

static NavigationPolicy NavigationPolicyForRequest(
    const FrameLoadRequest& request) {
  NavigationPolicy policy = kNavigationPolicyCurrentTab;
  Event* event = request.TriggeringEvent();
  if (!event)
    return policy;

  if (request.Form() && event->UnderlyingEvent())
    event = event->UnderlyingEvent();

  if (event->IsMouseEvent()) {
    MouseEvent* mouse_event = ToMouseEvent(event);
    NavigationPolicyFromMouseEvent(
        mouse_event->button(), mouse_event->ctrlKey(), mouse_event->shiftKey(),
        mouse_event->altKey(), mouse_event->metaKey(), &policy);
  } else if (event->IsKeyboardEvent()) {
    // The click is simulated when triggering the keypress event.
    KeyboardEvent* key_event = ToKeyboardEvent(event);
    NavigationPolicyFromMouseEvent(0, key_event->ctrlKey(),
                                   key_event->shiftKey(), key_event->altKey(),
                                   key_event->metaKey(), &policy);
  } else if (event->IsGestureEvent()) {
    // The click is simulated when triggering the gesture-tap event
    GestureEvent* gesture_event = ToGestureEvent(event);
    NavigationPolicyFromMouseEvent(
        0, gesture_event->ctrlKey(), gesture_event->shiftKey(),
        gesture_event->altKey(), gesture_event->metaKey(), &policy);
  }
  return policy;
}

void FrameLoader::StartNavigation(const FrameLoadRequest& passed_request,
                                  FrameLoadType frame_load_type,
                                  HistoryItem* history_item,
                                  HistoryLoadType history_load_type) {
  CHECK(!passed_request.GetSubstituteData().IsValid());
  CHECK(frame_load_type != kFrameLoadTypeBackForward);
  CHECK(!history_item);
  CHECK(history_load_type == kHistoryDifferentDocumentLoad);
  return LoadInternal(passed_request, frame_load_type, history_item,
                      history_load_type, true /* check_with_client */);
}

void FrameLoader::CommitNavigation(const FrameLoadRequest& passed_request,
                                   FrameLoadType frame_load_type,
                                   HistoryItem* history_item,
                                   HistoryLoadType history_load_type) {
  CHECK(!passed_request.OriginDocument());
  CHECK(passed_request.FrameName().IsEmpty());
  CHECK(!passed_request.TriggeringEvent());
  CHECK(!passed_request.Form());
  return LoadInternal(passed_request, frame_load_type, history_item,
                      history_load_type, false /* check_with_client */);
}

void FrameLoader::LoadInternal(const FrameLoadRequest& passed_request,
                               FrameLoadType frame_load_type,
                               HistoryItem* history_item,
                               HistoryLoadType history_load_type,
                               bool check_with_client) {
  DCHECK(frame_->GetDocument());

  if (HTMLFrameOwnerElement* element = frame_->DeprecatedLocalOwner())
    element->CancelPendingLazyLoad();

  if (IsBackForwardLoadType(frame_load_type) && !frame_->IsNavigationAllowed())
    return;

  if (in_stop_all_loaders_)
    return;

  FrameLoadRequest request(passed_request);
  request.GetResourceRequest().SetHasUserGesture(
      Frame::HasTransientUserActivation(frame_));

  if (!PrepareRequestForThisFrame(request))
    return;

  // Form submissions appear to need their special-case of finding the target at
  // schedule rather than at fire.
  Frame* target_frame = request.Form()
                            ? nullptr
                            : frame_->FindFrameForNavigation(
                                  AtomicString(request.FrameName()), *frame_,
                                  request.GetResourceRequest().Url());

  NavigationPolicy policy = NavigationPolicyForRequest(request);
  if (target_frame && target_frame != frame_ &&
      ShouldNavigateTargetFrame(policy)) {
    if (target_frame->IsLocalFrame() &&
        !ToLocalFrame(target_frame)->IsNavigationAllowed()) {
      return;
    }

    bool was_in_same_page = target_frame->GetPage() == frame_->GetPage();

    request.SetFrameName("_self");
    target_frame->Navigate(request);
    Page* page = target_frame->GetPage();
    if (!was_in_same_page && page)
      page->GetChromeClient().Focus(frame_);
    return;
  }

  SetReferrerForFrameRequest(request);

  if (!target_frame && !request.FrameName().IsEmpty()) {
    if (policy == kNavigationPolicyDownload) {
      Client()->DownloadURL(request.GetResourceRequest());
      return;  // Navigation/download will be handled by the client.
    } else if (ShouldNavigateTargetFrame(policy)) {
      request.GetResourceRequest().SetFrameType(
          network::mojom::RequestContextFrameType::kAuxiliary);
      CreateWindowForRequest(request, *frame_, policy);
      return;  // Navigation will be handled by the new frame/window.
    }
  }

  if (!frame_->IsNavigationAllowed())
    return;

  const KURL& url = request.GetResourceRequest().Url();
  FrameLoadType new_load_type = (frame_load_type == kFrameLoadTypeStandard)
                                    ? DetermineFrameLoadType(request)
                                    : frame_load_type;

  bool same_document_history_navigation =
      IsBackForwardLoadType(new_load_type) &&
      history_load_type == kHistorySameDocumentLoad;
  bool same_document_navigation =
      policy == kNavigationPolicyCurrentTab &&
      ShouldPerformFragmentNavigation(request.Form(),
                                      request.GetResourceRequest().HttpMethod(),
                                      new_load_type, url);

  // Perform same document navigation.
  if (same_document_history_navigation || same_document_navigation) {
    CommitSameDocumentNavigation(
        request.GetResourceRequest().Url(), new_load_type, history_item,
        request.ClientRedirect(), request.OriginDocument(),
        request.TriggeringEvent());
    return;
  }

  // PlzNavigate
  // If the loader classifies this navigation as a different document navigation
  // while the browser intended the navigation to be same-document, it means
  // that a different navigation must have committed while the IPC was sent.
  // This navigation is no more same-document. The navigation is simply dropped.
  if (request.GetResourceRequest().IsSameDocumentNavigation())
    return;

  StartLoad(request, new_load_type, policy, history_item, check_with_client);
}

mojom::CommitResult FrameLoader::CommitSameDocumentNavigation(
    const KURL& url,
    FrameLoadType frame_load_type,
    HistoryItem* history_item,
    ClientRedirectPolicy client_redirect_policy,
    Document* origin_document,
    Event* triggering_event) {
  DCHECK(!IsReloadLoadType(frame_load_type));
  DCHECK(frame_->GetDocument());

  if (in_stop_all_loaders_)
    return mojom::CommitResult::Aborted;

  bool history_navigation = IsBackForwardLoadType(frame_load_type);

  if (!frame_->IsNavigationAllowed() && history_navigation)
    return mojom::CommitResult::Aborted;

  if (!history_navigation) {
    // In the case of non-history navigations, check that this is a
    // same-document navigation. If not, the navigation should restart as a
    // cross-document navigation.
    if (!url.HasFragmentIdentifier() ||
        !EqualIgnoringFragmentIdentifier(frame_->GetDocument()->Url(), url) ||
        frame_->GetDocument()->IsFrameSet()) {
      return mojom::CommitResult::RestartCrossDocument;
    }
  }

  DCHECK(history_item || !history_navigation);
  scoped_refptr<SerializedScriptValue> state_object =
      history_navigation ? history_item->StateObject() : nullptr;

  if (!history_navigation) {
    document_loader_->SetNavigationType(
        DetermineNavigationType(frame_load_type, false, triggering_event));
    if (ShouldTreatURLAsSameAsCurrent(url))
      frame_load_type = kFrameLoadTypeReplaceCurrentItem;
  }

  // Perform the same-document navigation.
  LoadInSameDocument(url, state_object, frame_load_type, history_item,
                     client_redirect_policy, origin_document);
  return mojom::CommitResult::Ok;
}

SubstituteData FrameLoader::DefaultSubstituteDataForURL(const KURL& url) {
  if (!ShouldTreatURLAsSrcdocDocument(url))
    return SubstituteData();
  String srcdoc = frame_->DeprecatedLocalOwner()->FastGetAttribute(srcdocAttr);
  DCHECK(!srcdoc.IsNull());
  CString encoded_srcdoc = srcdoc.Utf8();
  return SubstituteData(
      SharedBuffer::Create(encoded_srcdoc.data(), encoded_srcdoc.length()),
      "text/html", "UTF-8", NullURL());
}

void FrameLoader::StopAllLoaders() {
  if (frame_->GetDocument()->PageDismissalEventBeingDispatched() !=
      Document::kNoDismissal)
    return;

  // If this method is called from within this method, infinite recursion can
  // occur (3442218). Avoid this.
  if (in_stop_all_loaders_)
    return;

  base::AutoReset<bool> in_stop_all_loaders(&in_stop_all_loaders_, true);

  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().NextSibling()) {
    if (child->IsLocalFrame())
      ToLocalFrame(child)->Loader().StopAllLoaders();
  }

  frame_->GetDocument()->CancelParsing();
  if (document_loader_)
    document_loader_->StopLoading();
  if (!protect_provisional_loader_)
    DetachDocumentLoader(provisional_document_loader_);
  frame_->GetNavigationScheduler().Cancel();
  DidFinishNavigation();

  TakeObjectSnapshot();
}

void FrameLoader::DidAccessInitialDocument() {
  // We only need to notify the client for the main frame.
  if (IsLoadingMainFrame()) {
    // Forbid script execution to prevent re-entering V8, since this is called
    // from a binding security check.
    ScriptForbiddenScope forbid_scripts;
    if (Client())
      Client()->DidAccessInitialDocument();
  }
}

bool FrameLoader::PrepareForCommit() {
  PluginScriptForbiddenScope forbid_plugin_destructor_scripting;
  DocumentLoader* pdl = provisional_document_loader_;

  if (frame_->GetDocument()) {
    unsigned node_count = 0;
    for (Frame* frame = frame_; frame; frame = frame->Tree().TraverseNext()) {
      if (frame->IsLocalFrame()) {
        LocalFrame* local_frame = ToLocalFrame(frame);
        node_count += local_frame->GetDocument()->NodeCount();
      }
    }
    unsigned total_node_count =
        InstanceCounters::CounterValue(InstanceCounters::kNodeCounter);
    float ratio = static_cast<float>(node_count) / total_node_count;
    ThreadState::Current()->SchedulePageNavigationGCIfNeeded(ratio);
  }

  // Don't allow any new child frames to load in this frame: attaching a new
  // child frame during or after detaching children results in an attached frame
  // on a detached DOM tree, which is bad.
  SubframeLoadingDisabler disabler(frame_->GetDocument());
  if (document_loader_) {
    Client()->DispatchWillCommitProvisionalLoad();
    DispatchUnloadEvent();
  }
  frame_->DetachChildren();
  // The previous calls to dispatchUnloadEvent() and detachChildren() can
  // execute arbitrary script via things like unload events. If the executed
  // script intiates a new load or causes the current frame to be detached, we
  // need to abandon the current load.
  if (pdl != provisional_document_loader_)
    return false;
  // detachFromFrame() will abort XHRs that haven't completed, which can trigger
  // event listeners for 'abort'. These event listeners might call
  // window.stop(), which will in turn detach the provisional document loader.
  // At this point, the provisional document loader should not detach, because
  // then the FrameLoader would not have any attached DocumentLoaders.
  if (document_loader_) {
    base::AutoReset<bool> in_detach_document_loader(
        &protect_provisional_loader_, true);
    DetachDocumentLoader(document_loader_);
  }
  // 'abort' listeners can also detach the frame.
  if (!frame_->Client())
    return false;
  DCHECK_EQ(provisional_document_loader_, pdl);
  // No more events will be dispatched so detach the Document.
  // TODO(yoav): Should we also be nullifying domWindow's document (or
  // domWindow) since the doc is now detached?
  if (frame_->GetDocument())
    frame_->GetDocument()->Shutdown();
  document_loader_ = provisional_document_loader_.Release();
  if (document_loader_)
    document_loader_->MarkAsCommitted();

  TakeObjectSnapshot();

  return true;
}

void FrameLoader::CommitProvisionalLoad() {
  DCHECK(Client()->HasWebView());

  // Check if the destination page is allowed to access the previous page's
  // timing information.
  if (frame_->GetDocument()) {
    scoped_refptr<const SecurityOrigin> security_origin =
        SecurityOrigin::Create(provisional_document_loader_->Url());
    provisional_document_loader_->GetTiming()
        .SetHasSameOriginAsPreviousDocument(
            security_origin->CanRequest(frame_->GetDocument()->Url()));
  }

  if (!PrepareForCommit())
    return;

  // If we are loading a local root, it is important to explicitly set the event
  // listener properties to Nothing as this triggers notifications to the
  // client. Clients may assume the presence of handlers for touch and wheel
  // events, so these notifications tell it there are (presently) no handlers.
  if (frame_->IsLocalRoot()) {
    frame_->GetPage()->GetChromeClient().SetEventListenerProperties(
        frame_, WebEventListenerClass::kTouchStartOrMove,
        WebEventListenerProperties::kNothing);
    frame_->GetPage()->GetChromeClient().SetEventListenerProperties(
        frame_, WebEventListenerClass::kMouseWheel,
        WebEventListenerProperties::kNothing);
    frame_->GetPage()->GetChromeClient().SetEventListenerProperties(
        frame_, WebEventListenerClass::kTouchEndOrCancel,
        WebEventListenerProperties::kNothing);
  }

  Client()->TransitionToCommittedForNewPage();

  frame_->GetNavigationScheduler().Cancel();
}

bool FrameLoader::IsLoadingMainFrame() const {
  return frame_->IsMainFrame();
}

void FrameLoader::RestoreScrollPositionAndViewState() {
  if (!frame_->GetPage() || !GetDocumentLoader() ||
      !GetDocumentLoader()->GetHistoryItem() || in_restore_scroll_) {
    return;
  }
  base::AutoReset<bool> in_restore_scroll(&in_restore_scroll_, true);
  RestoreScrollPositionAndViewState(
      GetDocumentLoader()->LoadType(), kHistoryDifferentDocumentLoad,
      GetDocumentLoader()->GetHistoryItem()->GetViewState(),
      GetDocumentLoader()->GetHistoryItem()->ScrollRestorationType());
}

void FrameLoader::RestoreScrollPositionAndViewState(
    FrameLoadType load_type,
    HistoryLoadType history_load_type,
    HistoryItem::ViewState* view_state,
    HistoryScrollRestorationType scroll_restoration_type) {
  LocalFrameView* view = frame_->View();
  if (!view || !view->LayoutViewportScrollableArea() ||
      !state_machine_.CommittedFirstRealDocumentLoad() ||
      !frame_->IsAttached()) {
    return;
  }
  if (!NeedsHistoryItemRestore(load_type) || !view_state)
    return;

  bool should_restore_scroll =
      scroll_restoration_type != kScrollRestorationManual;
  bool should_restore_scale = view_state->page_scale_factor_;

  // This tries to balance:
  // 1. restoring as soon as possible.
  // 2. not overriding user scroll (TODO(majidvp): also respect user scale).
  // 3. detecting clamping to avoid repeatedly popping the scroll position down
  //    as the page height increases.
  // 4. forcing a layout if necessary to avoid clamping.
  // 5. ignoring clamp detection if scroll state is not being restored, if load
  //    is complete, or if the navigation is same-document (as the new page may
  //    be smaller than the previous page).
  bool can_restore_without_clamping =
      view->LayoutViewportScrollableArea()->ClampScrollOffset(
          view_state->scroll_offset_) == view_state->scroll_offset_;

  bool should_force_clamping =
      !frame_->IsLoading() || history_load_type == kHistorySameDocumentLoad;
  // Here |can_restore_without_clamping| is false, but layout might be necessary
  // to ensure correct content size.
  if (!can_restore_without_clamping && should_force_clamping)
    frame_->GetDocument()->UpdateStyleAndLayout();

  bool can_restore_without_annoying_user =
      !GetDocumentLoader()->GetInitialScrollState().was_scrolled_by_user &&
      (can_restore_without_clamping || should_force_clamping ||
       !should_restore_scroll);
  if (!can_restore_without_annoying_user)
    return;

  if (should_restore_scroll) {
    ScrollOffset previous_offset =
        view->LayoutViewportScrollableArea()->GetScrollOffset();

    // TODO(pnoland): attempt to restore the anchor in more places than this.
    // Anchor-based restore should allow for earlier restoration.
    bool did_restore =
        ShouldSerializeScrollAnchor() &&
        view->LayoutViewportScrollableArea()->RestoreScrollAnchor(
            {view_state->scroll_anchor_data_.selector_,
             LayoutPoint(view_state->scroll_anchor_data_.offset_.x,
                         view_state->scroll_anchor_data_.offset_.y),
             view_state->scroll_anchor_data_.simhash_});
    if (!did_restore) {
      view->LayoutViewportScrollableArea()->SetScrollOffset(
          view_state->scroll_offset_, kProgrammaticScroll);
    }

    did_restore |= (previous_offset !=
                    view->LayoutViewportScrollableArea()->GetScrollOffset());

    // Measure how many successful scroll restoration may impacted if we allow
    // using js scroll to prevent browser scroll restoration.
    if (did_restore) {
      UMA_HISTOGRAM_BOOLEAN(
          "Layout.ScrollRestoration.PrecededByJsScroll",
          GetDocumentLoader()->GetInitialScrollState().was_scrolled_by_js);
    }
  }

  // For main frame restore scale and visual viewport position
  if (frame_->IsMainFrame()) {
    ScrollOffset visual_viewport_offset(
        view_state->visual_viewport_scroll_offset_);

    // If the visual viewport's offset is (-1, -1) it means the history item
    // is an old version of HistoryItem so distribute the scroll between
    // the main frame and the visual viewport as best as we can.
    if (visual_viewport_offset.Width() == -1 &&
        visual_viewport_offset.Height() == -1) {
      visual_viewport_offset =
          view_state->scroll_offset_ -
          view->LayoutViewportScrollableArea()->GetScrollOffset();
    }

    VisualViewport& visual_viewport = frame_->GetPage()->GetVisualViewport();
    if (should_restore_scale && should_restore_scroll) {
      visual_viewport.SetScaleAndLocation(view_state->page_scale_factor_,
                                          FloatPoint(visual_viewport_offset));
    } else if (should_restore_scale) {
      visual_viewport.SetScale(view_state->page_scale_factor_);
    } else if (should_restore_scroll) {
      visual_viewport.SetLocation(FloatPoint(visual_viewport_offset));
    }

    if (ScrollingCoordinator* scrolling_coordinator =
            frame_->GetPage()->GetScrollingCoordinator())
      scrolling_coordinator->FrameViewRootLayerDidChange(view);
  }

  GetDocumentLoader()->GetInitialScrollState().did_restore_from_history = true;
}

String FrameLoader::UserAgent() const {
  String user_agent = Client()->UserAgent();
  probe::applyUserAgentOverride(frame_->GetDocument(), &user_agent);
  return user_agent;
}

void FrameLoader::Detach() {
  DetachDocumentLoader(document_loader_);
  DetachDocumentLoader(provisional_document_loader_);

  if (progress_tracker_) {
    progress_tracker_->Dispose();
    progress_tracker_.Clear();
  }

  TRACE_EVENT_OBJECT_DELETED_WITH_ID("loading", "FrameLoader", this);
  detached_ = true;
}

void FrameLoader::DetachProvisionalDocumentLoader(DocumentLoader* loader) {
  DCHECK_EQ(loader, provisional_document_loader_);
  DetachDocumentLoader(provisional_document_loader_);
  DidFinishNavigation();
}

bool FrameLoader::ShouldPerformFragmentNavigation(bool is_form_submission,
                                                  const String& http_method,
                                                  FrameLoadType load_type,
                                                  const KURL& url) {
  // We don't do this if we are submitting a form with method other than "GET",
  // explicitly reloading, currently displaying a frameset, or if the URL does
  // not have a fragment.
  return DeprecatedEqualIgnoringCase(http_method, HTTPNames::GET) &&
         !IsReloadLoadType(load_type) &&
         load_type != kFrameLoadTypeBackForward &&
         url.HasFragmentIdentifier() &&
         // For provisional LocalFrame, there is no real document loaded and
         // the initial empty document should not be considered, so there is
         // no way to get a same-document load in this case.
         !frame_->IsProvisional() &&
         EqualIgnoringFragmentIdentifier(frame_->GetDocument()->Url(), url)
         // We don't want to just scroll if a link from within a frameset is
         // trying to reload the frameset into _top.
         && !frame_->GetDocument()->IsFrameSet();
}

void FrameLoader::ProcessFragment(const KURL& url,
                                  FrameLoadType frame_load_type,
                                  LoadStartType load_start_type) {
  LocalFrameView* view = frame_->View();
  if (!view)
    return;

  // Leaking scroll position to a cross-origin ancestor would permit the
  // so-called "framesniffing" attack.
  Frame* boundary_frame =
      url.HasFragmentIdentifier()
          ? frame_->FindUnsafeParentScrollPropagationBoundary()
          : nullptr;

  // FIXME: Handle RemoteFrames
  if (boundary_frame && boundary_frame->IsLocalFrame()) {
    ToLocalFrame(boundary_frame)
        ->View()
        ->SetSafeToPropagateScrollToParent(false);
  }

  // If scroll position is restored from history fragment or scroll
  // restoration type is manual, then we should not override it unless this
  // is a same document reload.
  bool should_scroll_to_fragment =
      (load_start_type == kNavigationWithinSameDocument &&
       !IsBackForwardLoadType(frame_load_type)) ||
      (!GetDocumentLoader()->GetInitialScrollState().did_restore_from_history &&
       !(GetDocumentLoader()->GetHistoryItem() &&
         GetDocumentLoader()->GetHistoryItem()->ScrollRestorationType() ==
             kScrollRestorationManual));

  view->ProcessUrlFragment(url, should_scroll_to_fragment
                                    ? LocalFrameView::kUrlFragmentScroll
                                    : LocalFrameView::kUrlFragmentDontScroll);

  if (boundary_frame && boundary_frame->IsLocalFrame())
    ToLocalFrame(boundary_frame)
        ->View()
        ->SetSafeToPropagateScrollToParent(true);
}

bool FrameLoader::ShouldClose(bool is_reload) {
  Page* page = frame_->GetPage();
  if (!page || !page->GetChromeClient().CanOpenBeforeUnloadConfirmPanel())
    return true;

  // Store all references to each subframe in advance since beforeunload's event
  // handler may modify frame
  HeapVector<Member<LocalFrame>> target_frames;
  target_frames.push_back(frame_);
  for (Frame* child = frame_->Tree().FirstChild(); child;
       child = child->Tree().TraverseNext(frame_)) {
    // FIXME: There is not yet any way to dispatch events to out-of-process
    // frames.
    if (child->IsLocalFrame())
      target_frames.push_back(ToLocalFrame(child));
  }

  bool should_close = false;
  {
    NavigationDisablerForBeforeUnload navigation_disabler;
    size_t i;

    bool did_allow_navigation = false;
    for (i = 0; i < target_frames.size(); i++) {
      if (!target_frames[i]->Tree().IsDescendantOf(frame_))
        continue;
      if (!target_frames[i]->GetDocument()->DispatchBeforeUnloadEvent(
              page->GetChromeClient(), is_reload, did_allow_navigation))
        break;
    }

    if (i == target_frames.size())
      should_close = true;
  }

  return should_close;
}

NavigationPolicy FrameLoader::ShouldContinueForNavigationPolicy(
    const ResourceRequest& request,
    Document* origin_document,
    const SubstituteData& substitute_data,
    DocumentLoader* loader,
    ContentSecurityPolicyDisposition
        should_check_main_world_content_security_policy,
    NavigationType type,
    NavigationPolicy policy,
    FrameLoadType frame_load_type,
    bool is_client_redirect,
    WebTriggeringEventInfo triggering_event_info,
    HTMLFormElement* form,
    mojom::blink::BlobURLTokenPtr blob_url_token,
    bool check_with_client) {
  // Don't ask if we are loading an empty URL.
  if (request.Url().IsEmpty() || substitute_data.IsValid())
    return kNavigationPolicyCurrentTab;

  // Check for non-escaped new lines in the url.
  if (request.Url().PotentiallyDanglingMarkup() &&
      request.Url().ProtocolIsInHTTPFamily()) {
    Deprecation::CountDeprecation(
        frame_, WebFeature::kCanRequestURLHTTPContainingNewline);
    if (RuntimeEnabledFeatures::RestrictCanRequestURLCharacterSetEnabled())
      return kNavigationPolicyIgnore;
  }

  if (MaybeCheckCSP(request, type, frame_, policy,
                    should_check_main_world_content_security_policy ==
                        kCheckContentSecurityPolicy,
                    ContentSecurityPolicy::CheckHeaderType::kCheckEnforce) ==
      kNavigationPolicyIgnore) {
    return kNavigationPolicyIgnore;
  }

  bool replaces_current_history_item =
      frame_load_type == kFrameLoadTypeReplaceCurrentItem;
  policy = Client()->DecidePolicyForNavigation(
      request, origin_document, loader, type, policy,
      replaces_current_history_item, is_client_redirect, triggering_event_info,
      form, should_check_main_world_content_security_policy,
      std::move(blob_url_token));
  if (!check_with_client)
    CHECK_EQ(kNavigationPolicyCurrentTab, policy);
  DCHECK(policy == kNavigationPolicyCurrentTab ||
         policy == kNavigationPolicyIgnore ||
         policy == kNavigationPolicyHandledByClient ||
         policy == kNavigationPolicyHandledByClientForInitialHistory)
      << policy;
  return policy;
}

NavigationPolicy FrameLoader::ShouldContinueForRedirectNavigationPolicy(
    const ResourceRequest& request,
    const SubstituteData& substitute_data,
    DocumentLoader* loader,
    ContentSecurityPolicyDisposition
        should_check_main_world_content_security_policy,
    NavigationType type,
    NavigationPolicy policy,
    FrameLoadType frame_load_type,
    bool is_client_redirect,
    HTMLFormElement* form) {
  // Check report-only CSP policies, which are not checked by
  // ShouldContinueForNavigationPolicy.
  MaybeCheckCSP(request, type, frame_, policy,
                should_check_main_world_content_security_policy ==
                    kCheckContentSecurityPolicy,
                ContentSecurityPolicy::CheckHeaderType::kCheckReportOnly);

  return ShouldContinueForNavigationPolicy(
      request,
      // |origin_document| is not set. It doesn't really matter here. It is
      // useful for PlzNavigate (aka browser-side-navigation). It is used
      // during the first navigation and not during redirects.
      nullptr,  // origin_document
      substitute_data, loader, should_check_main_world_content_security_policy,
      type, policy, frame_load_type, is_client_redirect,
      WebTriggeringEventInfo::kNotFromEvent, form, nullptr /* blob_url_token */,
      true /* check_with_client */);
}

void FrameLoader::ClientDroppedNavigation() {
  if (!provisional_document_loader_ || provisional_document_loader_->DidStart())
    return;

  DetachProvisionalDocumentLoader(provisional_document_loader_);
}

NavigationPolicy FrameLoader::CheckLoadCanStart(
    FrameLoadRequest& frame_load_request,
    FrameLoadType type,
    NavigationPolicy navigation_policy,
    NavigationType navigation_type,
    bool check_with_client) {
  if (frame_->GetDocument()->PageDismissalEventBeingDispatched() !=
      Document::kNoDismissal) {
    return kNavigationPolicyIgnore;
  }

  // Record the latest requiredCSP value that will be used when sending this
  // request.
  ResourceRequest& resource_request = frame_load_request.GetResourceRequest();
  RecordLatestRequiredCSP();
  // Before modifying the request, check report-only CSP headers to give the
  // site owner a chance to learn about requests that need to be modified.
  MaybeCheckCSP(
      resource_request, navigation_type, frame_, navigation_policy,
      frame_load_request.ShouldCheckMainWorldContentSecurityPolicy() ==
          kCheckContentSecurityPolicy,
      ContentSecurityPolicy::CheckHeaderType::kCheckReportOnly);
  ModifyRequestForCSP(resource_request, frame_load_request.OriginDocument());

  WebTriggeringEventInfo triggering_event_info =
      WebTriggeringEventInfo::kNotFromEvent;
  if (frame_load_request.TriggeringEvent()) {
    triggering_event_info = frame_load_request.TriggeringEvent()->isTrusted()
                                ? WebTriggeringEventInfo::kFromTrustedEvent
                                : WebTriggeringEventInfo::kFromUntrustedEvent;
  }
  return ShouldContinueForNavigationPolicy(
      resource_request, frame_load_request.OriginDocument(),
      frame_load_request.GetSubstituteData(), nullptr,
      frame_load_request.ShouldCheckMainWorldContentSecurityPolicy(),
      navigation_type, navigation_policy, type,
      frame_load_request.ClientRedirect() ==
          ClientRedirectPolicy::kClientRedirect,
      triggering_event_info, frame_load_request.Form(),
      frame_load_request.GetBlobURLToken(), check_with_client);
}

void FrameLoader::StartLoad(FrameLoadRequest& frame_load_request,
                            FrameLoadType type,
                            NavigationPolicy navigation_policy,
                            HistoryItem* history_item,
                            bool check_with_client) {
  DCHECK(Client()->HasWebView());
  ResourceRequest& resource_request = frame_load_request.GetResourceRequest();
  NavigationType navigation_type = DetermineNavigationType(
      type, resource_request.HttpBody() || frame_load_request.Form(),
      frame_load_request.TriggeringEvent());
  resource_request.SetRequestContext(
      DetermineRequestContextFromNavigationType(navigation_type));
  resource_request.SetFrameType(
      frame_->IsMainFrame() ? network::mojom::RequestContextFrameType::kTopLevel
                            : network::mojom::RequestContextFrameType::kNested);
  Document* origin_document = frame_load_request.OriginDocument();

  if (origin_document && origin_document->GetContentSecurityPolicy()
                             ->ExperimentalFeaturesEnabled()) {
    WebContentSecurityPolicyList initiator_csp =
        origin_document->GetContentSecurityPolicy()
            ->ExposeForNavigationalChecks();
    resource_request.SetInitiatorCSP(initiator_csp);
  }

  bool had_placeholder_client_document_loader =
      provisional_document_loader_ && !provisional_document_loader_->DidStart();
  navigation_policy =
      CheckLoadCanStart(frame_load_request, type, navigation_policy,
                        navigation_type, check_with_client);
  if (navigation_policy == kNavigationPolicyIgnore) {
    if (had_placeholder_client_document_loader &&
        !resource_request.CheckForBrowserSideNavigation()) {
      DetachDocumentLoader(provisional_document_loader_);
    }
    return;
  }

  // For PlzNavigate placeholder DocumentLoaders, don't send failure callbacks
  // for a placeholder simply being replaced with a new DocumentLoader.
  if (had_placeholder_client_document_loader)
    provisional_document_loader_->SetSentDidFinishLoad();
  frame_->GetDocument()->CancelParsing();

  // If we're starting a regular navigation on a regular document (i.e., there
  // was no placeholder DocumentLoader), it's not enough to cancel parsing, but
  // we also have to check whether the document was completed, so it's in a
  // defined state should the navigation fail.
  if (!had_placeholder_client_document_loader &&
      type == kFrameLoadTypeStandard &&
      (navigation_policy == kNavigationPolicyCurrentTab ||
       navigation_policy == kNavigationPolicyHandledByClient)) {
    frame_->GetDocument()->CheckCompleted();
  }
  DetachDocumentLoader(provisional_document_loader_);

  // beforeunload fired above, and detaching a DocumentLoader can fire events,
  // which can detach this frame.
  if (!frame_->GetPage())
    return;

  progress_tracker_->ProgressStarted(type);
  // TODO(japhet): This case wants to flag the frame as loading and do nothing
  // else. It'd be nice if it could go through the placeholder DocumentLoader
  // path, too.
  if (navigation_policy == kNavigationPolicyHandledByClientForInitialHistory)
    return;
  DCHECK(navigation_policy == kNavigationPolicyCurrentTab ||
         navigation_policy == kNavigationPolicyHandledByClient);

  provisional_document_loader_ = CreateDocumentLoader(
      resource_request, frame_load_request, type, navigation_type);

  // PlzNavigate: We need to ensure that script initiated navigations are
  // honored.
  if (!had_placeholder_client_document_loader ||
      navigation_policy == kNavigationPolicyHandledByClient) {
    frame_->GetNavigationScheduler().Cancel();
  }

  if (frame_load_request.Form())
    Client()->DispatchWillSubmitForm(frame_load_request.Form());

  provisional_document_loader_->AppendRedirect(
      provisional_document_loader_->Url());

  if (IsBackForwardLoadType(type)) {
    DCHECK(history_item);
    provisional_document_loader_->SetItemForHistoryNavigation(history_item);
  }

  DCHECK(!frame_load_request.GetResourceRequest().IsSameDocumentNavigation());
  frame_->GetFrameScheduler()->DidStartProvisionalLoad(frame_->IsMainFrame());

  // TODO(ananta):
  // We should get rid of the dependency on the DocumentLoader in consumers of
  // the DidStartProvisionalLoad() notification.
  Client()->DispatchDidStartProvisionalLoad(provisional_document_loader_,
                                            resource_request);
  DCHECK(provisional_document_loader_);

  if (navigation_policy == kNavigationPolicyCurrentTab) {
    provisional_document_loader_->StartLoading();
    // This should happen after the request is sent, so that the state
    // the inspector stored in the matching frameScheduledClientNavigation()
    // is available while sending the request.
    probe::frameClearedScheduledClientNavigation(frame_);
  } else {
    probe::frameScheduledClientNavigation(frame_);
  }

  TakeObjectSnapshot();
}

bool FrameLoader::ShouldTreatURLAsSameAsCurrent(const KURL& url) const {
  return document_loader_->GetHistoryItem() &&
         url == document_loader_->GetHistoryItem()->Url();
}

bool FrameLoader::ShouldTreatURLAsSrcdocDocument(const KURL& url) const {
  if (!url.IsAboutSrcdocURL())
    return false;
  HTMLFrameOwnerElement* owner_element = frame_->DeprecatedLocalOwner();
  if (!IsHTMLIFrameElement(owner_element))
    return false;
  return owner_element->FastHasAttribute(srcdocAttr);
}

void FrameLoader::DispatchDocumentElementAvailable() {
  ScriptForbiddenScope forbid_scripts;
  Client()->DocumentElementAvailable();
}

void FrameLoader::RunScriptsAtDocumentElementAvailable() {
  Client()->RunScriptsAtDocumentElementAvailable();
  // The frame might be detached at this point.
}

void FrameLoader::DispatchDidClearDocumentOfWindowObject() {
  DCHECK(frame_->GetDocument());
  if (state_machine_.CreatingInitialEmptyDocument())
    return;
  if (!frame_->GetDocument()->CanExecuteScripts(kNotAboutToExecuteScript))
    return;

  Settings* settings = frame_->GetSettings();
  if (settings && settings->GetForceMainWorldInitialization()) {
    // Forcibly instantiate WindowProxy.
    frame_->GetScriptController().WindowProxy(DOMWrapperWorld::MainWorld());
  }
  probe::didClearDocumentOfWindowObject(frame_);

  if (dispatching_did_clear_window_object_in_main_world_)
    return;
  base::AutoReset<bool> in_did_clear_window_object(
      &dispatching_did_clear_window_object_in_main_world_, true);
  // We just cleared the document, not the entire window object, but for the
  // embedder that's close enough.
  Client()->DispatchDidClearWindowObjectInMainWorld();
}

void FrameLoader::DispatchDidClearWindowObjectInMainWorld() {
  DCHECK(frame_->GetDocument());
  if (!frame_->GetDocument()->CanExecuteScripts(kNotAboutToExecuteScript))
    return;

  if (dispatching_did_clear_window_object_in_main_world_)
    return;
  base::AutoReset<bool> in_did_clear_window_object(
      &dispatching_did_clear_window_object_in_main_world_, true);
  Client()->DispatchDidClearWindowObjectInMainWorld();
}

SandboxFlags FrameLoader::EffectiveSandboxFlags() const {
  SandboxFlags flags = forced_sandbox_flags_;
  if (FrameOwner* frame_owner = frame_->Owner())
    flags |= frame_owner->GetSandboxFlags();
  // Frames need to inherit the sandbox flags of their parent frame.
  if (Frame* parent_frame = frame_->Tree().Parent())
    flags |= parent_frame->GetSecurityContext()->GetSandboxFlags();
  return flags;
}

WebInsecureRequestPolicy FrameLoader::GetInsecureRequestPolicy() const {
  Frame* parent_frame = frame_->Tree().Parent();
  if (!parent_frame)
    return kLeaveInsecureRequestsAlone;

  return parent_frame->GetSecurityContext()->GetInsecureRequestPolicy();
}

SecurityContext::InsecureNavigationsSet*
FrameLoader::InsecureNavigationsToUpgrade() const {
  DCHECK(frame_);
  Frame* parent_frame = frame_->Tree().Parent();
  if (!parent_frame)
    return nullptr;

  return parent_frame->GetSecurityContext()->InsecureNavigationsToUpgrade();
}

void FrameLoader::ModifyRequestForCSP(ResourceRequest& resource_request,
                                      Document* origin_document) const {
  if (RuntimeEnabledFeatures::EmbedderCSPEnforcementEnabled() &&
      !RequiredCSP().IsEmpty()) {
    DCHECK(
        ContentSecurityPolicy::IsValidCSPAttr(RequiredCSP().GetString(), ""));
    resource_request.SetHTTPHeaderField(HTTPNames::Sec_Required_CSP,
                                        RequiredCSP());
  }

  // Tack an 'Upgrade-Insecure-Requests' header to outgoing navigational
  // requests, as described in
  // https://w3c.github.io/webappsec-upgrade-insecure-requests/#feature-detect
  if (resource_request.GetFrameType() !=
      network::mojom::RequestContextFrameType::kNone) {
    // Early return if the request has already been upgraded.
    if (!resource_request.HttpHeaderField(HTTPNames::Upgrade_Insecure_Requests)
             .IsNull()) {
      return;
    }

    resource_request.SetHTTPHeaderField(HTTPNames::Upgrade_Insecure_Requests,
                                        "1");
  }

  UpgradeInsecureRequest(resource_request, origin_document);
}

// static
void FrameLoader::UpgradeInsecureRequest(ResourceRequest& resource_request,
                                         Document* origin_document) {
  // We always upgrade requests that meet any of the following criteria:
  //  1. Are for subresources.
  //  2. Are for nested frames.
  //  3. Are form submissions.
  //  4. Whose hosts are contained in the origin_document's upgrade insecure
  //     navigations set.

  // This happens for:
  // * Browser initiated main document loading. No upgrade required.
  // * Navigation initiated by a frame in another process. URL should have
  //   already been upgraded in the initiator's process.
  if (!origin_document)
    return;

  if (!(origin_document->GetInsecureRequestPolicy() & kUpgradeInsecureRequests))
    return;

  // Nested frames are always upgraded on the browser process.
  if (resource_request.GetFrameType() ==
      network::mojom::RequestContextFrameType::kNested) {
    return;
  }

  KURL url = resource_request.Url();
  if (!url.ProtocolIs("http"))
    return;

  if (resource_request.GetFrameType() ==
          network::mojom::RequestContextFrameType::kNone ||
      resource_request.GetRequestContext() ==
          WebURLRequest::kRequestContextForm ||
      (!url.Host().IsNull() &&
       origin_document->InsecureNavigationsToUpgrade()->Contains(
           url.Host().Impl()->GetHash()))) {
    UseCounter::Count(origin_document,
                      WebFeature::kUpgradeInsecureRequestsUpgradedRequest);
    url.SetProtocol("https");
    if (url.Port() == 80)
      url.SetPort(443);
    resource_request.SetURL(url);
  }
}

void FrameLoader::RecordLatestRequiredCSP() {
  required_csp_ =
      frame_->Owner() ? frame_->Owner()->RequiredCsp() : g_null_atom;
}

std::unique_ptr<TracedValue> FrameLoader::ToTracedValue() const {
  std::unique_ptr<TracedValue> traced_value = TracedValue::Create();
  traced_value->BeginDictionary("frame");
  traced_value->SetString("id_ref", IdentifiersFactory::FrameId(frame_.Get()));
  traced_value->EndDictionary();
  traced_value->SetBoolean("isLoadingMainFrame", IsLoadingMainFrame());
  traced_value->SetString("stateMachine", state_machine_.ToString());
  traced_value->SetString("provisionalDocumentLoaderURL",
                          provisional_document_loader_
                              ? provisional_document_loader_->Url().GetString()
                              : String());
  traced_value->SetString(
      "documentLoaderURL",
      document_loader_ ? document_loader_->Url().GetString() : String());
  return traced_value;
}

inline void FrameLoader::TakeObjectSnapshot() const {
  if (detached_) {
    // We already logged TRACE_EVENT_OBJECT_DELETED_WITH_ID in detach().
    return;
  }
  TRACE_EVENT_OBJECT_SNAPSHOT_WITH_ID("loading", "FrameLoader", this,
                                      ToTracedValue());
}

DocumentLoader* FrameLoader::CreateDocumentLoader(
    const ResourceRequest& request,
    const FrameLoadRequest& frame_load_request,
    FrameLoadType load_type,
    NavigationType navigation_type) {
  DocumentLoader* loader = Client()->CreateDocumentLoader(
      frame_, request,
      frame_load_request.GetSubstituteData().IsValid()
          ? frame_load_request.GetSubstituteData()
          : DefaultSubstituteDataForURL(request.Url()),
      frame_load_request.ClientRedirect(),
      frame_load_request.GetDevToolsNavigationToken());

  loader->SetLoadType(load_type);
  loader->SetNavigationType(navigation_type);
  // TODO(japhet): This is needed because the browser process DCHECKs if the
  // first entry we commit in a new frame has replacement set. It's unclear
  // whether the DCHECK is right, investigate removing this special case.
  bool replace_current_item = load_type == kFrameLoadTypeReplaceCurrentItem &&
                              (!Opener() || !request.Url().IsEmpty());
  loader->SetReplacesCurrentHistoryItem(replace_current_item);

  probe::lifecycleEvent(frame_, loader, "init", CurrentTimeTicksInSeconds());
  return loader;
}

STATIC_ASSERT_ENUM(kWebHistorySameDocumentLoad, kHistorySameDocumentLoad);
STATIC_ASSERT_ENUM(kWebHistoryDifferentDocumentLoad,
                   kHistoryDifferentDocumentLoad);

STATIC_ASSERT_ENUM(kWebHistoryScrollRestorationManual,
                   kScrollRestorationManual);
STATIC_ASSERT_ENUM(kWebHistoryScrollRestorationAuto, kScrollRestorationAuto);

STATIC_ASSERT_ENUM(WebFrameLoadType::kStandard, kFrameLoadTypeStandard);
STATIC_ASSERT_ENUM(WebFrameLoadType::kBackForward, kFrameLoadTypeBackForward);
STATIC_ASSERT_ENUM(WebFrameLoadType::kReload, kFrameLoadTypeReload);
STATIC_ASSERT_ENUM(WebFrameLoadType::kReplaceCurrentItem,
                   kFrameLoadTypeReplaceCurrentItem);
STATIC_ASSERT_ENUM(WebFrameLoadType::kInitialInChildFrame,
                   kFrameLoadTypeInitialInChildFrame);
STATIC_ASSERT_ENUM(WebFrameLoadType::kInitialHistoryLoad,
                   kFrameLoadTypeInitialHistoryLoad);
STATIC_ASSERT_ENUM(WebFrameLoadType::kReloadBypassingCache,
                   kFrameLoadTypeReloadBypassingCache);

}  // namespace blink
