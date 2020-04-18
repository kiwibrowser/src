// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DATA_REDUCTION_PROXY_UTIL_H_
#define CHROME_BROWSER_DATA_REDUCTION_PROXY_UTIL_H_

class GURL;

namespace content {
class ResourceContext;
}

bool IsDataReductionProxyResourceThrottleEnabledForUrl(
    content::ResourceContext* resource_context,
    const GURL& url);

#endif  // CHROME_BROWSER_DATA_REDUCTION_PROXY_UTIL_H_
