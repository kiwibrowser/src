// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_RESOURCE_THROTTLE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_RESOURCE_THROTTLE_H_

#include "android_webview/browser/aw_safe_browsing_ui_manager.h"
#include "base/macros.h"
#include "components/safe_browsing/browser/base_parallel_resource_throttle.h"
#include "components/safe_browsing/db/database_manager.h"
#include "content/public/common/resource_type.h"

namespace net {
class URLRequest;
}

namespace android_webview {

class AwSafeBrowsingWhitelistManager;

// Contructs a resource throttle for SafeBrowsing.
// It returns nullptr if GMS doesn't exist on device or support SafeBrowsing.
content::ResourceThrottle* MaybeCreateAwSafeBrowsingResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
    AwSafeBrowsingWhitelistManager* whitelist_manager);

bool IsCancelledBySafeBrowsing(const net::URLRequest* request);

// AwSafeBrowsingPrallelResourceThrottle uses a WebView-specific
// safe_browsing::UrlCheckerDelegate implementation with its base class
// safe_browsing::BaseParallelResourceThrottle.
class AwSafeBrowsingParallelResourceThrottle
    : public safe_browsing::BaseParallelResourceThrottle {
 private:
  friend content::ResourceThrottle* MaybeCreateAwSafeBrowsingResourceThrottle(
      net::URLRequest* request,
      content::ResourceType resource_type,
      scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager>
          database_manager,
      scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
      AwSafeBrowsingWhitelistManager* whitelist_manager);

  AwSafeBrowsingParallelResourceThrottle(
      net::URLRequest* request,
      content::ResourceType resource_type,
      scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager>
          database_manager,
      scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
      AwSafeBrowsingWhitelistManager* whitelist_manager);

  ~AwSafeBrowsingParallelResourceThrottle() override;

  // safe_browsing::BaseParallelResourceThrottle overrides:
  void CancelResourceLoad() override;

  net::URLRequest* request_;

  DISALLOW_COPY_AND_ASSIGN(AwSafeBrowsingParallelResourceThrottle);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_RESOURCE_THROTTLE_H_
