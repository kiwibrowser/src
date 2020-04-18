// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_

#include <memory>

#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

namespace extensions {

// This class creates navigation throttles that bounce off navigations that
// are out-of-scope of the current PWA window into a new foreground tab.
//
// BookmarkAppExperimentalNavigationThrottle is a superset of this throttle;
// besides handling out-of-scope navigations in PWA windows, the experimental
// throttle also handles in-scope navigations in non PWA contexts i.e.
// implements link capturing.
// TODO(crbug.com/819475): Consolidate overlapping behavior in a base class.
class BookmarkAppNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* navigation_handle);

  explicit BookmarkAppNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~BookmarkAppNavigationThrottle() override;

  // content::NavigationThrottle:
  const char* GetNameForLogging() override;
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;

 private:
  // Opens a new foreground tab with the target URL if the navigation is out of
  // scope of the current PWA window.
  content::NavigationThrottle::ThrottleCheckResult
  OpenForegroundTabIfOutOfScope(bool is_redirect);

  DISALLOW_COPY_AND_ASSIGN(BookmarkAppNavigationThrottle);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_BOOKMARK_APP_NAVIGATION_THROTTLE_H_
