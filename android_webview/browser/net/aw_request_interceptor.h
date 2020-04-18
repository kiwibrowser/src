// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_AW_REQUEST_INTERCEPTOR_H_
#define ANDROID_WEBVIEW_BROWSER_NET_AW_REQUEST_INTERCEPTOR_H_

#include <memory>

#include "base/macros.h"
#include "net/url_request/url_request_interceptor.h"

namespace net {
class URLRequest;
class URLRequestJob;
class NetworkDelegate;
}

namespace android_webview {

// This class allows the Java-side embedder to substitute the default
// URLRequest of a given request for an alternative job that will read data
// from a Java stream.
class AwRequestInterceptor : public net::URLRequestInterceptor {
 public:
  AwRequestInterceptor();
  ~AwRequestInterceptor() override;

  // net::URLRequestInterceptor override --------------------------------------
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AwRequestInterceptor);
};

} // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_AW_REQUEST_INTERCEPTOR_H_
