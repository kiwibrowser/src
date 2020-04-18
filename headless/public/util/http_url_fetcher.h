// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_PUBLIC_UTIL_HTTP_URL_FETCHER_H_
#define HEADLESS_PUBLIC_UTIL_HTTP_URL_FETCHER_H_

#include "base/macros.h"
#include "headless/public/util/url_fetcher.h"

namespace net {
class URLRequestContext;
}  // namespace

namespace headless {

// A simple URLFetcher that uses a net::URLRequestContext to as a back end for
// http(s) fetches.
class HttpURLFetcher : public URLFetcher {
 public:
  explicit HttpURLFetcher(const net::URLRequestContext* url_request_context);
  ~HttpURLFetcher() override;

  // URLFetcher implementation:
  void StartFetch(const Request* request,
                  ResultListener* result_listener) override;

 private:
  class Delegate;

  const net::URLRequestContext* url_request_context_;
  std::unique_ptr<Delegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(HttpURLFetcher);
};

}  // namespace headless

#endif  // HEADLESS_PUBLIC_UTIL_HTTP_URL_FETCHER_H_
