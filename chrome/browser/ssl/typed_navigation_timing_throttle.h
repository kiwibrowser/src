// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_TYPED_NAVIGATION_TIMING_THROTTLE_H_
#define CHROME_BROWSER_SSL_TYPED_NAVIGATION_TIMING_THROTTLE_H_

#include <memory>

#include "base/macros.h"
#include "content/public/browser/navigation_throttle.h"
#include "url/gurl.h"

namespace content {
class NavigationHandle;
}

// A TypedNavigationTimingThrottle tracks the timings of a navigation caused by
// a typed URL navigation for an HTTP URL which was then redirected to HTTPS.
class TypedNavigationTimingThrottle : public content::NavigationThrottle {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* handle);

  ~TypedNavigationTimingThrottle() override;

  // content::NavigationThrottle:
  content::NavigationThrottle::ThrottleCheckResult WillStartRequest() override;
  content::NavigationThrottle::ThrottleCheckResult WillRedirectRequest()
      override;
  const char* GetNameForLogging() override;

 private:
  explicit TypedNavigationTimingThrottle(content::NavigationHandle* handle);

  GURL initial_url_;
  bool recorded_ = false;

  DISALLOW_COPY_AND_ASSIGN(TypedNavigationTimingThrottle);
};

#endif  // CHROME_BROWSER_SSL_TYPED_NAVIGATION_TIMING_THROTTLE_H_
