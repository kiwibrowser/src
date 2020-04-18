// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ISOLATED_ORIGIN_UTIL_H_
#define CONTENT_BROWSER_ISOLATED_ORIGIN_UTIL_H_

#include "content/common/content_export.h"
#include "url/origin.h"

namespace content {

class CONTENT_EXPORT IsolatedOriginUtil {
 public:
  // Checks whether |origin| matches the isolated origin specified by
  // |isolated_origin|.  Subdomains are considered to match isolated origins,
  // so this will be true if
  // (1) |origin| has the same scheme, host, and port as |isolated_origin|, or
  // (2) |origin| has the same scheme and port as |isolated_origin|, and its
  //     host is a subdomain of |isolated_origin|'s host.
  // This does not consider site URLs, which don't care about port.
  //
  // For example, if |isolated_origin| is https://isolated.foo.com, this will
  // return true if |origin| is https://isolated.foo.com or
  // https://bar.isolated.foo.com, but it will return false for an |origin| of
  // https://unisolated.foo.com or https://foo.com.
  static bool DoesOriginMatchIsolatedOrigin(const url::Origin& origin,
                                            const url::Origin& isolated_origin);

  // Check if |origin| is a valid isolated origin.  Invalid isolated origins
  // include unique origins, origins that don't have an HTTP or HTTPS scheme,
  // and origins without a valid registry-controlled domain.  IP addresses are
  // allowed.
  static bool IsValidIsolatedOrigin(const url::Origin& origin);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ISOLATED_ORIGIN_UTIL_H_
