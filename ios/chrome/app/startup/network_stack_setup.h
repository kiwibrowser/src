// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_APP_STARTUP_NETWORK_STACK_SETUP_H_
#define IOS_CHROME_APP_STARTUP_NETWORK_STACK_SETUP_H_

#import <UIKit/UIKit.h>

#import <memory>

namespace web {
class RequestTrackerFactoryImpl;
class WebHTTPProtocolHandlerDelegate;
}  // namespace web

@interface NetworkStackSetup : NSObject

// Sets up the network stack: protocol handler and cache.
// TODO(crbug.com/585700): Remove the first parameter.
+ (void)setUpChromeNetworkStack:
            (std::unique_ptr<web::RequestTrackerFactoryImpl>*)
                requestTrackerFactory
    httpProtocolHandlerDelegate:
        (std::unique_ptr<web::WebHTTPProtocolHandlerDelegate>*)
            httpProtocolHandlerDelegate;

@end

#endif  // IOS_CHROME_APP_STARTUP_NETWORK_STACK_SETUP_H_
