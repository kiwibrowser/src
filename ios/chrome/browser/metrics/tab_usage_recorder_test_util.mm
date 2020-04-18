// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/metrics/tab_usage_recorder_test_util.h"

#import <Foundation/Foundation.h>

#import "base/ios/block_types.h"
#import "ios/chrome/app/main_controller.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/main/browser_view_information.h"
#include "ios/chrome/browser/ui/tab_grid/tab_grid_egtest_util.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_egtest_util.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_mode.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/browser/web_state_list/web_state_list.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/testing/wait_util.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The delay to wait for an element to appear before tapping on it.
const NSTimeInterval kWaitElementTimeout = 3;

// Shows the tab switcher by tapping the switcher button.  Works on both phone
// and tablet.
void ShowTabSwitcher() {
  id<GREYMatcher> matcher = nil;
  switch (GetTabSwitcherMode()) {
    case TabSwitcherMode::STACK:
      matcher = chrome_test_util::ShowTabsButton();
      break;
    case TabSwitcherMode::TABLET_SWITCHER:
      matcher = chrome_test_util::TabletTabSwitcherOpenButton();
      break;
    case TabSwitcherMode::GRID:
      matcher = chrome_test_util::TabGridOpenButton();
      break;
  }
  DCHECK(matcher);

  // Perform a tap with a timeout. Occasionally EG doesn't sync up properly to
  // the animations of tab switcher, so it is necessary to poll here.
  GREYCondition* tapTabSwitcher =
      [GREYCondition conditionWithName:@"Tap tab switcher button"
                                 block:^BOOL {
                                   NSError* error;
                                   [[EarlGrey selectElementWithMatcher:matcher]
                                       performAction:grey_tap()
                                               error:&error];
                                   return error == nil;
                                 }];

  // Wait until 2 seconds for the tap.
  BOOL hasClicked = [tapTabSwitcher waitWithTimeout:2];
  GREYAssertTrue(hasClicked, @"Tab switcher could not be tapped.");
}

// Closes the tabs switcher.
void CloseTabSwitcher() {
  id<GREYMatcher> matcher = nil;
  switch (GetTabSwitcherMode()) {
    case TabSwitcherMode::STACK:
      matcher = chrome_test_util::ShowTabsButton();
      break;
    case TabSwitcherMode::TABLET_SWITCHER:
      matcher = chrome_test_util::TabletTabSwitcherCloseButton();
      break;
    case TabSwitcherMode::GRID:
      matcher = chrome_test_util::TabGridDoneButton();
      break;
  }
  DCHECK(matcher);

  [[EarlGrey selectElementWithMatcher:matcher] performAction:grey_tap()];
}

}  // namespace

namespace tab_usage_recorder_test_util {

void OpenNewIncognitoTabUsingUIAndEvictMainTabs() {
  int nb_incognito_tab = chrome_test_util::GetIncognitoTabCount();
  [ChromeEarlGreyUI openToolsMenu];
  id<GREYMatcher> new_incognito_tab_button_matcher =
      grey_accessibilityID(kToolsMenuNewIncognitoTabId);
  [[EarlGrey selectElementWithMatcher:new_incognito_tab_button_matcher]
      performAction:grey_tap()];
  [ChromeEarlGrey waitForIncognitoTabCount:(nb_incognito_tab + 1)];
  ConditionBlock condition = ^bool {
    return chrome_test_util::IsIncognitoMode();
  };
  GREYAssert(
      testing::WaitUntilConditionOrTimeout(kWaitElementTimeout, condition),
      @"Waiting switch to incognito mode.");
  chrome_test_util::EvictOtherTabModelTabs();
}

void SwitchToNormalMode() {
  GREYAssertTrue(chrome_test_util::IsIncognitoMode(),
                 @"Switching to normal mode is only allowed from Incognito.");

  // Enter the tab switcher to switch modes.
  ShowTabSwitcher();

  // Switch modes and exit the tab switcher.
  switch (GetTabSwitcherMode()) {
    case TabSwitcherMode::STACK:
      [[EarlGrey selectElementWithMatcher:
                     chrome_test_util::ButtonWithAccessibilityLabelId(
                         IDS_IOS_TOOLS_MENU_NEW_INCOGNITO_TAB)]
          performAction:grey_swipeSlowInDirection(kGREYDirectionRight)];
      CloseTabSwitcher();
      break;
    case TabSwitcherMode::TABLET_SWITCHER:
      [[EarlGrey selectElementWithMatcher:
                     chrome_test_util::TabletTabSwitcherOpenTabsPanelButton()]
          performAction:grey_tap()];
      CloseTabSwitcher();
      break;
    case TabSwitcherMode::GRID:
      TabModel* model = [[chrome_test_util::GetMainController()
          browserViewInformation] mainTabModel];
      const int tab_index = model.webStateList->active_index();
      [[EarlGrey selectElementWithMatcher:chrome_test_util::
                                              TabGridOpenTabsPanelButton()]
          performAction:grey_tap()];
      [[EarlGrey selectElementWithMatcher:chrome_test_util::TabGridCellAtIndex(
                                              tab_index)]
          performAction:grey_tap()];
      break;
  }

  // Turn off synchronization of GREYAssert to test the pending states.
  [[GREYConfiguration sharedInstance]
          setValue:@(NO)
      forConfigKey:kGREYConfigKeySynchronizationEnabled];
  ConditionBlock condition = ^bool {
    return !chrome_test_util::IsIncognitoMode();
  };
  GREYAssert(
      testing::WaitUntilConditionOrTimeout(kWaitElementTimeout, condition),
      @"Waiting switch to normal mode.");

  [[GREYConfiguration sharedInstance]
          setValue:@(YES)
      forConfigKey:kGREYConfigKeySynchronizationEnabled];
}

}  // namespace tab_usage_recorder_test_util
