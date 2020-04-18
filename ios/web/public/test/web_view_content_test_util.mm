// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/web_view_content_test_util.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/values.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"
#import "net/base/mac/url_conversions.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::kWaitForDownloadTimeout;
using testing::WaitUntilConditionOrTimeout;

// A helper delegate class that allows downloading responses with invalid
// SSL certs.
@interface TestURLSessionDelegate : NSObject<NSURLSessionDelegate>
@end

@implementation TestURLSessionDelegate

- (void)URLSession:(NSURLSession*)session
    didReceiveChallenge:(NSURLAuthenticationChallenge*)challenge
      completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition,
                                  NSURLCredential*))completionHandler {
  SecTrustRef serverTrust = challenge.protectionSpace.serverTrust;
  completionHandler(NSURLSessionAuthChallengeUseCredential,
                    [NSURLCredential credentialForTrust:serverTrust]);
}

@end

namespace {
// Script that returns document.body as a string.
char kGetDocumentBodyJavaScript[] =
    "document.body ? document.body.textContent : null";

// Fetches the image from |image_url|.
UIImage* LoadImage(const GURL& image_url) {
  __block UIImage* image;
  __block NSError* error;
  TestURLSessionDelegate* session_delegate =
      [[TestURLSessionDelegate alloc] init];
  NSURLSessionConfiguration* session_config =
      [NSURLSessionConfiguration ephemeralSessionConfiguration];
  NSURLSession* session =
      [NSURLSession sessionWithConfiguration:session_config
                                    delegate:session_delegate
                               delegateQueue:nil];
  id completion_handler = ^(NSData* data, NSURLResponse*, NSError* task_error) {
    error = task_error;
    image = [[UIImage alloc] initWithData:data];
  };

  NSURLSessionDataTask* task =
      [session dataTaskWithURL:net::NSURLWithGURL(image_url)
             completionHandler:completion_handler];
  [task resume];

  bool task_completed = WaitUntilConditionOrTimeout(kWaitForDownloadTimeout, ^{
    return image || error;
  });

  if (!task_completed) {
    return nil;
  }
  return image;
}
}

using testing::WaitUntilConditionOrTimeout;
using testing::kWaitForUIElementTimeout;

namespace web {
namespace test {

bool IsWebViewContainingText(web::WebState* web_state,
                             const std::string& text) {
  std::unique_ptr<base::Value> value =
      web::test::ExecuteJavaScript(web_state, kGetDocumentBodyJavaScript);
  std::string body;
  if (value && value->GetAsString(&body)) {
    return body.find(text) != std::string::npos;
  }
  return false;
}

bool WaitForWebViewContainingText(web::WebState* web_state, std::string text) {
  return WaitUntilConditionOrTimeout(kWaitForUIElementTimeout, ^{
    return IsWebViewContainingText(web_state, text);
  });
}

bool WaitForWebViewContainingImage(std::string image_id,
                                   web::WebState* web_state,
                                   ImageStateElement image_state) {
  std::string get_url_script =
      base::StringPrintf("document.getElementById('%s').src", image_id.c_str());
  std::unique_ptr<base::Value> url_as_value =
      web::test::ExecuteJavaScript(web_state, get_url_script);
  std::string url_as_string;
  if (!url_as_value->GetAsString(&url_as_string))
    return false;

  UIImage* image = LoadImage(GURL(url_as_string));
  if (!image)
    return false;

  CGSize expected_size = image.size;

  return WaitUntilConditionOrTimeout(kWaitForUIElementTimeout, ^{
    NSString* const kGetElementAttributesScript =
        [NSString stringWithFormat:
                      @"var image = document.getElementById('%@');"
                      @"var imageHeight = image.height;"
                      @"var imageWidth = image.width;"
                      @"JSON.stringify({"
                      @"  height:imageHeight,"
                      @"  width:imageWidth"
                      @"});",
                      base::SysUTF8ToNSString(image_id)];
    std::unique_ptr<base::Value> value = web::test::ExecuteJavaScript(
        web_state, base::SysNSStringToUTF8(kGetElementAttributesScript));
    std::string result;
    if (value && value->GetAsString(&result)) {
      NSString* evaluation_result = base::SysUTF8ToNSString(result);
      NSData* image_attributes_as_data =
          [evaluation_result dataUsingEncoding:NSUTF8StringEncoding];
      NSDictionary* image_attributes =
          [NSJSONSerialization JSONObjectWithData:image_attributes_as_data
                                          options:0
                                            error:nil];
      CGFloat height = [image_attributes[@"height"] floatValue];
      CGFloat width = [image_attributes[@"width"] floatValue];
      switch (image_state) {
        case IMAGE_STATE_BLOCKED:
          return height < expected_size.height && width < expected_size.width;
        case IMAGE_STATE_LOADED:
          return height == expected_size.height && width == expected_size.width;
      }
    }
    return false;
  });
}

bool IsWebViewContainingCssSelector(web::WebState* web_state,
                                    const std::string& css_selector) {
  // Script that tests presence of css selector.
  char testCssSelectorJavaScriptTemplate[] =
      "!!document.querySelector(\"%s\");";
  std::string script = base::StringPrintf(testCssSelectorJavaScriptTemplate,
                                          css_selector.c_str());

  bool did_succeed = false;
  std::unique_ptr<base::Value> value =
      web::test::ExecuteJavaScript(web_state, script);
  if (value) {
    value->GetAsBoolean(&did_succeed);
  }
  return did_succeed;
}

}  // namespace test
}  // namespace web
