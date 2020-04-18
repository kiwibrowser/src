// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <XCTest/XCTest.h>

#include "base/ios/block_types.h"
#include "base/mac/foundation_util.h"
#include "base/strings/sys_string_conversions.h"
#import "base/test/ios/wait_util.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/pref_names.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/browser_view_controller.h"
#import "ios/chrome/browser/ui/stack_view/card_view.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller.h"
#import "ios/chrome/browser/ui/stack_view/stack_view_controller_private.h"
#include "ios/chrome/browser/ui/tab_switcher/tab_switcher_mode.h"
#import "ios/chrome/browser/ui/toolbar/buttons/toolbar_constants.h"
#import "ios/chrome/browser/ui/toolbar/legacy/toolbar_controller_constants.h"
#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/stack_view_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/http_server/blank_page_response_provider.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using web::test::HttpServer;

namespace {

// Returns a GREYMatcher that matches |view|.
// TODO(crbug.com/642619): Evaluate whether this should be shared code.
id<GREYMatcher> ViewMatchingView(UIView* view) {
  MatchesBlock matches = ^BOOL(UIView* viewToMatch) {
    return viewToMatch == view;
  };
  DescribeToBlock describe = ^void(id<GREYDescription> description) {
    NSString* matcherDescription =
        [NSString stringWithFormat:@"View matching %@", view];
    [description appendText:matcherDescription];
  };
  return [[GREYElementMatcherBlock alloc] initWithMatchesBlock:matches
                                              descriptionBlock:describe];
}

// Returns a matcher for the StackViewController's view.
id<GREYMatcher> StackView() {
  return ViewMatchingView([chrome_test_util::GetStackViewController() view]);
}

// Waits for the Stack View to be active/inactive.
void WaitForStackViewActive(bool active) {
  NSString* activeStatusString = active ? @"active" : @"inactive";
  NSString* activeTabSwitcherDescription =
      [NSString stringWithFormat:@"Waiting for tab switcher to be %@.",
                                 activeStatusString];
  BOOL (^activeTabSwitcherBlock)
  () = ^BOOL {
    BOOL isActive = chrome_test_util::GetStackViewController() &&
                    chrome_test_util::IsTabSwitcherActive();
    return active ? isActive : !isActive;
  };
  GREYCondition* activeTabSwitcherCondition =
      [GREYCondition conditionWithName:activeTabSwitcherDescription
                                 block:activeTabSwitcherBlock];
  NSString* assertDescription = [NSString
      stringWithFormat:@"Tab switcher did not become %@.", activeStatusString];

  GREYAssert([activeTabSwitcherCondition
                 waitWithTimeout:testing::kWaitForUIElementTimeout],
             assertDescription);
}

// Verify the visibility of the stack view.
void CheckForStackViewVisibility(bool visible) {
  id<GREYMatcher> visibilityMatcher =
      grey_allOf(visible ? grey_sufficientlyVisible() : grey_notVisible(),
                 visible ? grey_notNil() : grey_nil(), nil);
  [[EarlGrey selectElementWithMatcher:StackView()]
      assertWithMatcher:visibilityMatcher];
}

// Opens the StackViewController.
void OpenStackView() {
  if (chrome_test_util::IsTabSwitcherActive())
    return;
  // Tap on the toolbar's tab switcher button.
  id<GREYMatcher> stackButtonMatcher =
      grey_allOf(grey_accessibilityID(kToolbarStackButtonIdentifier),
                 grey_sufficientlyVisible(), nil);
  [[EarlGrey selectElementWithMatcher:stackButtonMatcher]
      performAction:grey_tap()];
  // Verify that a StackViewController was presented.
  WaitForStackViewActive(true);
  CheckForStackViewVisibility(true);
}

// Shows either the normal or incognito deck.
enum class DeckType : bool { NORMAL, INCOGNITO };
void ShowDeckWithType(DeckType type) {
  OpenStackView();
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  UIView* activeDisplayView = stackViewController.activeCardSet.displayView;
  // The inactive deck region is in scroll view coordinates, but the tap
  // recognizer is installed on the active card set's display view.
  CGRect inactiveDeckRegion =
      [activeDisplayView convertRect:[stackViewController inactiveDeckRegion]
                            fromView:stackViewController.scrollView];
  bool showIncognito = type == DeckType::INCOGNITO;
  if (showIncognito) {
    GREYAssert(!CGRectIsEmpty(inactiveDeckRegion),
               @"Cannot show Incognito deck if no Incognito tabs are open");
  }
  if (showIncognito != [stackViewController isCurrentSetIncognito]) {
    CGPoint tapPoint = CGPointMake(CGRectGetMidX(inactiveDeckRegion),
                                   CGRectGetMidY(inactiveDeckRegion));
    [[EarlGrey selectElementWithMatcher:ViewMatchingView(activeDisplayView)]
        performAction:grey_tapAtPoint(tapPoint)];
  }
}

// Opens a new tab using the stack view button.
void OpenNewTabUsingStackView() {
  // Open the stack view, tap the New Tab button, and wait for the animation to
  // finish.
  ShowDeckWithType(DeckType::NORMAL);
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(@"New Tab")]
      performAction:grey_tap()];
  WaitForStackViewActive(false);
  CheckForStackViewVisibility(false);
}

// Opens the tools menu from the stack view.
void OpenToolsMenu() {
  OpenStackView();
  [[EarlGrey selectElementWithMatcher:chrome_test_util::ToolsMenuButton()]
      performAction:grey_tap()];
}

// Opens a new Incognito Tab using the stack view button.
void OpenNewIncognitoTabUsingStackView() {
  OpenToolsMenu();
  NSString* newIncognitoTabID = kToolsMenuNewIncognitoTabId;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(newIncognitoTabID)]
      performAction:grey_tap()];
  WaitForStackViewActive(false);
  CheckForStackViewVisibility(false);
}

// Taps the CardView associated with |tab|.
void SelectTabUsingStackView(Tab* tab) {
  DCHECK(tab);
  // Present the StackViewController.
  OpenStackView();
  // Get the StackCard associated with |tab|.
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  StackCard* nextCard = [[stackViewController activeCardSet] cardForTab:tab];
  UIView* card_title_label = static_cast<UIView*>([[nextCard view] titleLabel]);
  [[EarlGrey selectElementWithMatcher:ViewMatchingView(card_title_label)]
      performAction:grey_tap()];
  // Wait for the StackViewController to be dismissed.
  WaitForStackViewActive(false);
  CheckForStackViewVisibility(false);
  // Checks that the next Tab has been selected.
  GREYAssertEqual(tab, chrome_test_util::GetCurrentTab(),
                  @"The next Tab was not selected");
}
}

// Tests for interacting with the StackViewController.
@interface StackViewTestCase : ChromeTestCase
@end

@implementation StackViewTestCase

// Switches between three Tabs via the stack view.
- (void)testSwitchTabs {
  if (GetTabSwitcherMode() != TabSwitcherMode::STACK) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Open two additional Tabs.
  const NSUInteger kAdditionalTabCount = 2;
  for (NSUInteger i = 0; i < kAdditionalTabCount; ++i)
    OpenNewTabUsingStackView();
  // Select each additional Tab using the stack view UI.
  for (NSUInteger i = 0; i < kAdditionalTabCount + 1; ++i)
    SelectTabUsingStackView(chrome_test_util::GetNextTab());
}

// Tests closing a tab in the stack view.
- (void)testCloseTab {
  if (GetTabSwitcherMode() != TabSwitcherMode::STACK) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Open the stack view and tap the close button on the current CardView.
  OpenStackView();
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  Tab* currentTab = chrome_test_util::GetCurrentTab();
  StackCard* card = [[stackViewController activeCardSet] cardForTab:currentTab];
  CardView* cardView = card.view;
  NSString* identifier = card.view.closeButtonId;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(identifier)]
      performAction:grey_tap()];
  // Verify that the CardView and its associated Tab were removed.
  [[EarlGrey selectElementWithMatcher:ViewMatchingView(cardView)]
      assertWithMatcher:grey_notVisible()];
  GREYAssertEqual(chrome_test_util::GetMainTabCount(), 0,
                  @"All Tabs should be closed.");
}

// Tests closing all Tabs in the stack view.
- (void)testCloseAllTabs {
  if (GetTabSwitcherMode() != TabSwitcherMode::STACK) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Open an incognito Tab.
  OpenNewIncognitoTabUsingStackView();
  GREYAssertEqual(chrome_test_util::GetIncognitoTabCount(), 1,
                  @"Incognito Tab was not opened.");
  // Open two additional Tabs.
  const NSUInteger kAdditionalTabCount = 2;
  for (NSUInteger i = 0; i < kAdditionalTabCount; ++i)
    OpenNewTabUsingStackView();
  GREYAssertEqual(chrome_test_util::GetMainTabCount(), kAdditionalTabCount + 1,
                  @"Additional Tabs were not opened.");
  // Record all created CardViews.
  OpenStackView();
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  NSMutableArray* cardViews = [NSMutableArray array];
  for (StackCard* card in [stackViewController activeCardSet].cards) {
    if (card.viewIsLive)
      [cardViews addObject:card.view];
  }
  for (StackCard* card in [stackViewController inactiveCardSet].cards) {
    if (card.viewIsLive)
      [cardViews addObject:card.view];
  }
  // Open the tools menu and select "Close all tabs".
  OpenToolsMenu();
  NSString* closeAllTabsID = kToolsMenuCloseAllTabsId;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(closeAllTabsID)]
      performAction:grey_tap()];
  // Wait for CardViews to be dismissed.
  for (CardView* cardView in cardViews) {
    [[EarlGrey selectElementWithMatcher:ViewMatchingView(cardView)]
        assertWithMatcher:grey_notVisible()];
  }
  // Check that all Tabs were closed.
  GREYAssertEqual(chrome_test_util::GetMainTabCount(), 0,
                  @"Tabs were not closed.");
  GREYAssertEqual(chrome_test_util::GetIncognitoTabCount(), 0,
                  @"Incognito Tab was not closed.");
}

// Tests that tapping on the inactive deck region switches modes.
- (void)testSwitchingModes {
  if (GetTabSwitcherMode() != TabSwitcherMode::STACK) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Open an Incognito Tab then switch decks.
  OpenNewIncognitoTabUsingStackView();
  ShowDeckWithType(DeckType::INCOGNITO);
  // Verify that the current CardSet is the incognito set.
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  GREYAssert([stackViewController isCurrentSetIncognito],
             @"Incognito deck not selected.");
  // Switch back to the main CardSet and verify that is selected.
  ShowDeckWithType(DeckType::NORMAL);
  GREYAssert(![stackViewController isCurrentSetIncognito],
             @"Normal deck not selected.");
}

// Tests that closing a Tab that has a queued dialog successfully cancels the
// dialog.
- (void)testCloseTabWithDialog {
  if (GetTabSwitcherMode() != TabSwitcherMode::STACK) {
    EARL_GREY_TEST_SKIPPED(
        @"TabSwitcher tests are not applicable in this configuration");
  }

  // Load the blank test page so that JavaScript can be executed.
  const GURL kBlankPageURL = HttpServer::MakeUrl("http://blank-page");
  web::test::AddResponseProvider(
      web::test::CreateBlankPageResponseProvider(kBlankPageURL));
  [ChromeEarlGrey loadURL:kBlankPageURL];

  // Enter stack view and show a dialog from the test page.
  OpenStackView();
  NSString* const kCancelledMessageText = @"CANCELLED";
  NSString* const kAlertFormat = @"alert(\"%@\");";
  chrome_test_util::ExecuteJavaScript(
      [NSString stringWithFormat:kAlertFormat, kCancelledMessageText], nil);

  // Close the Tab that is attempting to display a dialog.
  StackViewController* stackViewController =
      chrome_test_util::GetStackViewController();
  Tab* currentTab = chrome_test_util::GetCurrentTab();
  StackCard* card = [[stackViewController activeCardSet] cardForTab:currentTab];
  NSString* identifier = card.view.closeButtonId;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(identifier)]
      performAction:grey_tap()];

  // Open a new tab.  This will exit the stack view and will make the non-
  // incognito BrowserState active.  Attempt to present an alert with
  // kMessageText to verify that there aren't any alerts still in the queue.
  OpenNewTabUsingStackView();
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

@end
