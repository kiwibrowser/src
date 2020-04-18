// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_WEB_STATE_JS_CRW_JS_POST_REQUEST_LOADER_H_
#define IOS_WEB_WEB_STATE_JS_CRW_JS_POST_REQUEST_LOADER_H_

#import <WebKit/WebKit.h>

@class CRWWKScriptMessageRouter;

// Class to load POST requests in a provided web view via JavaScript.
// TODO(crbug.com/740987): Remove |CRWJSPOSTRequestLoader| once iOS 10 is
// dropped.
@interface CRWJSPOSTRequestLoader : NSObject

// Asynchronously loads a POST |request| in provided |webView|.
// It temporarily installs JavaScript message routers with |messageRouter| to
// handle HTTP errors. The |completionHandler| is called once the request has
// been executed. In case of successful request, the passed error is nil.
// The |completionHandler| must not be null. The |messageRouter| and |webView|
// must not be nil. The |request| must be a POST request.
- (WKNavigation*)loadPOSTRequest:(NSURLRequest*)request
                       inWebView:(WKWebView*)webView
                   messageRouter:(CRWWKScriptMessageRouter*)messageRouter
               completionHandler:(void (^)(NSError*))completionHandler;

@end

#endif  // IOS_WEB_WEB_STATE_JS_CRW_JS_POST_REQUEST_LOADER_H_
