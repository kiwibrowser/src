// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/web_state/js/crw_js_post_request_loader.h"

#include "base/json/string_escape.h"
#import "base/strings/sys_string_conversions.h"
#import "ios/web/web_state/js/page_script_util.h"
#import "ios/web/web_state/ui/crw_wk_script_message_router.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Escapes characters and encloses given string in quotes for use in JavaScript.
NSString* EscapeAndQuoteStringForJavaScript(NSString* unescapedString) {
  std::string string = base::SysNSStringToUTF8(unescapedString);
  return base::SysUTF8ToNSString(base::GetQuotedJSONString(string));
}

// JavaScript message handler name installed in WKWebView for request errors.
NSString* const kErrorHandlerName = @"POSTErrorHandler";

// JavaScript message handler name installed in WKWebView for successful
// request completion.
NSString* const kSuccessHandlerName = @"POSTSuccessHandler";

}  // namespace

@interface CRWJSPOSTRequestLoader () {
  NSString* _requestScript;
}

// JavaScript used to execute POST requests. Lazily instantiated.
@property(nonatomic, copy, readonly) NSString* requestScript;

// Handler for UIApplicationDidReceiveMemoryWarningNotification.
- (void)handleMemoryWarning;

// Forms a JavaScript method call to |requestScript| that executes given
// |request|.
- (NSString*)scriptToExecutePOSTRequest:(NSURLRequest*)request;

// Converts a dictionary of HTTP request headers to a JavaScript object.
- (NSString*)JSONForJavaScriptFromRequestHeaders:(NSDictionary*)headers;

@end

@implementation CRWJSPOSTRequestLoader

- (instancetype)init {
  self = [super init];
  if (self) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(handleMemoryWarning)
               name:UIApplicationDidReceiveMemoryWarningNotification
             object:nil];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (NSString*)requestScript {
  if (!_requestScript) {
    _requestScript = [web::GetPageScript(@"post_request") copy];
  }
  return _requestScript;
}

- (WKNavigation*)loadPOSTRequest:(NSURLRequest*)request
                       inWebView:(WKWebView*)webView
                   messageRouter:(CRWWKScriptMessageRouter*)messageRouter
               completionHandler:(void (^)(NSError*))completionHandler {
  DCHECK([request.HTTPMethod isEqualToString:@"POST"]);
  DCHECK(webView);
  DCHECK(messageRouter);
  DCHECK(completionHandler);

  // Install error handling and success routers.
  __weak CRWWKScriptMessageRouter* weakRouter = messageRouter;
  [messageRouter setScriptMessageHandler:^(WKScriptMessage* message) {
    // Cleaning up script handlers.
    [weakRouter removeScriptMessageHandlerForName:kErrorHandlerName
                                          webView:webView];
    [weakRouter removeScriptMessageHandlerForName:kSuccessHandlerName
                                          webView:webView];
    completionHandler(nil);
  }
                                    name:kSuccessHandlerName
                                 webView:webView];

  [messageRouter setScriptMessageHandler:^(WKScriptMessage* message) {
    NSNumber* statusCode = message.body;
    NSError* error = [NSError errorWithDomain:NSURLErrorDomain
                                         code:statusCode.integerValue
                                     userInfo:nil];
    [weakRouter removeScriptMessageHandlerForName:kErrorHandlerName
                                          webView:webView];
    [weakRouter removeScriptMessageHandlerForName:kSuccessHandlerName
                                          webView:webView];
    completionHandler(error);
  }
                                    name:kErrorHandlerName
                                 webView:webView];

  NSString* HTML =
      [NSString stringWithFormat:@"<html><script>%@%@</script></html>",
                                 self.requestScript,
                                 [self scriptToExecutePOSTRequest:request]];
  return [webView loadHTMLString:HTML baseURL:request.URL];
}

#pragma mark - Private methods.

- (void)handleMemoryWarning {
  // Request script can be recreated from file at any moment.
  _requestScript = nil;
}

- (NSString*)scriptToExecutePOSTRequest:(NSURLRequest*)request {
  NSDictionary* headers = [request allHTTPHeaderFields];
  NSString* headerString = [self JSONForJavaScriptFromRequestHeaders:headers];
  NSString* URLString = [[request URL] absoluteString];
  NSString* contentType = headers[@"Content-Type"];
  NSString* base64Data = [[request HTTPBody] base64EncodedStringWithOptions:0];

  // Here |headerString| is already properly escaped when returned from
  // -JSONForJavaScriptFromRequestHeaders:.
  return
      [NSString stringWithFormat:
                    @"__crPostRequestWorkaround.runPostRequest(%@, %@, %@, %@)",
                    EscapeAndQuoteStringForJavaScript(URLString), headerString,
                    EscapeAndQuoteStringForJavaScript(base64Data),
                    EscapeAndQuoteStringForJavaScript(contentType)];
}

- (NSString*)JSONForJavaScriptFromRequestHeaders:(NSDictionary*)headers {
  if (headers) {
    NSData* headerData =
        [NSJSONSerialization dataWithJSONObject:headers options:0 error:nil];
    if (headerData) {
      // This string is properly escaped by NSJSONSerialization. It needs to
      // have no quotes since JavaScripts takes this parameter as an
      // Object<string, string>.
      return [[NSString alloc] initWithData:headerData
                                   encoding:NSUTF8StringEncoding];
    }
  }
  return @"{}";
}

@end
