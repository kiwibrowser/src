// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/error_page_generator.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/web/public/test/error_test_util.h"
#include "ios/web/public/test/web_test.h"
#import "net/base/mac/url_conversions.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// From static_html_view_controller.mm: callback for the HtmlGenerator protocol.
typedef void (^HtmlCallback)(NSString*);

// To test the error page generator code, generate  error data, and examine the
// generated HTML to make sure the expected text has been included. In detail:
// 1. Use GenerateError to create an NSError object to pass in.
// 2. Initialize errorPageGenerator_ with NSError objects.
// 3. Pass the |TestHTMLForError| function to errorPageContent_'s generateHTML
//    function, along with the string that's expected to appear in the
//    generated HTML for this error.
class ErrorPageGeneratorTest : public web::WebTest {
 protected:
  void SetUp() override { web::WebTest::SetUp(); }

  // Test the HTML generated in the test against the error we were supposed to
  // find.
  static void TestHTMLForError(NSString* html, NSString* errorToFind) {
    EXPECT_TRUE([html rangeOfString:errorToFind].length > 0);
  }

  // Generate an NSError object with the format expected by LocalizedError,
  // given a URL, error domain, and error code.
  NSError* GenerateError(const GURL& errorURL,
                         NSString* errorDomain,
                         NSInteger errorCode) {
    NSDictionary* info = @{
      NSURLErrorFailingURLStringErrorKey :
          base::SysUTF8ToNSString(errorURL.spec())
    };
    return web::testing::CreateTestNetError(
        [NSError errorWithDomain:errorDomain code:errorCode userInfo:info]);
  }

  // ErrorPageContent object to be tested.
  ErrorPageGenerator* errorPageGenerator_;

  // Mock for the URLLoader that would otherwise handle the HTML produced.
  OCMockObject* mockLoader_;
};

// Test with a timed out error.
TEST_F(ErrorPageGeneratorTest, TimedOut) {
  GURL errorURL("http://www.google.com");
  NSError* error =
      GenerateError(errorURL, NSURLErrorDomain, kCFURLErrorTimedOut);
  errorPageGenerator_ =
      [[ErrorPageGenerator alloc] initWithError:error isPost:NO isIncognito:NO];
  [errorPageGenerator_ generateHtml:^(NSString* html) {
    TestHTMLForError(html, @"ERR_CONNECTION_TIMED_OUT");
  }];
}

// Test with a bad URL scheme (see b/7448754).
TEST_F(ErrorPageGeneratorTest, BadURLScheme) {
  GURL errorURL(
      "itms-appss://itunes.apple.com/gb/app/google-search/id284815942?mt=8");
  NSError* error =
      GenerateError(errorURL, NSURLErrorDomain, kCFURLErrorTimedOut);
  errorPageGenerator_ =
      [[ErrorPageGenerator alloc] initWithError:error isPost:NO isIncognito:NO];
  [errorPageGenerator_ generateHtml:^(NSString* html) {
    TestHTMLForError(html, @"ERR_CONNECTION_TIMED_OUT");
  }];
}

// Test with an empty GURL object.
// TODO(ios): [merge 191784] b/8525110 fix the code or the test.
TEST_F(ErrorPageGeneratorTest, EmptyURLObject) {
  GURL errorURL;
  NSError* error =
      GenerateError(errorURL, NSURLErrorDomain, kCFURLErrorTimedOut);
  errorPageGenerator_ =
      [[ErrorPageGenerator alloc] initWithError:error isPost:NO isIncognito:NO];
  [errorPageGenerator_ generateHtml:^(NSString* html) {
    TestHTMLForError(html, @"ERR_CONNECTION_TIMED_OUT");
  }];
}
