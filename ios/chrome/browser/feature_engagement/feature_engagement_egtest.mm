// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#include "base/strings/sys_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/feature_engagement/public/event_constants.h"
#include "components/feature_engagement/public/feature_constants.h"
#include "components/feature_engagement/public/tracker.h"
#include "components/feature_engagement/test/test_tracker.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/feature_engagement/tracker_factory.h"
#import "ios/chrome/browser/ui/popup_menu/popup_menu_constants.h"
#import "ios/chrome/browser/ui/tab_grid/tab_grid_egtest_util.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_egtest_util.h"
#import "ios/chrome/browser/ui/tab_switcher/tab_switcher_mode.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/wait_util.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// The minimum number of times Chrome must be opened in order for the Reading
// List Badge to be shown.
const int kMinChromeOpensRequiredForReadingList = 5;

// The minimum number of times Chrome must be opened in order for the New Tab
// Tip to be shown.
const int kMinChromeOpensRequiredForNewTabTip = 3;

// Matcher for the Reading List Text Badge.
id<GREYMatcher> ReadingListTextBadge() {
  return grey_allOf(
      grey_accessibilityID(@"kToolsMenuTextBadgeAccessibilityIdentifier"),
      grey_ancestor(grey_allOf(grey_accessibilityID(kToolsMenuReadingListId),
                               grey_sufficientlyVisible(), nil)),
      nil);
}

// Matcher for the New Tab Tip Bubble.
id<GREYMatcher> NewTabTipBubble() {
  return grey_accessibilityLabel(
      l10n_util::GetNSStringWithFixup(IDS_IOS_NEW_TAB_IPH_PROMOTION_TEXT));
}

// Opens and closes the tab switcher.
void OpenAndCloseTabSwitcher() {
  id<GREYMatcher> openTabSwitcherMatcher =
      IsIPadIdiom() ? chrome_test_util::TabletTabSwitcherOpenButton()
                    : chrome_test_util::ShowTabsButton();
  [[EarlGrey selectElementWithMatcher:openTabSwitcherMatcher]
      performAction:grey_tap()];

  switch (GetTabSwitcherMode()) {
    case TabSwitcherMode::GRID:
      [[EarlGrey selectElementWithMatcher:chrome_test_util::TabGridDoneButton()]
          performAction:grey_tap()];
      break;
    case TabSwitcherMode::TABLET_SWITCHER:
    case TabSwitcherMode::STACK:
      id<GREYMatcher> closeTabSwitcherMatcher =
          IsIPadIdiom() ? chrome_test_util::TabletTabSwitcherCloseButton()
                        : chrome_test_util::ShowTabsButton();
      [[EarlGrey selectElementWithMatcher:closeTabSwitcherMatcher]
          performAction:grey_tap()];
      break;
  }
}

// Create a test FeatureEngagementTracker.
std::unique_ptr<KeyedService> CreateTestFeatureEngagementTracker(
    web::BrowserState* context) {
  return feature_engagement::CreateTestTracker();
}

// Simulate a Chrome Opened event for the Feature Engagement Tracker.
void SimulateChromeOpenedEvent() {
  feature_engagement::TrackerFactory::GetForBrowserState(
      chrome_test_util::GetOriginalBrowserState())
      ->NotifyEvent(feature_engagement::events::kChromeOpened);
}

// Loads the FeatureEngagementTracker.
void LoadFeatureEngagementTracker() {
  ios::ChromeBrowserState* browserState =
      chrome_test_util::GetOriginalBrowserState();

  feature_engagement::TrackerFactory::GetInstance()->SetTestingFactory(
      browserState, CreateTestFeatureEngagementTracker);
}

// Enables the Badged Reading List help to be triggered for |feature_list|.
void EnableBadgedReadingListTriggering(
    base::test::ScopedFeatureList& feature_list) {
  std::map<std::string, std::string> badged_reading_list_params;

  badged_reading_list_params["event_1"] =
      "name:chrome_opened;comparator:>=5;window:90;storage:90";
  badged_reading_list_params["event_trigger"] =
      "name:badged_reading_list_trigger;comparator:==0;window:1095;storage:"
      "1095";
  badged_reading_list_params["event_used"] =
      "name:viewed_reading_list;comparator:==0;window:90;storage:90";
  badged_reading_list_params["session_rate"] = "==0";
  badged_reading_list_params["availability"] = "any";

  feature_list.InitAndEnableFeatureWithParameters(
      feature_engagement::kIPHBadgedReadingListFeature,
      badged_reading_list_params);
}

// Enables the New Tab Tip to be triggered for |feature_list|.
void EnableNewTabTipTriggering(base::test::ScopedFeatureList& feature_list) {
  std::map<std::string, std::string> new_tab_tip_params;

  new_tab_tip_params["event_1"] =
      "name:chrome_opened;comparator:>=3;window:90;storage:90";
  new_tab_tip_params["event_trigger"] =
      "name:new_tab_tip_trigger;comparator:<2;window:1095;storage:"
      "1095";
  new_tab_tip_params["event_used"] =
      "name:new_tab_opened;comparator:==0;window:90;storage:90";
  new_tab_tip_params["session_rate"] = "==0";
  new_tab_tip_params["availability"] = "any";

  feature_list.InitAndEnableFeatureWithParameters(
      feature_engagement::kIPHNewTabTipFeature, new_tab_tip_params);
}

}  // namespace

// Tests related to the triggering of In Product Help features.
@interface FeatureEngagementTestCase : ChromeTestCase
@end

@implementation FeatureEngagementTestCase

// Verifies that the Badged Reading List feature shows when triggering
// conditions are met. Also verifies that the Badged Reading List does not
// appear again after being shown.
- (void)testBadgedReadingListFeatureShouldShow {
  base::test::ScopedFeatureList scoped_feature_list;

  EnableBadgedReadingListTriggering(scoped_feature_list);

  // Ensure that the FeatureEngagementTracker picks up the new feature
  // configuration provided by |scoped_feature_list|.
  LoadFeatureEngagementTracker();

  // Ensure that Chrome has been launched enough times for the Badged Reading
  // List to appear.
  for (int index = 0; index < kMinChromeOpensRequiredForReadingList; index++) {
    SimulateChromeOpenedEvent();
  }

  [ChromeEarlGreyUI openToolsMenu];

  [[[EarlGrey selectElementWithMatcher:ReadingListTextBadge()]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown, 150)
      onElementWithMatcher:grey_accessibilityID(kPopupMenuToolsMenuTableViewId)]
      assertWithMatcher:grey_notNil()];

  // Close tools menu by tapping reload.
  if (IsUIRefreshPhase1Enabled()) {
    [[[EarlGrey
        selectElementWithMatcher:grey_allOf(
                                     chrome_test_util::ReloadButton(),
                                     grey_ancestor(grey_accessibilityID(
                                         kPopupMenuToolsMenuTableViewId)),
                                     nil)]
           usingSearchAction:grey_scrollInDirection(kGREYDirectionUp, 150)
        onElementWithMatcher:grey_accessibilityID(
                                 kPopupMenuToolsMenuTableViewId)]
        performAction:grey_tap()];
  } else {
    [[[EarlGrey selectElementWithMatcher:chrome_test_util::ReloadButton()]
           usingSearchAction:grey_scrollInDirection(kGREYDirectionUp, 150)
        onElementWithMatcher:grey_accessibilityID(
                                 kPopupMenuToolsMenuTableViewId)]
        performAction:grey_tap()];
  }

  // Reopen tools menu to verify that the badge does not appear again.
  [ChromeEarlGreyUI openToolsMenu];
  // Make sure the ReadingList entry is visible.
  [[[EarlGrey
      selectElementWithMatcher:grey_allOf(grey_accessibilityID(
                                              kToolsMenuReadingListId),
                                          grey_sufficientlyVisible(), nil)]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown, 150)
      onElementWithMatcher:grey_accessibilityID(kPopupMenuToolsMenuTableViewId)]
      assertWithMatcher:grey_notNil()];

  [[EarlGrey selectElementWithMatcher:ReadingListTextBadge()]
      assertWithMatcher:grey_notVisible()];
}

// Verifies that the Badged Reading List feature does not show if Chrome has
// not opened enough times.
- (void)testBadgedReadingListFeatureTooFewChromeOpens {
  base::test::ScopedFeatureList scoped_feature_list;

  EnableBadgedReadingListTriggering(scoped_feature_list);

  // Ensure that the FeatureEngagementTracker picks up the new feature
  // configuration provided by |scoped_feature_list|.
  LoadFeatureEngagementTracker();

  // Open Chrome just one time.
  SimulateChromeOpenedEvent();

  [ChromeEarlGreyUI openToolsMenu];

  [[EarlGrey selectElementWithMatcher:ReadingListTextBadge()]
      assertWithMatcher:grey_notVisible()];
}

// Verifies that the Badged Reading List feature does not show if the reading
// list has already been used.
- (void)testBadgedReadingListFeatureReadingListAlreadyUsed {
  base::test::ScopedFeatureList scoped_feature_list;

  EnableBadgedReadingListTriggering(scoped_feature_list);

  // Ensure that the FeatureEngagementTracker picks up the new feature
  // configuration provided by |scoped_feature_list|.
  LoadFeatureEngagementTracker();

  // Ensure that Chrome has been launched enough times to meet the trigger
  // condition.
  for (int index = 0; index < kMinChromeOpensRequiredForReadingList; index++) {
    SimulateChromeOpenedEvent();
  }

  [chrome_test_util::BrowserCommandDispatcherForMainBVC() showReadingList];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(@"Done")]
      performAction:grey_tap()];

  [ChromeEarlGreyUI openToolsMenu];

  [[EarlGrey selectElementWithMatcher:ReadingListTextBadge()]
      assertWithMatcher:grey_notVisible()];
}

// Verifies that the New Tab Tip appears when all conditions are met.
- (void)testNewTabTipPromoShouldShow {
  base::test::ScopedFeatureList scoped_feature_list;

  EnableNewTabTipTriggering(scoped_feature_list);

  // Ensure that the FeatureEngagementTracker picks up the new feature
  // configuration provided by |scoped_feature_list|.
  LoadFeatureEngagementTracker();

  // Ensure that Chrome has been launched enough times to meet the trigger
  // condition.
  for (int index = 0; index < kMinChromeOpensRequiredForNewTabTip; index++) {
    SimulateChromeOpenedEvent();
  }

  // Navigate to a page other than the NTP to allow for the New Tab Tip to
  // appear.
  [ChromeEarlGrey loadURL:GURL("chrome://version")];

  // Open and close the tab switcher to trigger the New Tab tip.
  OpenAndCloseTabSwitcher();

  // Verify that the New Tab Tip appeared.
  [[EarlGrey selectElementWithMatcher:NewTabTipBubble()]
      assertWithMatcher:grey_sufficientlyVisible()];
}

// Verifies that the New Tab Tip does not appear if all conditions are met,
// but the NTP is open.
- (void)testNewTabTipPromoDoesNotAppearOnNTP {
  base::test::ScopedFeatureList scoped_feature_list;

  EnableNewTabTipTriggering(scoped_feature_list);

  // Ensure that the FeatureEngagementTracker picks up the new feature
  // configuration provided by |scoped_feature_list|.
  LoadFeatureEngagementTracker();

  // Ensure that Chrome has been launched enough times to meet the trigger
  // condition.
  for (int index = 0; index < kMinChromeOpensRequiredForNewTabTip; index++) {
    SimulateChromeOpenedEvent();
  }

  // Open and close the tab switcher to potentially trigger the New Tab Tip.
  OpenAndCloseTabSwitcher();

  // Verify that the New Tab Tip did not appear.
  [[EarlGrey selectElementWithMatcher:NewTabTipBubble()]
      assertWithMatcher:grey_notVisible()];
}

@end
