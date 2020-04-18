// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/typed_navigation_timing_throttle.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/navigation_throttle.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "ui/base/page_transition_types.h"
#include "url/url_constants.h"

// static
std::unique_ptr<content::NavigationThrottle>
TypedNavigationTimingThrottle::MaybeCreateThrottleFor(
    content::NavigationHandle* handle) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!handle->GetURL().SchemeIs(url::kHttpScheme))
    return nullptr;

  return base::WrapUnique(new TypedNavigationTimingThrottle(handle));
}

TypedNavigationTimingThrottle::~TypedNavigationTimingThrottle() {}

content::NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillStartRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  url::Replacements<char> remove_port;
  remove_port.ClearPort();
  initial_url_ = navigation_handle()->GetURL().ReplaceComponents(remove_port);
  return content::NavigationThrottle::PROCEED;
}

content::NavigationThrottle::ThrottleCheckResult
TypedNavigationTimingThrottle::WillRedirectRequest() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (recorded_ ||
      !ui::PageTransitionCoreTypeIs(navigation_handle()->GetPageTransition(),
                                    ui::PAGE_TRANSITION_TYPED)) {
    return content::NavigationThrottle::PROCEED;
  }

  // Check if the URL is the same as the original but upgraded to HTTPS.
  // We ignore the port numbers (in case of non-standard HTTP or HTTPS ports)
  // and allow adding or dropping of a "www." prefix.
  url::Replacements<char> remove_port;
  remove_port.ClearPort();
  const GURL& redirect_url =
      navigation_handle()->GetURL().ReplaceComponents(remove_port);
  if (redirect_url.SchemeIs(url::kHttpsScheme) &&
      (redirect_url.GetContent() == initial_url_.GetContent() ||
       redirect_url.GetContent() == "www." + initial_url_.GetContent() ||
       "www." + redirect_url.GetContent() == initial_url_.GetContent())) {
    UMA_HISTOGRAM_TIMES(
        "Omnibox.URLNavigationTimeToRedirectToHTTPS",
        base::TimeTicks::Now() - navigation_handle()->NavigationStart());
    recorded_ = true;
  }

  return content::NavigationThrottle::PROCEED;
}

const char* TypedNavigationTimingThrottle::GetNameForLogging() {
  return "TypedNavigationTimingThrottle";
}

TypedNavigationTimingThrottle::TypedNavigationTimingThrottle(
    content::NavigationHandle* handle)
    : content::NavigationThrottle(handle) {}
