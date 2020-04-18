// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_CHROMIUM_URL_REQUEST_H_
#define REMOTING_BASE_CHROMIUM_URL_REQUEST_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "remoting/base/url_request.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace remoting {

// UrlRequest implementation based on net::URLFetcher.
class ChromiumUrlRequest : public UrlRequest, public net::URLFetcherDelegate {
 public:
  ChromiumUrlRequest(
      scoped_refptr<net::URLRequestContextGetter> url_context,
      UrlRequest::Type type,
      const std::string& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~ChromiumUrlRequest() override;

  // UrlRequest interface.
  void AddHeader(const std::string& value) override;
  void SetPostData(const std::string& content_type,
                   const std::string& data) override;
  void Start(const OnResultCallback& on_result_callback) override;

 private:
  // net::URLFetcherDelegate interface.
  void OnURLFetchComplete(const net::URLFetcher* url_fetcher) override;

  std::unique_ptr<net::URLFetcher> url_fetcher_;
  OnResultCallback on_result_callback_;
};

class ChromiumUrlRequestFactory : public UrlRequestFactory {
 public:
  ChromiumUrlRequestFactory(
      scoped_refptr<net::URLRequestContextGetter> url_context);
  ~ChromiumUrlRequestFactory() override;

  // UrlRequestFactory interface.
  std::unique_ptr<UrlRequest> CreateUrlRequest(
      UrlRequest::Type type,
      const std::string& url,
      const net::NetworkTrafficAnnotationTag& traffic_annotation) override;

 private:
  scoped_refptr<net::URLRequestContextGetter> url_context_;
};

}  // namespace remoting

#endif  // REMOTING_BASE_CHROMIUM_URL_REQUEST_H_
