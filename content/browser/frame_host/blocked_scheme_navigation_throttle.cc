// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/blocked_scheme_navigation_throttle.h"

#include "base/feature_list.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "content/browser/frame_host/frame_tree.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/console_message_level.h"
#include "content/public/common/content_features.h"
#include "url/url_constants.h"

namespace content {

namespace {
const char kConsoleError[] = "Not allowed to navigate top frame to %s URL: %s";
}

BlockedSchemeNavigationThrottle::BlockedSchemeNavigationThrottle(
    NavigationHandle* navigation_handle)
    : NavigationThrottle(navigation_handle) {}

BlockedSchemeNavigationThrottle::~BlockedSchemeNavigationThrottle() {}

NavigationThrottle::ThrottleCheckResult
BlockedSchemeNavigationThrottle::WillProcessResponse() {
  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(navigation_handle());
  if (handle->IsDownload())
    return PROCEED;

  RenderFrameHost* top_frame =
      handle->frame_tree_node()->frame_tree()->root()->current_frame_host();
  top_frame->AddMessageToConsole(
      CONSOLE_MESSAGE_LEVEL_ERROR,
      base::StringPrintf(kConsoleError, handle->GetURL().scheme().c_str(),
                         handle->GetURL().spec().c_str()));
  return CANCEL;
}

const char* BlockedSchemeNavigationThrottle::GetNameForLogging() {
  return "BlockedSchemeNavigationThrottle";
}

// static
std::unique_ptr<NavigationThrottle>
BlockedSchemeNavigationThrottle::CreateThrottleForNavigation(
    NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame() &&
      navigation_handle->IsRendererInitiated() &&
      !navigation_handle->IsSameDocument() &&
      (navigation_handle->GetURL().SchemeIs(url::kDataScheme) ||
       navigation_handle->GetURL().SchemeIs(url::kFileSystemScheme)) &&
      !base::FeatureList::IsEnabled(
          features::kAllowContentInitiatedDataUrlNavigations)) {
    return std::make_unique<BlockedSchemeNavigationThrottle>(navigation_handle);
  }
  return nullptr;
}

}  // namespace content
