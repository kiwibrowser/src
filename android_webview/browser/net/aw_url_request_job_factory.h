// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_NET_AW_URL_REQUEST_JOB_FACTORY_H_
#define ANDROID_WEBVIEW_BROWSER_NET_AW_URL_REQUEST_JOB_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "net/url_request/url_request_job_factory.h"

namespace net {
class URLRequestJobFactoryImpl;
}  // namespace net

namespace android_webview {

// android_webview uses a custom URLRequestJobFactoryImpl to support
// navigation interception and URLRequestJob interception for navigations to
// url with unsupported schemes.
// This is achieved by returning a URLRequestErrorJob for schemes that would
// otherwise be unhandled, which gives the embedder an opportunity to intercept
// the request.
class AwURLRequestJobFactory : public net::URLRequestJobFactory {
 public:
  AwURLRequestJobFactory();
  ~AwURLRequestJobFactory() override;

  bool SetProtocolHandler(const std::string& scheme,
                          std::unique_ptr<ProtocolHandler> protocol_handler);

  // net::URLRequestJobFactory implementation.
  net::URLRequestJob* MaybeCreateJobWithProtocolHandler(
      const std::string& scheme,
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  net::URLRequestJob* MaybeInterceptRedirect(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      const GURL& location) const override;

  net::URLRequestJob* MaybeInterceptResponse(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

  bool IsHandledProtocol(const std::string& scheme) const override;
  bool IsSafeRedirectTarget(const GURL& location) const override;

 private:
  // By default calls are forwarded to this factory, to avoid having to
  // subclass an existing implementation class.
  std::unique_ptr<net::URLRequestJobFactoryImpl> next_factory_;

  DISALLOW_COPY_AND_ASSIGN(AwURLRequestJobFactory);
};

} // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_NET_AW_URL_REQUEST_JOB_FACTORY_H_
