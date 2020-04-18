// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "base/ios/ios_util.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#include "ios/chrome/test/app/navigation_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "ios/testing/earl_grey/disabled_test_macros.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// URL which leads to a PDF file.
const char kPDFURL[] = "http://ios/testing/data/http_server_files/testpage.pdf";

// A test HTML URL.
const char kHTMLURL[] = "http://test";
}  // namespace

// Print test cases. These are Earl Grey integration tests.
// They test printing a normal page and a pdf.
@interface PrintControllerTestCase : ChromeTestCase

// Tests that it is possible to print the current page by opening the sharing
// menu, tapping on print and verifying it displays the print page.
- (void)printCurrentPage;

@end

@implementation PrintControllerTestCase

// Tests that the AirPrint menu successfully loads when a normal web page is
// loaded.
// TODO(crbug.com/683280): Does this test serve any purpose on iOS11?
- (void)testPrintNormalPage {
  if (base::ios::IsRunningOnIOS11OrLater() && IsUIRefreshPhase1Enabled()) {
    EARL_GREY_TEST_SKIPPED(
        @"Dispatcher-based printing does not work on iOS11 when the "
        @"UIRefresh flag is enabled.");
  }

  GURL url = web::test::HttpServer::MakeUrl(kHTMLURL);
  std::map<GURL, std::string> responses;
  std::string response = "Test";
  responses[url] = response;
  web::test::SetUpSimpleHttpServer(responses);

  chrome_test_util::LoadUrl(url);
  [ChromeEarlGrey waitForWebViewContainingText:response];

  [self printCurrentPage];
}

// Tests that the AirPrint menu successfully loads when a PDF is loaded.
// TODO(crbug.com/683280): Does this test serve any purpose on iOS11?
- (void)testPrintPDF {
  if (base::ios::IsRunningOnIOS11OrLater() && IsUIRefreshPhase1Enabled()) {
    EARL_GREY_TEST_SKIPPED(
        @"Dispatcher-based printing does not work on iOS11 when the "
        @"UIRefresh flag is enabled.");
  }

  web::test::SetUpFileBasedHttpServer();
  GURL url = web::test::HttpServer::MakeUrl(kPDFURL);
  chrome_test_util::LoadUrl(url);

  [self printCurrentPage];
}

- (void)printCurrentPage {
  // EarlGrey does not have the ability to interact with the share menu in
  // iOS11, so use the dispatcher to trigger the print view controller instead.
  if (base::ios::IsRunningOnIOS11OrLater()) {
    DCHECK(!IsUIRefreshPhase1Enabled());
    [chrome_test_util::DispatcherForActiveViewController() printTab];
  } else {
    [ChromeEarlGreyUI openShareMenu];
    id<GREYMatcher> printButton =
        grey_allOf(grey_accessibilityLabel(@"Print"),
                   grey_accessibilityTrait(UIAccessibilityTraitButton), nil);
    [[EarlGrey selectElementWithMatcher:printButton] performAction:grey_tap()];
  }

  id<GREYMatcher> printerOptionButton = grey_allOf(
      grey_accessibilityID(@"Printer Options"),
      grey_not(grey_accessibilityTrait(UIAccessibilityTraitHeader)), nil);
  [[EarlGrey selectElementWithMatcher:printerOptionButton]
      assertWithMatcher:grey_sufficientlyVisible()];
}

@end
