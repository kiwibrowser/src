// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#import "ios/chrome/app/main_controller_private.h"
#include "ios/chrome/browser/chrome_switches.h"
#import "ios/chrome/browser/ui/authentication/signin_earlgrey_utils.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_egtest_util.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_mode.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_panel_cell.h"
#import "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#include "ios/chrome/test/earl_grey/accessibility_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity_service.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/http_server/blank_page_response_provider.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::ButtonWithAccessibilityLabel;
using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::StaticTextWithAccessibilityLabelId;
using chrome_test_util::TabletTabSwitcherCloseButton;
using chrome_test_util::TabletTabSwitcherCloseTabButton;
using chrome_test_util::TabletTabSwitcherIncognitoTabsPanelButton;
using chrome_test_util::TabletTabSwitcherNewIncognitoTabButton;
using chrome_test_util::TabletTabSwitcherNewTabButton;
using chrome_test_util::TabletTabSwitcherOpenButton;
using chrome_test_util::TabletTabSwitcherOpenTabsPanelButton;
using chrome_test_util::TabletTabSwitcherOtherDevicesPanelButton;
using web::test::HttpServer;

@interface TabSwitcherControllerTestCase : ChromeTestCase
@end

@implementation TabSwitcherControllerTestCase

// Checks that the tab switcher is not presented.
- (void)assertTabSwitcherIsInactive {
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  MainController* main_controller = chrome_test_util::GetMainController();
  GREYAssertTrue(![main_controller isTabSwitcherActive],
                 @"Tab Switcher should be inactive");
}

// Checks that the tab switcher is active.
- (void)assertTabSwitcherIsActive {
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
  MainController* main_controller = chrome_test_util::GetMainController();
  GREYAssertTrue([main_controller isTabSwitcherActive],
                 @"Tab Switcher should be active");
}

// Checks that the text associated with |messageId| is somewhere on screen.
- (void)assertMessageIsVisible:(int)messageId {
  id<GREYMatcher> messageMatcher =
      grey_allOf(StaticTextWithAccessibilityLabelId(messageId),
                 grey_sufficientlyVisible(), nil);
  [[EarlGrey selectElementWithMatcher:messageMatcher]
      assertWithMatcher:grey_notNil()];
}

// Checks that the text associated with |messageId| is not visible.
- (void)assertMessageIsNotVisible:(int)messageId {
  id<GREYMatcher> messageMatcher =
      grey_allOf(StaticTextWithAccessibilityLabelId(messageId),
                 grey_sufficientlyVisible(), nil);
  [[EarlGrey selectElementWithMatcher:messageMatcher]
      assertWithMatcher:grey_nil()];
}

// Tests entering and leaving the tab switcher.
- (void)testEnteringTabSwitcher {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  [self assertTabSwitcherIsInactive];

  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];

  // Check that the "No Open Tabs" message is not displayed.
  [self assertMessageIsNotVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_NON_INCOGNITO_TABS_TITLE];

  // Press the :: icon to exit the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsInactive];
}

// Tests entering tab switcher by closing all tabs, and leaving the tab switcher
// by creating a new tab.
- (void)testClosingAllTabsAndCreatingNewTab {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  [self assertTabSwitcherIsInactive];

  // Close the tab.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseTabButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsActive];

  // Check that the "No Open Tabs" message is displayed.
  [self assertMessageIsVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_NON_INCOGNITO_TABS_TITLE];

  // Create a new tab.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewTabButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsInactive];
}

// Tests entering tab switcher from incognito mode.
- (void)testIncognitoTabs {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  [self assertTabSwitcherIsInactive];

  // Create new incognito tab from tools menu.
  [ChromeEarlGreyUI openNewIncognitoTab];

  // Close the incognito tab and check that the we are entering the tab
  // switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseTabButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];

  // Check that the "No Incognito Tabs" message is shown.
  [self assertMessageIsVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_INCOGNITO_TABS_PROMO];

  // Create new incognito tab.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewIncognitoTabButton()]
      performAction:grey_tap()];

  // Verify that we've left the tab switcher.
  [self assertTabSwitcherIsInactive];

  // Close tab and verify we've entered the tab switcher again.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseTabButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];

  // Switch to the non incognito panel.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenTabsPanelButton()]
      performAction:grey_tap()];

  // Press the :: icon to exit the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];

  // Verify that we've left the tab switcher.
  [self assertTabSwitcherIsInactive];
}

// Tests leaving the tab switcher while on the "Other Devices" panel.
- (void)testLeavingSwitcherFromOtherDevices {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  [self assertTabSwitcherIsInactive];

  // Enter the tab switcher and press the "Other Devices" button.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];

  // Leave the tab switcher and verify that the normal BVC is shown.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsInactive];
  GREYAssertFalse(chrome_test_util::IsIncognitoMode(),
                  @"Expected to be in normal mode");

  // Open a new incognito tab and reopen the tab switcher.
  [ChromeEarlGreyUI openNewIncognitoTab];
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];

  // Leave the tab switcher and verify that the incognito BVC is shown.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsInactive];
  GREYAssertTrue(chrome_test_util::IsIncognitoMode(),
                 @"Expected to be in incognito mode");
}

// Tests that elements on iPad tab switcher are accessible.
// TODO: (crbug.com/691095) Open tabs label is not accessible
- (void)DISABLED_testAccessibilityOnTabSwitcher {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }
  [self assertTabSwitcherIsInactive];

  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];
  // Check that the "No Open Tabs" message is not displayed.
  [self assertMessageIsNotVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_NON_INCOGNITO_TABS_TITLE];

  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Press the :: icon to exit the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsInactive];
}

// Tests that elements on iPad tab switcher incognito tab are accessible.
// TODO: (crbug.com/691095) Incognito tabs label should be tappable.
- (void)DISABLED_testAccessibilityOnIncognitoTabSwitcher {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }
  [self assertTabSwitcherIsInactive];

  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];
  // Check that the "No Open Tabs" message is not displayed.
  [self assertMessageIsNotVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_NON_INCOGNITO_TABS_TITLE];

  // Press incognito tabs button.
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherIncognitoTabsPanelButton()]
      performAction:grey_tap()];

  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Press the :: icon to exit the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsInactive];
}

// Tests that elements on iPad tab switcher other devices are accessible.
// TODO: (crbug.com/691095) Other devices label should be tappable.
- (void)DISABLED_testAccessibilityOnOtherDeviceTabSwitcher {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }
  [self assertTabSwitcherIsInactive];

  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [self assertTabSwitcherIsActive];
  // Check that the "No Open Tabs" message is not displayed.
  [self assertMessageIsNotVisible:
            IDS_IOS_TAB_SWITCHER_NO_LOCAL_NON_INCOGNITO_TABS_TITLE];

  // Press other devices button.
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];

  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Create new incognito tab to exit the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewIncognitoTabButton()]
      performAction:grey_tap()];

  [self assertTabSwitcherIsInactive];
}

// Tests that closing a Tab that has a queued dialog successfully cancels the
// dialog.
- (void)testCloseTabWithDialog {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Load the blank test page so that JavaScript can be executed.
  const GURL kBlankPageURL = HttpServer::MakeUrl("http://blank-page");
  web::test::AddResponseProvider(
      web::test::CreateBlankPageResponseProvider(kBlankPageURL));
  [ChromeEarlGrey loadURL:kBlankPageURL];

  // Enter the tab switcher and show a dialog from the test page.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  NSString* const kCancelledMessageText = @"CANCELLED";
  NSString* const kAlertFormat = @"alert(\"%@\");";
  chrome_test_util::ExecuteJavaScript(
      [NSString stringWithFormat:kAlertFormat, kCancelledMessageText], nil);

  // Close the tab so that the queued dialog is cancelled.
  [[self class] closeAllTabs];

  // Open a new tab.  This will exit the stack view and will make the non-
  // incognito BrowserState active.  Attempt to present an alert with
  // kMessageText to verify that there aren't any alerts still in the queue.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherNewTabButton()]
      performAction:grey_tap()];
  [ChromeEarlGrey loadURL:kBlankPageURL];
  NSString* const kMessageText = @"MESSAGE";
  chrome_test_util::ExecuteJavaScript(
      [NSString stringWithFormat:kAlertFormat, kMessageText], nil);

  // Wait for an alert with kMessageText.  If it is shown, then the dialog using
  // kCancelledMessageText for the message was properly cancelled when the Tab
  // was closed.
  id<GREYMatcher> messageLabel =
      chrome_test_util::StaticTextWithAccessibilityLabel(kMessageText);
  ConditionBlock condition = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:messageLabel]
        assertWithMatcher:grey_notNil()
                    error:&error];
    return !error;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, condition),
             @"Alert with message was not found: %@", kMessageText);
}

// Tests sign-in promo view in cold state.
- (void)testColdSigninPromoView {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Enter the tab switcher and press the "Other Devices" button.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];
  // Check the sign-in promo view with cold state.
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeColdState
                          closeButton:NO];
}

// Tests sign-in promo view in warm state.
- (void)testWarmSigninPromoView {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Set up a fake identity.
  ChromeIdentity* identity = [SigninEarlGreyUtils fakeIdentity1];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      identity);

  // Enter the tab switcher and press the "Other Devices" button.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];
  // Check the sign-in promo view with warm state.
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState
                          closeButton:NO];

  // Tap the secondary button.
  [[EarlGrey
      selectElementWithMatcher:grey_allOf(
                                   chrome_test_util::PrimarySignInButton(),
                                   grey_sufficientlyVisible(), nil)]
      performAction:grey_tap()];
  // Tap the UNDO button.
  [[EarlGrey selectElementWithMatcher:grey_buttonTitle(@"UNDO")]
      performAction:grey_tap()];
  // Check the sign-in promo view with warm state.
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState
                          closeButton:NO];
}

// Tests to reload the other devices tab after sign-in.
// See crbug.comm/832527
- (void)testReloadOtherTabDevicesTab {
  if (GetTabSwitcherMode() != TabSwitcherMode::TABLET_SWITCHER) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Set up a fake identity.
  ChromeIdentity* identity = [SigninEarlGreyUtils fakeIdentity1];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      identity);

  // Enter the tab switcher and press the "Other Devices" tab.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];
  // Close the tab switcher.
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherCloseButton()]
      performAction:grey_tap()];

  // Open the settings to sign-in.
  [ChromeEarlGreyUI openSettingsMenu];
  [ChromeEarlGreyUI
      tapSettingsMenuButton:chrome_test_util::PrimarySignInButton()];
  [ChromeEarlGreyUI confirmSigninConfirmationDialog];
  [ChromeEarlGreyUI
      tapSettingsMenuButton:chrome_test_util::SettingsAccountButton()];
  // Sign-out.
  [ChromeEarlGreyUI
      tapSettingsMenuButton:chrome_test_util::SignOutAccountsButton()];
  [[EarlGrey selectElementWithMatcher:
                 ButtonWithAccessibilityLabelId(
                     IDS_IOS_DISCONNECT_DIALOG_CONTINUE_BUTTON_MOBILE)]
      performAction:grey_tap()];

  // Open the tab switcher to the "Other Devices" tab.
  [[EarlGrey selectElementWithMatcher:chrome_test_util::SettingsDoneButton()]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:TabletTabSwitcherOpenButton()]
      performAction:grey_tap()];
  [[EarlGrey
      selectElementWithMatcher:TabletTabSwitcherOtherDevicesPanelButton()]
      performAction:grey_tap()];

  // Check the sign-in promo view with warm state.
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState
                          closeButton:NO];
}

@end
