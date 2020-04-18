// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/web_http_protocol_handler_delegate.h"

#import <Foundation/Foundation.h>

#import "base/strings/sys_string_conversions.h"
#import "ios/web/public/url_scheme_util.h"
#import "ios/web/public/web_client.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

bool IsAppSpecificScheme(NSURL* url) {
  NSString* scheme = [url scheme];
  if (![scheme length])
    return false;
  // Use the GURL implementation, but with a scheme-only URL to avoid
  // unnecessary parsing in GURL construction.
  GURL gurl(base::SysNSStringToUTF8([scheme stringByAppendingString:@":"]));
  return web::GetWebClient()->IsAppSpecificURL(gurl);
}

}  // namespace

namespace web {

bool IsStaticFileRequest(NSURLRequest* request) {
  NSString* user_agent = [request allHTTPHeaderFields][@"User-Agent"];
  if (user_agent) {
    return [user_agent hasPrefix:@"UIWebViewForStaticFileContent"];
  }

  // If a request originated from another file:/// page, the User-Agent
  // is not available. In this case, check that the request is for image
  // resources only.
  NSString* suffix = [[request URL] pathExtension];
  return [@[ @"png", @"jpg", @"jpeg" ] containsObject:[suffix lowercaseString]];
}

WebHTTPProtocolHandlerDelegate::WebHTTPProtocolHandlerDelegate(
    net::URLRequestContextGetter* default_getter)
    : default_getter_(default_getter) {
  DCHECK(default_getter_);
}

WebHTTPProtocolHandlerDelegate::~WebHTTPProtocolHandlerDelegate() {
}

bool WebHTTPProtocolHandlerDelegate::CanHandleRequest(NSURLRequest* request) {
  // Accept all the requests. If we declined a request, it would then be passed
  // to the default iOS network stack, which would possibly load it.
  // As we want to control what is loaded, we have to prevent the default stack
  // from loading anything.
  return true;
}

bool WebHTTPProtocolHandlerDelegate::IsRequestSupported(NSURLRequest* request) {
  return web::UrlHasWebScheme([request URL]) || IsStaticFileRequest(request) ||
         (IsAppSpecificScheme([request URL]) &&
          IsAppSpecificScheme([request mainDocumentURL]));
}

net::URLRequestContextGetter*
WebHTTPProtocolHandlerDelegate::GetDefaultURLRequestContext() {
  return default_getter_.get();
}

}  // namespace web
