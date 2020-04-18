// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <XCTest/XCTest.h>

#include "ios/chrome/browser/ui/tools_menu/public/tools_menu_constants.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/web_view_interaction_test_util.h"
#include "ios/chrome/test/earl_grey/accessibility_util.h"
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

using chrome_test_util::ButtonWithAccessibilityLabel;
using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::NavigationBarDoneButton;
using chrome_test_util::SettingsDoneButton;
using chrome_test_util::SettingsMenuBackButton;
using chrome_test_util::TapWebViewElementWithId;

namespace {

// Expectation of how the saved autofill profile looks like, a map from cell
// name IDs to expected contents.
struct DisplayStringIDToExpectedResult {
  int display_string_id;
  NSString* expected_result;
};

const DisplayStringIDToExpectedResult kExpectedFields[] = {
    {IDS_IOS_AUTOFILL_FULLNAME, @"George Washington"},
    {IDS_IOS_AUTOFILL_COMPANY_NAME, @""},
    {IDS_IOS_AUTOFILL_ADDRESS1, @"1600 Pennsylvania Ave NW"},
    {IDS_IOS_AUTOFILL_ADDRESS2, @""},
    {IDS_IOS_AUTOFILL_CITY, @"Washington"},
    {IDS_IOS_AUTOFILL_STATE, @"DC"},
    {IDS_IOS_AUTOFILL_ZIP, @"20500"},
    {IDS_IOS_AUTOFILL_PHONE, @""},
    {IDS_IOS_AUTOFILL_EMAIL, @""}};

// Expectation of how user-typed country names should be canonicalized.
struct UserTypedCountryExpectedResultPair {
  NSString* user_typed_country;
  NSString* expected_result;
};

const UserTypedCountryExpectedResultPair kCountryTests[] = {
    {@"Brasil", @"Brazil"},
    {@"China", @"China"},
    {@"DEUTSCHLAND", @"Germany"},
    {@"GREAT BRITAIN", @"United Kingdom"},
    {@"IN", @"India"},
    {@"JaPaN", @"Japan"},
    {@"JP", @"Japan"},
    {@"Nigeria", @"Nigeria"},
    {@"TW", @"Taiwan"},
    {@"U.S.A.", @"United States"},
    {@"UK", @"United Kingdom"},
    {@"USA", @"United States"},
    {@"Nonexistia", @""},
};

// Given a resource ID of a category of an address profile, it returns a
// NSString consisting of the resource string concatenated with "_textField".
// This is the a11y ID of the text field corresponding to the category in the
// edit dialog of the address profile.
NSString* GetTextFieldForID(int categoryId) {
  return [NSString
      stringWithFormat:@"%@_textField", l10n_util::GetNSString(categoryId)];
}

}  // namespace

// Various tests for the Autofill section of the settings.
@interface AutofillSettingsTestCase : ChromeTestCase
@end

@implementation AutofillSettingsTestCase

// Helper to load a page with an address form and submit it.
- (void)loadAndSubmitTheForm {
  web::test::SetUpFileBasedHttpServer();
  const GURL URL = web::test::HttpServer::MakeUrl(
      "http://ios/testing/data/http_server_files/autofill_smoke_test.html");

  [ChromeEarlGrey loadURL:URL];

  // Autofill one of the forms.
  GREYAssert(TapWebViewElementWithId("fill_profile_president"),
             @"Failed to tap \"fill_profile_president\"");
  GREYAssert(TapWebViewElementWithId("submit_profile"),
             @"Failed to tap \"submit_profile\"");
}

// Helper to open the settings page for the record with |address|.
- (void)openEditAddress:(NSString*)address {
  [ChromeEarlGreyUI openSettingsMenu];
  NSString* label = l10n_util::GetNSString(IDS_IOS_AUTOFILL);
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabel(label)]
      performAction:grey_tap()];

  NSString* cellLabel = address;
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(cellLabel)]
      performAction:grey_tap()];
}

// Close the settings.
- (void)exitSettingsMenu {
  [[EarlGrey selectElementWithMatcher:SettingsMenuBackButton()]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:SettingsMenuBackButton()]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:SettingsDoneButton()]
      performAction:grey_tap()];
  // Wait for UI components to finish loading.
  [[GREYUIThreadExecutor sharedInstance] drainUntilIdle];
}

// Test that submitting a form ensures saving the data as an autofill profile.
- (void)testAutofillProfileSaving {
  [self loadAndSubmitTheForm];
  [self openEditAddress:@"George Washington, 1600 Pennsylvania Ave NW"];

  // Check that all fields and values match the expectations.
  for (const DisplayStringIDToExpectedResult& expectation : kExpectedFields) {
    [[EarlGrey
        selectElementWithMatcher:
            grey_accessibilityLabel([NSString
                stringWithFormat:@"%@, %@", l10n_util::GetNSString(
                                                expectation.display_string_id),
                                 expectation.expected_result])]
        assertWithMatcher:grey_notNil()];
  }

  [self exitSettingsMenu];
}

// Test that editing country names is followed by validating the value and
// replacing it with a canonical one.
- (void)testAutofillProfileEditing {
  [self loadAndSubmitTheForm];
  [self openEditAddress:@"George Washington, 1600 Pennsylvania Ave NW"];

  // Keep editing the Country field and verify that validation works.
  for (const UserTypedCountryExpectedResultPair& expectation : kCountryTests) {
    // Switch on edit mode.
    [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                            IDS_IOS_NAVIGATION_BAR_EDIT_BUTTON)]
        performAction:grey_tap()];

    // Replace the text field with the user-version of the country.
    [[EarlGrey selectElementWithMatcher:grey_accessibilityID(GetTextFieldForID(
                                            IDS_IOS_AUTOFILL_COUNTRY))]
        performAction:grey_replaceText(expectation.user_typed_country)];

    // Switch off edit mode.
    [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
        performAction:grey_tap()];

    // Verify that the country value was changed to canonical.
    [[EarlGrey
        selectElementWithMatcher:
            grey_accessibilityLabel([NSString
                stringWithFormat:@"%@, %@", l10n_util::GetNSString(
                                                IDS_IOS_AUTOFILL_COUNTRY),
                                 expectation.expected_result])]
        assertWithMatcher:grey_notNil()];
  }

  [self exitSettingsMenu];
}

// Test that the page for viewing autofill profile details is accessible.
- (void)testAccessibilityOnAutofillProfileViewPage {
  [self loadAndSubmitTheForm];
  [self openEditAddress:@"George Washington, 1600 Pennsylvania Ave NW"];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  [self exitSettingsMenu];
}

// Test that the page for editing autofill profile details is accessible.
- (void)testAccessibilityOnAutofillProfileEditPage {
  [self loadAndSubmitTheForm];
  [self openEditAddress:@"George Washington, 1600 Pennsylvania Ave NW"];
  // Switch on edit mode.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_IOS_NAVIGATION_BAR_EDIT_BUTTON)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  [self exitSettingsMenu];
}

// Checks that if the autofill profiles and credit cards list view is in edit
// mode, the "autofill" and "wallet" switch items are disabled.
- (void)testListViewEditMode {
  [self loadAndSubmitTheForm];

  [ChromeEarlGreyUI openSettingsMenu];
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabel(
                                   l10n_util::GetNSString(IDS_IOS_AUTOFILL))]
      performAction:grey_tap()];

  // Switch on edit mode.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_IOS_NAVIGATION_BAR_EDIT_BUTTON)]
      performAction:grey_tap()];

  // Check the "autofill" and "wallet" switches are disabled. Disabled switches
  // are toggled off.
  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::CollectionViewSwitchCell(
                                   @"autofillItem_switch", NO, NO)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey
      selectElementWithMatcher:chrome_test_util::CollectionViewSwitchCell(
                                   @"walletItem_switch", NO, NO)]
      assertWithMatcher:grey_notNil()];
}

@end
