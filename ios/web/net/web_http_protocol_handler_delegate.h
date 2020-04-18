// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_NET_WEB_HTTP_PROTOCOL_HANDLER_DELEGATE_H_
#define IOS_WEB_NET_WEB_HTTP_PROTOCOL_HANDLER_DELEGATE_H_

#include "base/memory/ref_counted.h"
#import "ios/net/crn_http_protocol_handler.h"

@class NSURLRequest;

namespace web {

// Returns whether the request should be allowed for rendering into a special
// UIWebView that allows static file content.
bool IsStaticFileRequest(NSURLRequest* request);

// Web-specific implementation of net::HTTPProtocolHandlerDelegate.
class WebHTTPProtocolHandlerDelegate : public net::HTTPProtocolHandlerDelegate {
 public:
  WebHTTPProtocolHandlerDelegate(net::URLRequestContextGetter* default_getter);
  ~WebHTTPProtocolHandlerDelegate() override;

  // net::HTTPProtocolHandlerDelegate implementation:
  bool CanHandleRequest(NSURLRequest* request) override;
  bool IsRequestSupported(NSURLRequest* request) override;
  net::URLRequestContextGetter* GetDefaultURLRequestContext() override;

 private:
  scoped_refptr<net::URLRequestContextGetter> default_getter_;
};

}  // namespace web

#endif  // IOS_WEB_NET_WEB_HTTP_PROTOCOL_HANDLER_DELEGATE_H_
