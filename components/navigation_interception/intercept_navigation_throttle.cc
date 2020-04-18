// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/navigation_interception/intercept_navigation_throttle.h"

#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "components/navigation_interception/navigation_params.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"

using content::BrowserThread;

namespace navigation_interception {

InterceptNavigationThrottle::InterceptNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    CheckCallback should_ignore_callback)
    : content::NavigationThrottle(navigation_handle),
      should_ignore_callback_(should_ignore_callback) {}

InterceptNavigationThrottle::~InterceptNavigationThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  base::ElapsedTimer timer;

  auto result = CheckIfShouldIgnoreNavigation(false);
  UMA_HISTOGRAM_COUNTS_10M("Navigation.Intercept.WillStart",
                           timer.Elapsed().InMicroseconds());
  return result;
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::WillRedirectRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return CheckIfShouldIgnoreNavigation(true);
}

const char* InterceptNavigationThrottle::GetNameForLogging() {
  return "InterceptNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
InterceptNavigationThrottle::CheckIfShouldIgnoreNavigation(bool is_redirect) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  NavigationParams navigation_params(
      navigation_handle()->GetURL(), navigation_handle()->GetReferrer(),
      navigation_handle()->HasUserGesture(), navigation_handle()->IsPost(),
      navigation_handle()->GetPageTransition(), is_redirect,
      navigation_handle()->IsExternalProtocol(), true,
      navigation_handle()->GetBaseURLForDataURL());
  bool should_ignore_navigation = should_ignore_callback_.Run(
      navigation_handle()->GetWebContents(), navigation_params);
  return should_ignore_navigation
             ? content::NavigationThrottle::CANCEL_AND_IGNORE
             : content::NavigationThrottle::PROCEED;
}

}  // namespace navigation_interception
