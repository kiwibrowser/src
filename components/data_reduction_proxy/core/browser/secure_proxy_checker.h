// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_SECURE_PROXY_CHECKER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_SECURE_PROXY_CHECKER_H_

#include <string>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_status.h"

namespace net {

class URLFetcher;
class URLRequestContextGetter;

}  // namespace net

namespace data_reduction_proxy {

typedef base::Callback<
    void(const std::string&, const net::URLRequestStatus&, int)>
    SecureProxyCheckerCallback;

// Checks if the secure proxy is allowed by the carrier by sending a probe.
class SecureProxyChecker : public net::URLFetcherDelegate {
 public:
  explicit SecureProxyChecker(const scoped_refptr<net::URLRequestContextGetter>&
                                  url_request_context_getter);

  ~SecureProxyChecker() override;

  void CheckIfSecureProxyIsAllowed(SecureProxyCheckerCallback fetcher_callback);

 private:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // The URLFetcher being used for the secure proxy check.
  std::unique_ptr<net::URLFetcher> fetcher_;
  SecureProxyCheckerCallback fetcher_callback_;

  // Used to determine the latency in performing the Data Reduction Proxy secure
  // proxy check.
  base::Time secure_proxy_check_start_time_;

  DISALLOW_COPY_AND_ASSIGN(SecureProxyChecker);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_SECURE_PROXY_CHECKER_H_