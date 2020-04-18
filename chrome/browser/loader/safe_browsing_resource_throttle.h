// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOADER_SAFE_BROWSING_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_LOADER_SAFE_BROWSING_RESOURCE_THROTTLE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/safe_browsing/browser/base_parallel_resource_throttle.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"

namespace net {
class URLRequest;
}

namespace safe_browsing {
class SafeBrowsingService;
}

class ProfileIOData;

// Contructs a resource throttle for SafeBrowsing.
// It could return nullptr if URL checking is not supported on this
// build+device.
content::ResourceThrottle* MaybeCreateSafeBrowsingResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    safe_browsing::SafeBrowsingService* sb_service,
    const ProfileIOData* io_data);

// SafeBrowsingParallelResourceThrottle uses a Chrome-specific
// safe_browsing::UrlCheckerDelegate implementation with its base class
// safe_browsing::BaseParallelResourceThrottle.
class SafeBrowsingParallelResourceThrottle
    : public safe_browsing::BaseParallelResourceThrottle {
 private:
  friend content::ResourceThrottle* MaybeCreateSafeBrowsingResourceThrottle(
      net::URLRequest* request,
      content::ResourceType resource_type,
      safe_browsing::SafeBrowsingService* sb_service,
      const ProfileIOData* io_data);

  SafeBrowsingParallelResourceThrottle(
      const net::URLRequest* request,
      content::ResourceType resource_type,
      safe_browsing::SafeBrowsingService* sb_service);

  ~SafeBrowsingParallelResourceThrottle() override;

  const char* GetNameForLogging() const override;

  DISALLOW_COPY_AND_ASSIGN(SafeBrowsingParallelResourceThrottle);
};

#endif  // CHROME_BROWSER_LOADER_SAFE_BROWSING_RESOURCE_THROTTLE_H_
