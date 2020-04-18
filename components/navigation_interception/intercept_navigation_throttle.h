// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NAVIGATION_INTERCEPTION_INTERCEPT_NAVIGATION_THROTTLE_H_
#define COMPONENTS_NAVIGATION_INTERCEPTION_INTERCEPT_NAVIGATION_THROTTLE_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
class WebContents;
}

namespace navigation_interception {

class NavigationParams;

// This class allows the provider of the Callback to selectively ignore top
// level navigations. This is a UI thread class.
class InterceptNavigationThrottle : public content::NavigationThrottle {
 public:
  typedef base::Callback<bool(content::WebContents* /* source */,
                              const NavigationParams& /* navigation_params */)>
      CheckCallback;

  InterceptNavigationThrottle(content::NavigationHandle* navigation_handle,
                              CheckCallback should_ignore_callback);
  ~InterceptNavigationThrottle() override;

  // content::NavigationThrottle implementation:
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult CheckIfShouldIgnoreNavigation(bool is_redirect);

  CheckCallback should_ignore_callback_;

  DISALLOW_COPY_AND_ASSIGN(InterceptNavigationThrottle);
};

}  // namespace navigation_interception

#endif  // COMPONENTS_NAVIGATION_INTERCEPTION_INTERCEPT_NAVIGATION_THROTTLE_H_
