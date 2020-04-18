// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#import "base/test/ios/wait_util.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/earl_grey/web_view_matchers.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#import "ios/web/shell/test/app/web_shell_test_util.h"
#import "ios/web/shell/test/earl_grey/shell_earl_grey.h"
#import "ios/web/shell/test/earl_grey/web_shell_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// URL spec for test PDF page.
const char kTestPDFURL[] =
    "http://ios/web/shell/test/http_server_files/testpage.pdf";

// Matcher for WKWebView displaying PDF.
id<GREYMatcher> WebViewWithPdf() {
  web::WebState* web_state = web::shell_test_util::GetCurrentWebState();
  MatchesBlock matches = ^BOOL(UIView* view) {
    return testing::WaitUntilConditionOrTimeout(
        testing::kWaitForUIElementTimeout, ^{
          return web_state->GetContentsMimeType() == "application/pdf";
        });
  };

  DescribeToBlock describe = ^(id<GREYDescription> description) {
    [description appendText:@"web view with PDF"];
  };

  return grey_allOf(
      WebViewInWebState(web_state),
      [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                           descriptionBlock:describe],
      nil);
}

}  // namespace

using web::test::HttpServer;

// PDF test cases for the web shell.
@interface PDFTestCase : WebShellTestCase
@end

@implementation PDFTestCase

// Tests MIME type of the loaded PDF document.
- (void)testMIMEType {
  web::test::SetUpFileBasedHttpServer();
  [ShellEarlGrey loadURL:HttpServer::MakeUrl(kTestPDFURL)];
  [[EarlGrey selectElementWithMatcher:WebViewWithPdf()]
      assertWithMatcher:grey_notNil()];
}

@end
