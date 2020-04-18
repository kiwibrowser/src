// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_ORIGIN_UTIL_H_
#define CONTENT_PUBLIC_COMMON_ORIGIN_UTIL_H_

#include "content/common/content_export.h"
#include "url/origin.h"

class GURL;

namespace content {

// Returns true if the origin is trustworthy: that is, if its contents can be
// said to have been transferred to the browser in a way that a network attacker
// cannot tamper with or observe.
//
// See https://www.w3.org/TR/powerful-features/#is-origin-trustworthy.
bool CONTENT_EXPORT IsOriginSecure(const GURL& url);

// Returns true if the origin can register a service worker.  Scheme must be
// http (localhost only), https, or a custom-set secure scheme.
bool CONTENT_EXPORT OriginCanAccessServiceWorkers(const GURL& url);

// This is based on SecurityOrigin::isPotentiallyTrustworthy and tries to mimic
// its behavior.
bool CONTENT_EXPORT IsPotentiallyTrustworthyOrigin(const url::Origin& origin);

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_ORIGIN_UTIL_H_
