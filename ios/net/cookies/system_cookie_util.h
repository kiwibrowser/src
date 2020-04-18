// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_NET_COOKIES_SYSTEM_COOKIE_UTIL_H_
#define IOS_NET_COOKIES_SYSTEM_COOKIE_UTIL_H_

#include <stddef.h>

#include "net/cookies/canonical_cookie.h"

@class NSHTTPCookie;

namespace base {
class Time;
}

namespace net {

// Converts NSHTTPCookie to net::CanonicalCookie.
net::CanonicalCookie CanonicalCookieFromSystemCookie(
    NSHTTPCookie* cookie,
    const base::Time& ceation_time);

// Converts net::CanonicalCookie to NSHTTPCookie.
NSHTTPCookie* SystemCookieFromCanonicalCookie(
    const net::CanonicalCookie& cookie);

enum CookieEvent {
  COOKIES_READ,                     // Cookies have been read from disk.
  COOKIES_APPLICATION_FOREGROUNDED  // The application has been foregrounded.
};

// Report metrics if the number of cookies drops unexpectedly.
void CheckForCookieLoss(size_t cookie_count, CookieEvent event);

// Reset the cookie count internally used by the CheckForCookieLoss() function.
void ResetCookieCountMetrics();

}  // namespace net

#endif  // IOS_NET_COOKIES_SYSTEM_COOKIE_UTIL_H_
