// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_BACKGROUND_TAB_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_BACKGROUND_TAB_NAVIGATION_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"

namespace resource_coordinator {

// BackgroundTabNavigationThrottle plumbs navigation information to TabManager
// and enables TabManager to delay navigation for background tabs. This is only
// used on non-Android platforms, same as TabManager.
class BackgroundTabNavigationThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<BackgroundTabNavigationThrottle>
  MaybeCreateThrottleFor(content::NavigationHandle* navigation_handle);

  explicit BackgroundTabNavigationThrottle(
      content::NavigationHandle* navigation_handle);
  ~BackgroundTabNavigationThrottle() override;

  // content::NavigationThrottle implementation
  const char* GetNameForLogging() override;
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;

  // Virtual to allow unit tests to make this a no-op.
  virtual void ResumeNavigation();

 private:
  DISALLOW_COPY_AND_ASSIGN(BackgroundTabNavigationThrottle);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_BACKGROUND_TAB_NAVIGATION_THROTTLE_H_
