// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#include "base/strings/sys_string_conversions.h"
#include "components/infobars/core/infobar.h"
#include "components/infobars/core/infobar_manager.h"
#import "ios/chrome/app/main_controller.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/infobars/test_infobar_delegate.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Timeout for how long to wait for an infobar to appear or disapper.
const CFTimeInterval kTimeout = 4.0;

// Returns the InfoBarManager for the current tab.  Only works in normal
// (non-incognito) mode.
infobars::InfoBarManager* GetCurrentInfoBarManager() {
  MainController* main_controller = chrome_test_util::GetMainController();
  web::WebState* webState =
      [[[[main_controller browserViewInformation] mainTabModel] currentTab]
          webState];
  if (webState) {
    return InfoBarManagerImpl::FromWebState(webState);
  }
  return nullptr;
}

// Adds a TestInfoBar to the current tab.
bool AddTestInfoBarToCurrentTab() {
  infobars::InfoBarManager* manager = GetCurrentInfoBarManager();
  return TestInfoBarDelegate::Create(manager);
}

// Verifies that a single TestInfoBar is either present or absent on the current
// tab.
void VerifyTestInfoBarVisibleForCurrentTab(bool visible) {
  // Expected values.
  bool expected_count = visible ? 1U : 0U;
  id<GREYMatcher> expected_visibility =
      visible ? grey_sufficientlyVisible() : grey_notVisible();
  NSString* condition_name =
      visible ? @"Waiting for infobar to show" : @"Waiting for infobar to hide";

  infobars::InfoBarManager* manager = GetCurrentInfoBarManager();
  GREYAssertEqual(expected_count, manager->infobar_count(),
                  @"Incorrect number of infobars.");
  [[GREYCondition
      conditionWithName:condition_name
                  block:^BOOL {
                    NSError* error = nil;
                    [[EarlGrey
                        selectElementWithMatcher:
                            chrome_test_util::StaticTextWithAccessibilityLabel(
                                base::SysUTF8ToNSString(kTestInfoBarTitle))]
                        assertWithMatcher:expected_visibility
                                    error:&error];
                    return error == nil;
                  }] waitWithTimeout:kTimeout];
}

}  // namespace

// Tests functionality related to infobars.
@interface InfobarTestCase : ChromeTestCase
@end

@implementation InfobarTestCase

// Tests that page infobars don't persist on navigation.
- (void)testInfobarsDismissOnNavigate {
  web::test::SetUpFileBasedHttpServer();

  // Open a new tab and navigate to the test page.
  const GURL testURL = web::test::HttpServer::MakeUrl(
      "http://ios/testing/data/http_server_files/pony.html");
  [ChromeEarlGrey loadURL:testURL];
  [ChromeEarlGrey waitForMainTabCount:1];

  // Add a test infobar to the current tab. Verify that the infobar is present
  // in the model and that the infobar view is visible on screen.
  GREYAssert(AddTestInfoBarToCurrentTab(),
             @"Failed to add infobar to test tab.");
  VerifyTestInfoBarVisibleForCurrentTab(true);

  // Navigate to a different page.  Verify that the infobar is dismissed and no
  // longer visible on screen.
  [ChromeEarlGrey loadURL:GURL(url::kAboutBlankURL)];
  VerifyTestInfoBarVisibleForCurrentTab(false);
}

// Tests that page infobars persist only on the tabs they are opened on, and
// that navigation in other tabs doesn't affect them.
- (void)testInfobarTabSwitch {
  const GURL destinationURL = web::test::HttpServer::MakeUrl(
      "http://ios/testing/data/http_server_files/destination.html");
  const GURL ponyURL = web::test::HttpServer::MakeUrl(
      "http://ios/testing/data/http_server_files/pony.html");
  web::test::SetUpFileBasedHttpServer();

  // Create the first tab and navigate to the test page.
  [ChromeEarlGrey loadURL:destinationURL];
  [ChromeEarlGrey waitForMainTabCount:1];

  // Create the second tab, navigate to the test page, and add the test infobar.
  chrome_test_util::OpenNewTab();
  [ChromeEarlGrey loadURL:ponyURL];
  [ChromeEarlGrey waitForMainTabCount:2];
  VerifyTestInfoBarVisibleForCurrentTab(false);
  GREYAssert(AddTestInfoBarToCurrentTab(),
             @"Failed to add infobar to second tab.");
  VerifyTestInfoBarVisibleForCurrentTab(true);

  // Switch back to the first tab and make sure no infobar is visible.
  chrome_test_util::SelectTabAtIndexInCurrentMode(0U);
  VerifyTestInfoBarVisibleForCurrentTab(false);

  // Navigate to a different URL in the first tab, to verify that this
  // navigation does not hide the infobar in the second tab.
  [ChromeEarlGrey loadURL:ponyURL];

  // Close the first tab.  Verify that there is only one tab remaining and its
  // infobar is visible.
  chrome_test_util::CloseCurrentTab();
  [ChromeEarlGrey waitForMainTabCount:1];
  VerifyTestInfoBarVisibleForCurrentTab(true);
}

@end
