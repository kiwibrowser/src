// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#import "ios/chrome/browser/ui/uikit_ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/earl_grey/accessibility_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const char kPDFURL[] = "http://ios/testing/data/http_server_files/testpage.pdf";

// Matcher for the button to find in page.
id<GREYMatcher> FindInPageButton() {
  return grey_accessibilityID(kToolsMenuFindInPageId);
}
}  // namespace

// Tests for the tools popup menu.
@interface ToolsPopupMenuTestCase : ChromeTestCase
@end

@implementation ToolsPopupMenuTestCase

// Tests that the menu is closed when tapping the close button or the scrim.
- (void)testOpenAndCloseToolsMenu {
  [ChromeEarlGreyUI openToolsMenu];

  // When the UIRefresh flag is enabled, a scrim covers the whole window and
  // tapping on this scrim dismisses the tools menu.  The "Tools Menu" button
  // happens to be outside of the bounds of the menu and is a convenient place
  // to tap to activate the scrim.
  //
  // When the UIRefresh flag is disabled, a scrim is present on phone with the
  // |IDS_IOS_TOOLBAR_CLOSE_MENU| accessibility label.  On tablet, the tools
  // menu button is an item in the tools menu and can be pressed again to close
  // the menu.
  if (!IsCompactWidth() && !IsUIRefreshPhase1Enabled()) {
    [[EarlGrey
        selectElementWithMatcher:grey_accessibilityLabel(l10n_util::GetNSString(
                                     IDS_IOS_TOOLBAR_CLOSE_MENU))]
        performAction:grey_tap()];
  } else {
    [[EarlGrey selectElementWithMatcher:chrome_test_util::ToolsMenuButton()]
        performAction:grey_tap()];
  }

  [[EarlGrey selectElementWithMatcher:chrome_test_util::ToolsMenuView()]
      assertWithMatcher:grey_notVisible()];
}

// Navigates to a pdf page and verifies that the "Find in Page..." tool
// is not enabled
- (void)testNoSearchForPDF {
  web::test::SetUpFileBasedHttpServer();
  const GURL URL = web::test::HttpServer::MakeUrl(kPDFURL);

  // Navigate to a mock pdf and verify that the find button is disabled.
  [ChromeEarlGrey loadURL:URL];
  [ChromeEarlGreyUI openToolsMenu];
  [[EarlGrey selectElementWithMatcher:FindInPageButton()]
      assertWithMatcher:grey_accessibilityTrait(
                            UIAccessibilityTraitNotEnabled)];
}

// Open tools menu and verify elements are accessible.
- (void)testAccessibilityOnToolsMenu {
  [ChromeEarlGreyUI openToolsMenu];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();
  // Close Tools menu.
  [ChromeTestCase removeAnyOpenMenusAndInfoBars];
}

@end
