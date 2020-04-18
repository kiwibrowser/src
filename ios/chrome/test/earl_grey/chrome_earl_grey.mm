// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"

#import <Foundation/Foundation.h>
#import <WebKit/WebKit.h>

#include "base/format_macros.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "base/test/ios/wait_util.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/ui/static_content/static_html_view_controller.h"
#import "ios/chrome/test/app/bookmarks_test_util.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/history_test_util.h"
#include "ios/chrome/test/app/navigation_test_util.h"
#import "ios/chrome/test/app/static_html_view_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/earl_grey/js_test_util.h"
#import "ios/web/public/test/web_view_content_test_util.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace chrome_test_util {

id ExecuteJavaScript(NSString* javascript,
                     NSError* __autoreleasing* out_error) {
  __block bool did_complete = false;
  __block id result = nil;
  __block NSError* temp_error = nil;
  CRWJSInjectionReceiver* evaluator =
      chrome_test_util::GetCurrentWebState()->GetJSInjectionReceiver();
  [evaluator executeJavaScript:javascript
             completionHandler:^(id value, NSError* error) {
               did_complete = true;
               result = [value copy];
               temp_error = [error copy];
             }];

  // Wait for completion.
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for JavaScript execution to complete."
                  block:^BOOL {
                    return did_complete;
                  }];
  [condition waitWithTimeout:testing::kWaitForJSCompletionTimeout];
  if (!did_complete)
    return nil;
  if (out_error) {
    NSError* __autoreleasing auto_released_error = temp_error;
    *out_error = auto_released_error;
  }
  return result;
}

}  // namespace chrome_test_util

@implementation ChromeEarlGrey

#pragma mark - History Utilities

+ (void)clearBrowsingHistory {
  GREYAssertTrue(chrome_test_util::ClearBrowsingHistory(),
                 @"Clearing Browsing History timed out");
  // After clearing browsing history via code, wait for the UI to be done
  // with any updates. This includes icons from the new tab page being removed.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
}

#pragma mark - Cookie Utilities

+ (NSDictionary*)cookies {
  NSString* const kGetCookiesScript =
      @"document.cookie ? document.cookie.split(/;\\s*/) : [];";

  NSError* error = nil;
  id result = chrome_test_util::ExecuteJavaScript(kGetCookiesScript, &error);

  GREYAssertTrue(result && !error, @"Failed to get cookies.");

  NSArray* nameValuePairs = base::mac::ObjCCastStrict<NSArray>(result);
  NSMutableDictionary* cookies = [NSMutableDictionary dictionary];
  for (NSString* nameValuePair in nameValuePairs) {
    NSArray* cookieNameValue = [nameValuePair componentsSeparatedByString:@"="];
    GREYAssertEqual(2U, cookieNameValue.count, @"Cookie has invalid format.");

    NSString* cookieName = cookieNameValue[0];
    NSString* cookieValue = cookieNameValue[1];
    cookies[cookieName] = cookieValue;
  }

  return cookies;
}

#pragma mark - Navigation Utilities

+ (void)loadURL:(const GURL&)URL {
  chrome_test_util::LoadUrl(URL);
  [ChromeEarlGrey waitForPageToFinishLoading];

  web::WebState* webState = chrome_test_util::GetCurrentWebState();
  if (webState->ContentIsHTML())
    web::WaitUntilWindowIdInjected(webState);
}

+ (void)reload {
  [chrome_test_util::BrowserCommandDispatcherForMainBVC() reload];
  [ChromeEarlGrey waitForPageToFinishLoading];
}

+ (void)goBack {
  [chrome_test_util::BrowserCommandDispatcherForMainBVC() goBack];

  [ChromeEarlGrey waitForPageToFinishLoading];
}

+ (void)goForward {
  [chrome_test_util::BrowserCommandDispatcherForMainBVC() goForward];

  [ChromeEarlGrey waitForPageToFinishLoading];
}

+ (void)waitForPageToFinishLoading {
  GREYAssert(chrome_test_util::WaitForPageToFinishLoading(),
             @"Page did not complete loading.");
}

+ (void)tapWebViewElementWithID:(NSString*)elementID {
  BOOL success =
      web::test::TapWebViewElementWithId(chrome_test_util::GetCurrentWebState(),
                                         base::SysNSStringToUTF8(elementID));
  GREYAssertTrue(success, @"Failed to tap web view element with ID: %@",
                 elementID);
}

+ (void)waitForErrorPage {
  NSString* const kErrorPageText =
      l10n_util::GetNSString(IDS_ERRORPAGES_HEADING_NOT_AVAILABLE);
  [self waitForStaticHTMLViewContainingText:kErrorPageText];
}

+ (void)waitForStaticHTMLViewContainingText:(NSString*)text {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for static HTML text."
                  block:^BOOL {
                    return chrome_test_util::StaticHtmlViewContainingText(
                        chrome_test_util::GetCurrentWebState(),
                        base::SysNSStringToUTF8(text));
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed to find static html view containing %@", text);
}

+ (void)waitForStaticHTMLViewNotContainingText:(NSString*)text {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for absence of static HTML text."
                  block:^BOOL {
                    return !chrome_test_util::StaticHtmlViewContainingText(
                        chrome_test_util::GetCurrentWebState(),
                        base::SysNSStringToUTF8(text));
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed, there was a static html view containing %@", text);
}

+ (void)waitForWebViewContainingText:(std::string)text {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for web view containing text"
                  block:^BOOL {
                    return web::test::IsWebViewContainingText(
                        chrome_test_util::GetCurrentWebState(), text);
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for web view containing %s", text.c_str());
}

+ (void)waitForWebViewContainingCSSSelector:(std::string)selector {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for web view containing CSS selector"
                  block:^BOOL {
                    return web::test::IsWebViewContainingCssSelector(
                        chrome_test_util::GetCurrentWebState(), selector);
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for web view containing css selector: %s",
             selector.c_str());
}

+ (void)waitForWebViewNotContainingText:(std::string)text {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for web view not containing text"
                  block:^BOOL {
                    return !web::test::IsWebViewContainingText(
                        chrome_test_util::GetCurrentWebState(), text);
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for web view not containing %s", text.c_str());
}

+ (void)waitForMainTabCount:(NSUInteger)count {
  // Allow the UI to become idle, in case any tabs are being opened or closed.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for main tab count"
                  block:^BOOL {
                    return chrome_test_util::GetMainTabCount() == count;
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for main tab count to become %" PRIuNS, count);
}

+ (void)waitForIncognitoTabCount:(NSUInteger)count {
  // Allow the UI to become idle, in case any tabs are being opened or closed.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for incognito tab count"
                  block:^BOOL {
                    return chrome_test_util::GetIncognitoTabCount() == count;
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for incognito tab count to become %" PRIuNS,
             count);
}

+ (void)waitForWebViewContainingBlockedImageElementWithID:(std::string)imageID {
  GREYAssert(web::test::WaitForWebViewContainingImage(
                 imageID, chrome_test_util::GetCurrentWebState(),
                 web::test::IMAGE_STATE_BLOCKED),
             @"Failed waiting for web view blocked image %s", imageID.c_str());
}

+ (void)waitForWebViewContainingLoadedImageElementWithID:(std::string)imageID {
  GREYAssert(web::test::WaitForWebViewContainingImage(
                 imageID, chrome_test_util::GetCurrentWebState(),
                 web::test::IMAGE_STATE_LOADED),
             @"Failed waiting for web view loaded image %s", imageID.c_str());
}

+ (void)waitForBookmarksToFinishLoading {
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout,
                 ^{
                   return chrome_test_util::BookmarksLoaded();
                 }),
             @"Bookmark model did not load");
}

+ (void)waitForElementWithMatcherSufficientlyVisible:(id<GREYMatcher>)matcher {
  GREYCondition* condition = [GREYCondition
      conditionWithName:@"Wait for element with matcher sufficiently visible"
                  block:^BOOL {
                    NSError* error = nil;
                    [[EarlGrey selectElementWithMatcher:matcher]
                        assertWithMatcher:grey_sufficientlyVisible()
                                    error:&error];
                    return error == nil;
                  }];
  GREYAssert([condition waitWithTimeout:testing::kWaitForUIElementTimeout],
             @"Failed waiting for element with matcher %@ to become visible",
             matcher);
}

@end
