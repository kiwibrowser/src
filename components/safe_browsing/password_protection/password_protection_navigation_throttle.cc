// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/safe_browsing/password_protection/password_protection_navigation_throttle.h"

#include "components/safe_browsing/password_protection/password_protection_request.h"
#include "content/public/browser/navigation_handle.h"

namespace safe_browsing {
PasswordProtectionNavigationThrottle::PasswordProtectionNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    scoped_refptr<PasswordProtectionRequest> request,
    bool is_warning_showing)
    : content::NavigationThrottle(navigation_handle),
      request_(request),
      is_warning_showing_(is_warning_showing) {
  // Only call AddThrottle() if there is no modal warning showing. If there's a
  // modal dialog, PPNavigationThrottle will simply cancel this navigation
  // immediately, therefore no need to keep track of it.
  if (!is_warning_showing_)
    request_->AddThrottle(this);
}

PasswordProtectionNavigationThrottle::~PasswordProtectionNavigationThrottle() {
  if (request_)
    request_->RemoveThrottle(this);
}

content::NavigationThrottle::ThrottleCheckResult
PasswordProtectionNavigationThrottle::WillStartRequest() {
  if (is_warning_showing_)
    return content::NavigationThrottle::CANCEL;
  return content::NavigationThrottle::DEFER;
}

content::NavigationThrottle::ThrottleCheckResult
PasswordProtectionNavigationThrottle::WillRedirectRequest() {
  if (is_warning_showing_)
    return content::NavigationThrottle::CANCEL;
  return content::NavigationThrottle::DEFER;
}

const char* PasswordProtectionNavigationThrottle::GetNameForLogging() {
  return "PasswordProtectionNavigationThrottle";
}

void PasswordProtectionNavigationThrottle::ResumeNavigation() {
  Resume();
}

void PasswordProtectionNavigationThrottle::CancelNavigation(
    content::NavigationThrottle::ThrottleCheckResult result) {
  CancelDeferredNavigation(result);
}

}  // namespace safe_browsing
