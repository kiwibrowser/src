// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_INTERCEPTOR_H_
#define CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_INTERCEPTOR_H_

#include "base/macros.h"
#include "net/url_request/url_request_interceptor.h"

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestJob;
}

namespace previews {
class PreviewsDecider;
}

namespace offline_pages {

// An interceptor to hijack requests and potentially service them based on
// their offline information. Created one per profile.
class OfflinePageRequestInterceptor : public net::URLRequestInterceptor {
 public:
  // Embedder must guarantee that |previews_decider| outlives |this|.
  explicit OfflinePageRequestInterceptor(
      previews::PreviewsDecider* previews_decider);
  ~OfflinePageRequestInterceptor() override;

 private:
  // Overrides from net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  // Used to determine if an URLRequest is eligible for offline previews.
  previews::PreviewsDecider* previews_decider_;

  DISALLOW_COPY_AND_ASSIGN(OfflinePageRequestInterceptor);
};

}  // namespace offline_pages

#endif // CHROME_BROWSER_OFFLINE_PAGES_OFFLINE_PAGE_REQUEST_INTERCEPTOR_H_
