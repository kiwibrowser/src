// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/mixed_content_navigation_throttle.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/browser/frame_host/render_frame_host_delegate.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_client.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/web_preferences.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace content {

namespace {

// Should return the same value as SchemeRegistry::shouldTreatURLSchemeAsSecure.
bool IsSecureScheme(const std::string& scheme) {
  return base::ContainsValue(url::GetSecureSchemes(), scheme);
}

// Should return the same value as SecurityOrigin::isLocal and
// SchemeRegistry::shouldTreatURLSchemeAsCORSEnabled.
bool ShouldTreatURLSchemeAsCORSEnabled(const GURL& url) {
  return base::ContainsValue(url::GetCORSEnabledSchemes(), url.scheme());
}

// Should return the same value as the resource URL checks assigned to
// |isAllowed| made inside MixedContentChecker::isMixedContent.
bool IsUrlPotentiallySecure(const GURL& url) {
  // blob: and filesystem: URLs never hit the network, and access is restricted
  // to same-origin contexts, so they are not blocked.
  bool is_secure = url.SchemeIs(url::kBlobScheme) ||
                   url.SchemeIs(url::kFileSystemScheme) ||
                   IsOriginSecure(url) ||
                   IsPotentiallyTrustworthyOrigin(url::Origin::Create(url));

  // TODO(mkwst): Remove this once the following draft is implemented:
  // https://tools.ietf.org/html/draft-west-let-localhost-be-localhost-03. See:
  // https://crbug.com/691930.
  if (is_secure && url.SchemeIs(url::kHttpScheme) &&
      net::IsLocalHostname(url.HostNoBracketsPiece(), nullptr)) {
    is_secure = false;
  }

  return is_secure;
}

// This method should return the same results as
// SchemeRegistry::shouldTreatURLSchemeAsRestrictingMixedContent.
bool DoesOriginSchemeRestrictMixedContent(const url::Origin& origin) {
  return origin.scheme() == url::kHttpsScheme;
}

void UpdateRendererOnMixedContentFound(NavigationHandleImpl* navigation_handle,
                                       const GURL& mixed_content_url,
                                       bool was_allowed,
                                       bool for_redirect) {
  // TODO(carlosk): the root node should never be considered as being/having
  // mixed content for now. Once/if the browser should also check form submits
  // for mixed content than this will be allowed to happen and this DCHECK
  // should be updated.
  DCHECK(navigation_handle->frame_tree_node()->parent());
  RenderFrameHost* rfh =
      navigation_handle->frame_tree_node()->current_frame_host();
  FrameMsg_MixedContentFound_Params params;
  params.main_resource_url = mixed_content_url;
  params.mixed_content_url = navigation_handle->GetURL();
  params.request_context_type = navigation_handle->request_context_type();
  params.was_allowed = was_allowed;
  params.had_redirect = for_redirect;
  params.source_location = navigation_handle->source_location();

  rfh->Send(new FrameMsg_MixedContentFound(rfh->GetRoutingID(), params));
}

}  // namespace

// static
std::unique_ptr<NavigationThrottle>
MixedContentNavigationThrottle::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  if (IsBrowserSideNavigationEnabled())
    return base::WrapUnique(
        new MixedContentNavigationThrottle(navigation_handle));
  return nullptr;
}

MixedContentNavigationThrottle::MixedContentNavigationThrottle(
    NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {
  DCHECK(IsBrowserSideNavigationEnabled());
}

MixedContentNavigationThrottle::~MixedContentNavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult
MixedContentNavigationThrottle::WillStartRequest() {
  bool should_block = ShouldBlockNavigation(false);
  return should_block ? CANCEL : PROCEED;
}

NavigationThrottle::ThrottleCheckResult
MixedContentNavigationThrottle::WillRedirectRequest() {
  // Upon redirects the same checks are to be executed as for requests.
  bool should_block = ShouldBlockNavigation(true);
  return should_block ? CANCEL : PROCEED;
}

NavigationThrottle::ThrottleCheckResult
MixedContentNavigationThrottle::WillProcessResponse() {
  // TODO(carlosk): At this point we are about to process the request response.
  // So if we ever need to, here/now it is a good moment to check for the final
  // attained security level of the connection. For instance, does it use an
  // outdated protocol? The implementation should be based off
  // MixedContentChecker::handleCertificateError. See https://crbug.com/576270.
  return PROCEED;
}

const char* MixedContentNavigationThrottle::GetNameForLogging() {
  return "MixedContentNavigationThrottle";
}

// Based off of MixedContentChecker::shouldBlockFetch.
bool MixedContentNavigationThrottle::ShouldBlockNavigation(bool for_redirect) {
  NavigationHandleImpl* handle_impl =
      static_cast<NavigationHandleImpl*>(navigation_handle());
  FrameTreeNode* node = handle_impl->frame_tree_node();

  // Find the parent node where mixed content is characterized, if any.
  FrameTreeNode* mixed_content_node =
      InWhichFrameIsContentMixed(node, handle_impl->GetURL());
  if (!mixed_content_node) {
    MaybeSendBlinkFeatureUsageReport();
    return false;
  }

  // From this point on we know this is not a main frame navigation and that
  // there is mixed content. Now let's decide if it's OK to proceed with it.
  const WebPreferences& prefs = mixed_content_node->current_frame_host()
                                    ->render_view_host()
                                    ->GetWebkitPreferences();

  ReportBasicMixedContentFeatures(handle_impl->request_context_type(),
                                  handle_impl->mixed_content_context_type(),
                                  prefs);

  // If we're in strict mode, we'll automagically fail everything, and
  // intentionally skip the client/embedder checks in order to prevent degrading
  // the site's security UI.
  bool block_all_mixed_content = !!(
      mixed_content_node->current_replication_state().insecure_request_policy &
      blink::kBlockAllMixedContent);
  bool strict_mode =
      prefs.strict_mixed_content_checking || block_all_mixed_content;

  blink::WebMixedContentContextType mixed_context_type =
      handle_impl->mixed_content_context_type();

  if (!ShouldTreatURLSchemeAsCORSEnabled(handle_impl->GetURL()))
    mixed_context_type =
        blink::WebMixedContentContextType::kOptionallyBlockable;

  bool allowed = false;
  RenderFrameHostDelegate* frame_host_delegate =
      node->current_frame_host()->delegate();
  switch (mixed_context_type) {
    case blink::WebMixedContentContextType::kOptionallyBlockable:
      allowed = !strict_mode;
      if (allowed) {
        frame_host_delegate->PassiveInsecureContentFound(handle_impl->GetURL());
        frame_host_delegate->DidDisplayInsecureContent();
      }
      break;

    case blink::WebMixedContentContextType::kBlockable: {
      // Note: from the renderer side implementation it seems like we don't need
      // to care about reporting
      // blink::UseCounter::BlockableMixedContentInSubframeBlocked because it is
      // only triggered for sub-resources which are not checked for in the
      // browser.
      bool should_ask_delegate =
          !strict_mode && (!prefs.strictly_block_blockable_mixed_content ||
                           prefs.allow_running_insecure_content);
      allowed =
          should_ask_delegate &&
          frame_host_delegate->ShouldAllowRunningInsecureContent(
              handle_impl->GetWebContents(),
              prefs.allow_running_insecure_content,
              mixed_content_node->current_origin(), handle_impl->GetURL());
      if (allowed) {
        const GURL& origin_url = mixed_content_node->current_origin().GetURL();
        frame_host_delegate->DidRunInsecureContent(origin_url,
                                                   handle_impl->GetURL());
        GetContentClient()->browser()->RecordURLMetric(
            "ContentSettings.MixedScript.RanMixedScript", origin_url);
        mixed_content_features_.insert(MIXED_CONTENT_BLOCKABLE_ALLOWED);
      }
      break;
    }

    case blink::WebMixedContentContextType::kShouldBeBlockable:
      allowed = !strict_mode;
      if (allowed)
        frame_host_delegate->DidDisplayInsecureContent();
      break;

    case blink::WebMixedContentContextType::kNotMixedContent:
      NOTREACHED();
      break;
  };

  UpdateRendererOnMixedContentFound(
      handle_impl, mixed_content_node->current_url(), allowed, for_redirect);
  MaybeSendBlinkFeatureUsageReport();

  return !allowed;
}

// This method mirrors MixedContentChecker::inWhichFrameIsContentMixed but is
// implemented in a different form that seems more appropriate here.
FrameTreeNode* MixedContentNavigationThrottle::InWhichFrameIsContentMixed(
    FrameTreeNode* node,
    const GURL& url) {
  // Main frame navigations cannot be mixed content.
  // TODO(carlosk): except for form submissions which might be supported in the
  // future.
  if (node->IsMainFrame())
    return nullptr;

  // There's no mixed content if any of these are true:
  // - The navigated URL is potentially secure.
  // - Neither the root nor parent frames have secure origins.
  // This next section diverges in how the Blink version is implemented but
  // should get to the same results. Especially where isMixedContent calls
  // exist, here they are partially fulfilled here  and partially replaced by
  // DoesOriginSchemeRestrictMixedContent.
  FrameTreeNode* mixed_content_node = nullptr;
  FrameTreeNode* root = node->frame_tree()->root();
  FrameTreeNode* parent = node->parent();
  if (!IsUrlPotentiallySecure(url)) {
    // TODO(carlosk): we might need to check more than just the immediate parent
    // and the root. See https://crbug.com/623486.

    // Checks if the root and then the immediate parent frames' origins are
    // secure.
    if (DoesOriginSchemeRestrictMixedContent(root->current_origin()))
      mixed_content_node = root;
    else if (DoesOriginSchemeRestrictMixedContent(parent->current_origin()))
      mixed_content_node = parent;
  }

  // Note: The code below should behave the same way as the two calls to
  // measureStricterVersionOfIsMixedContent from inside
  // MixedContentChecker::inWhichFrameIs.
  if (mixed_content_node) {
    // We're currently only checking for mixed content in `https://*` contexts.
    // What about other "secure" contexts the SchemeRegistry knows about? We'll
    // use this method to measure the occurrence of non-webby mixed content to
    // make sure we're not breaking the world without realizing it.
    if (mixed_content_node->current_origin().scheme() != url::kHttpsScheme) {
      mixed_content_features_.insert(
          MIXED_CONTENT_IN_NON_HTTPS_FRAME_THAT_RESTRICTS_MIXED_CONTENT);
    }
  } else if (!IsOriginSecure(url) &&
             (IsSecureScheme(root->current_origin().scheme()) ||
              IsSecureScheme(parent->current_origin().scheme()))) {
    mixed_content_features_.insert(
        MIXED_CONTENT_IN_SECURE_FRAME_THAT_DOES_NOT_RESTRICT_MIXED_CONTENT);
  }
  return mixed_content_node;
}

void MixedContentNavigationThrottle::MaybeSendBlinkFeatureUsageReport() {
  if (!mixed_content_features_.empty()) {
    NavigationHandleImpl* handle_impl =
        static_cast<NavigationHandleImpl*>(navigation_handle());
    RenderFrameHost* rfh = handle_impl->frame_tree_node()->current_frame_host();
    rfh->Send(new FrameMsg_BlinkFeatureUsageReport(rfh->GetRoutingID(),
                                                   mixed_content_features_));
    mixed_content_features_.clear();
  }
}

// Based off of MixedContentChecker::count.
void MixedContentNavigationThrottle::ReportBasicMixedContentFeatures(
    RequestContextType request_context_type,
    blink::WebMixedContentContextType mixed_content_context_type,
    const WebPreferences& prefs) {
  mixed_content_features_.insert(MIXED_CONTENT_PRESENT);

  // Report any blockable content.
  if (mixed_content_context_type ==
      blink::WebMixedContentContextType::kBlockable) {
    mixed_content_features_.insert(MIXED_CONTENT_BLOCKABLE);
    return;
  }

  // Note: as there's no mixed content checks for sub-resources on the browser
  // side there should only be a subset of RequestContextType values that could
  // ever be found here.
  UseCounterFeature feature;
  switch (request_context_type) {
    case REQUEST_CONTEXT_TYPE_INTERNAL:
      feature = MIXED_CONTENT_INTERNAL;
      break;
    case REQUEST_CONTEXT_TYPE_PREFETCH:
      feature = MIXED_CONTENT_PREFETCH;
      break;

    case REQUEST_CONTEXT_TYPE_AUDIO:
    case REQUEST_CONTEXT_TYPE_DOWNLOAD:
    case REQUEST_CONTEXT_TYPE_FAVICON:
    case REQUEST_CONTEXT_TYPE_IMAGE:
    case REQUEST_CONTEXT_TYPE_PLUGIN:
    case REQUEST_CONTEXT_TYPE_VIDEO:
    default:
      NOTREACHED() << "RequestContextType has value " << request_context_type
                   << " and has WebMixedContentContextType of "
                   << static_cast<int>(mixed_content_context_type);
      return;
  }
  mixed_content_features_.insert(feature);
}

// static
bool MixedContentNavigationThrottle::IsMixedContentForTesting(
    const GURL& origin_url,
    const GURL& url) {
  const url::Origin origin = url::Origin::Create(origin_url);
  return !IsUrlPotentiallySecure(url) &&
         DoesOriginSchemeRestrictMixedContent(origin);
}

}  // namespace content
