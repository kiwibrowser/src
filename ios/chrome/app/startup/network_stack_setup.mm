// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/app/startup/network_stack_setup.h"

#include <memory>

#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/net/empty_nsurlcache.h"
#include "ios/web/net/request_tracker_factory_impl.h"
#include "ios/web/net/web_http_protocol_handler_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation NetworkStackSetup

+ (void)setUpChromeNetworkStack:
            (std::unique_ptr<web::RequestTrackerFactoryImpl>*)
                requestTrackerFactory
    httpProtocolHandlerDelegate:
        (std::unique_ptr<web::WebHTTPProtocolHandlerDelegate>*)
            httpProtocolHandlerDelegate {
  // Disable the default cache.
  [NSURLCache setSharedURLCache:[EmptyNSURLCache emptyNSURLCache]];

  // Configuration for the HTTP protocol handler.
  //  TODO(crbug.com/585700): Remove this code.
  *httpProtocolHandlerDelegate =
      std::make_unique<web::WebHTTPProtocolHandlerDelegate>(
          GetApplicationContext()->GetSystemURLRequestContext());
  net::HTTPProtocolHandlerDelegate::SetInstance(
      httpProtocolHandlerDelegate->get());

  // Register the chrome http protocol handler to replace the default one.
  // TODO(crbug.com/665036): Move the network stack initialization to the web
  // layer.
  BOOL success = [NSURLProtocol registerClass:[CRNHTTPProtocolHandler class]];
  DCHECK(success);
  *requestTrackerFactory =
      std::make_unique<web::RequestTrackerFactoryImpl>(kChromeUIScheme);
  net::RequestTracker::SetRequestTrackerFactory(requestTrackerFactory->get());

  DCHECK(success);
}

@end
