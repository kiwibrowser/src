// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/origin_security_checker.h"

#include "content/public/common/origin_util.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace payments {

// static
bool OriginSecurityChecker::IsOriginSecure(const GURL& url) {
  return url.is_valid() && content::IsOriginSecure(url);
}

// static
bool OriginSecurityChecker::IsSchemeCryptographic(const GURL& url) {
  return url.is_valid() && url.SchemeIsCryptographic();
}

// static
bool OriginSecurityChecker::IsOriginLocalhostOrFile(const GURL& url) {
  return url.is_valid() && (net::IsLocalhost(url) || url.SchemeIsFile());
}

}  // namespace payments
