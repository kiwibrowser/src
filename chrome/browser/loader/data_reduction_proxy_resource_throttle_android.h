// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_LOADER_DATA_REDUCTION_PROXY_RESOURCE_THROTTLE_ANDROID_H_
#define CHROME_BROWSER_LOADER_DATA_REDUCTION_PROXY_RESOURCE_THROTTLE_ANDROID_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/safe_browsing/ui_manager.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"

namespace content {
class ResourceContext;
}  // namespace content

namespace net {
struct RedirectInfo;
class URLRequest;
}  // namepsace net

// DataReductionProxyResourceThrottle checks that URLs are "safe" before
// navigating to them. To be considered "safe", a URL must not be tagged as a
// phishing or malware URL by the SPDY proxy.
//
// If the URL is tagged as unsafe, a warning page is shown and the request
// remains suspended. If the user decides to cancel, the request is cancelled.
// If on the other hand the user decides the URL is safe, the request is
// resumed.
//
// The SPDY proxy and the browser make use of extra headers to communicate.
// When a proxy detects a malware or a phishing page, it injects a special
// header and returns a 307 redirect to the same location. If the user decides
// to proceed, the client injects a special header.
//
// Header              Sent From     Description
// X-Phishing-Url        Proxy      Identified as phishing url.
// X-Malware-Url         Proxy      Identified as malware url
// X-Unsafe-Url-Proceed  Browser    User requests proceed.
//
class DataReductionProxyResourceThrottle
    : public content::ResourceThrottle,
      public base::SupportsWeakPtr<DataReductionProxyResourceThrottle> {
 public:
  // Create a DataReductionProxyResourceThrottle.  If it's not enabled
  // or we can't process this request, will return NULL.
  static DataReductionProxyResourceThrottle* MaybeCreate(
      net::URLRequest* request,
      content::ResourceContext* resource_context,
      content::ResourceType resource_type,
      safe_browsing::SafeBrowsingService* sb_service);

  DataReductionProxyResourceThrottle(net::URLRequest* request,
                            content::ResourceType resource_type,
                            safe_browsing::SafeBrowsingService* safe_browsing);

  // content::ResourceThrottle implementation (called on IO thread).
  void WillRedirectRequest(const net::RedirectInfo& redirect_info,
                           bool* defer) override;
  const char* GetNameForLogging() const override;

 private:
  // Describes what phase of the check a throttle is in.
  enum State {
    STATE_NONE,
    STATE_DISPLAYING_BLOCKING_PAGE,
  };

  ~DataReductionProxyResourceThrottle() override;

  // SafeBrowsingService::UrlCheckCallback implementation.
  void OnBlockingPageComplete(bool proceed);

  // Returns the threat type.
  safe_browsing::SBThreatType CheckUrl();

  // Starts displaying the safe browsing interstitial page if it is not
  // prerendering. Called on the UI thread.
  static void StartDisplayingBlockingPage(
      const base::WeakPtr<DataReductionProxyResourceThrottle>& throttle,
      scoped_refptr<safe_browsing::SafeBrowsingUIManager> ui_manager,
      const security_interstitials::UnsafeResource& resource);

  // Resumes the request, by continuing the deferred action (either starting the
  // request, or following a redirect).
  void ResumeRequest();

  State state_;

  // The redirect chain for this resource.
  std::vector<GURL> redirect_urls_;

  scoped_refptr<safe_browsing::SafeBrowsingService> safe_browsing_;
  net::URLRequest* request_;
  const bool is_subresource_;
  const bool is_subframe_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyResourceThrottle);
};

#endif  // CHROME_BROWSER_LOADER_DATA_REDUCTION_PROXY_RESOURCE_THROTTLE_ANDROID_H_
