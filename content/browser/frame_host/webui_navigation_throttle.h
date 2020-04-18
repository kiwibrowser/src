// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_WEBUI_NAVIGATION_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_WEBUI_NAVIGATION_THROTTLE_H_

#include "content/public/browser/navigation_throttle.h"

namespace content {

// This NavigationThrottle class is used to check for subframe navigations to
// web content in WebUI processes and/or chrome:// documents. When the
// parent frame is at a chrome:// URL or is in a process with WebUI
// bindings, subframes are only allowed to navigate to chrome:// URLs.
// Note: There are WebUI documents that live on non-chrome: schemes and do
// not have WebUI bindings. Those are not covered by this restriction.
//
// This is an important security property to uphold, because by default
// WebUI documents have high privileges and if malicious web content is
// loaded in their process, it can be used as an easy step towards a sandbox
// escape.
//
// Note: Navigations in the main frame are allowed, as those will result in a
// process change with BrowsingInstance change and drop of privileges.
// Subframes are resticted because they must be in the same BrowsingInstance
// and would have the ability to communicate with the parent document.
class WebUINavigationThrottle : public NavigationThrottle {
 public:
  static std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);

  explicit WebUINavigationThrottle(NavigationHandle* navigation_handle);
  ~WebUINavigationThrottle() override;

  // NavigationThrottle methods
  ThrottleCheckResult WillStartRequest() override;
  const char* GetNameForLogging() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebUINavigationThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_WEBUI_NAVIGATION_THROTTLE_H_
