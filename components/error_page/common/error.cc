// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/error_page/common/error.h"

#include "net/base/net_errors.h"

namespace error_page {

const char Error::kNetErrorDomain[] = "net";
const char Error::kHttpErrorDomain[] = "http";
const char Error::kDnsProbeErrorDomain[] = "dnsprobe";

Error Error::NetError(const GURL& url, int reason, bool stale_copy_in_cache) {
  return Error(url, kNetErrorDomain, reason, stale_copy_in_cache);
}

Error Error::HttpError(const GURL& url, int http_status_code) {
  return Error(url, kHttpErrorDomain, http_status_code, false);
}

Error Error::DnsProbeError(const GURL& url,
                           int status,
                           bool stale_copy_in_cache) {
  return Error(url, kDnsProbeErrorDomain, status, stale_copy_in_cache);
}

Error::Error(const GURL& url,
             const std::string& domain,
             int reason,
             bool stale_copy_in_cache)
    : url_(url),
      domain_(domain),
      reason_(reason),
      stale_copy_in_cache_(stale_copy_in_cache) {}

}  // namespace error_page
