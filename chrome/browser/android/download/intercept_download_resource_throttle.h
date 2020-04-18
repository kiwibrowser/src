// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_DOWNLOAD_INTERCEPT_DOWNLOAD_RESOURCE_THROTTLE_H_
#define CHROME_BROWSER_ANDROID_DOWNLOAD_INTERCEPT_DOWNLOAD_RESOURCE_THROTTLE_H_

#include "base/memory/weak_ptr.h"
#include "chrome/browser/android/download/download_controller_base.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/resource_throttle.h"
#include "net/cookies/canonical_cookie.h"

namespace net {
class URLRequest;
}

// InterceptDownloadResourceThrottle checks if a download request should be
// handled by Chrome or passsed to the Android Download Manager.
class InterceptDownloadResourceThrottle : public content::ResourceThrottle {
 public:
  InterceptDownloadResourceThrottle(
      net::URLRequest* request,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter);
  ~InterceptDownloadResourceThrottle() override;

  // content::ResourceThrottle implementation:
  void WillProcessResponse(bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  void CheckCookiePolicy(const net::CookieList& cookie_list);
  void StartDownload(const DownloadInfo& info);

  const net::URLRequest* request_;
  content::ResourceRequestInfo::WebContentsGetter wc_getter_;

  base::WeakPtrFactory<InterceptDownloadResourceThrottle> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(InterceptDownloadResourceThrottle);
};

#endif  // CHROME_BROWSER_ANDROID_DOWNLOAD_INTERCEPT_DOWNLOAD_RESOURCE_THROTTLE_H_
