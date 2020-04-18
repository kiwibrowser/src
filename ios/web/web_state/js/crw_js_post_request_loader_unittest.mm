// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/js/crw_js_post_request_loader.h"

#import <WebKit/WebKit.h>

#import "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "base/test/ios/wait_util.h"
#include "ios/web/public/test/web_test.h"
#import "ios/web/public/web_view_creation_util.h"
#import "ios/web/web_state/ui/crw_wk_script_message_router.h"
#import "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace base {
namespace {

typedef web::WebTest CRWJSPOSTRequestLoaderTest;

// This script takes a JavaScript blob and converts it to a base64-encoded
// string asynchronously, then is sent to XHRSendHandler message handler.
NSString* const kBlobToBase64StringScript =
    @"var blobToBase64 = function(x) {"
     "  var reader = new window.FileReader();"
     "  reader.readAsDataURL(x);"
     "  reader.onloadend = function() {"
     "    base64data = reader.result;"
     "    window.webkit.messageHandlers.XHRSendHandler.postMessage(base64data);"
     "  };"
     "};";

TEST_F(CRWJSPOSTRequestLoaderTest, LoadsCorrectHTML) {
  // Set up necessary objects.
  CRWJSPOSTRequestLoader* loader = [[CRWJSPOSTRequestLoader alloc] init];
  WKWebView* web_view = web::BuildWKWebView(CGRectZero, GetBrowserState());
  WKUserContentController* contentController =
      web_view.configuration.userContentController;
  CRWWKScriptMessageRouter* messageRouter = [[CRWWKScriptMessageRouter alloc]
      initWithUserContentController:contentController];

  // Override XMLHttpRequest.send() to call kBlobToBase64StringScript.
  __block BOOL overrideSuccessfull = NO;
  NSString* JS = [kBlobToBase64StringScript stringByAppendingString:@";\
      XMLHttpRequest.prototype.send = function(x) { blobToBase64(x); };"];
  [web_view evaluateJavaScript:JS
             completionHandler:^(id, NSError*) {
               overrideSuccessfull = YES;
             }];
  base::test::ios::WaitUntilCondition(^bool {
    return overrideSuccessfull;
  });

  NSString* post_body = @"123";

  // Adds XHRSendHandler handler that checks that the POST request body is
  // correct. Sets |complete| flag upon completion.
  __block BOOL complete = NO;
  void (^XHRSendHandler)(WKScriptMessage*) = ^(WKScriptMessage* message) {
    NSString* body = base::mac::ObjCCast<NSString>(message.body);
    NSArray* components = [body componentsSeparatedByString:@","];
    EXPECT_EQ(components.count, 2u);
    EXPECT_NSEQ(components[0], @"data:;base64");
    NSData* expectedData = [post_body dataUsingEncoding:NSUTF8StringEncoding];
    EXPECT_NSEQ(components[1], [expectedData base64EncodedStringWithOptions:0]);
    complete = YES;
  };

  [messageRouter setScriptMessageHandler:XHRSendHandler
                                    name:@"XHRSendHandler"
                                 webView:web_view];

  // Construct and perform the POST request.
  NSURL* url = [NSURL URLWithString:@"http://google.com"];
  NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
  request.HTTPMethod = @"POST";
  request.HTTPBody = [post_body dataUsingEncoding:NSUTF8StringEncoding];
  [loader loadPOSTRequest:request
                inWebView:web_view
            messageRouter:messageRouter
        completionHandler:^(NSError*){
        }];

  // Wait until the JavaScript message handler is called.
  base::test::ios::WaitUntilCondition(^bool {
    return complete;
  });

  // Clean up installed script handler.
  [messageRouter removeScriptMessageHandlerForName:@"XHRSendHandler"
                                           webView:web_view];
}

}  // namespace
}  // namespace base
