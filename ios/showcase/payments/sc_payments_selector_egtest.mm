// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>

#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/showcase/test/showcase_eg_utils.h"
#import "ios/showcase/test/showcase_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using ::showcase_utils::Open;
using ::showcase_utils::Close;
using ::chrome_test_util::ButtonWithAccessibilityLabel;

// Returns the GREYMatcher for the header item.
id<GREYMatcher> HeaderItem() {
  return grey_allOf(grey_accessibilityLabel(@"Header item"),
                    grey_kindOfClass([UILabel class]),
                    grey_sufficientlyVisible(), nil);
}

// Returns the GREYMatcher for the selectable item with the given label.
// |selected| states whether or not the item is selected.
id<GREYMatcher> SelectableItemWithLabel(NSString* label, BOOL selected) {
  id<GREYMatcher> matcher = grey_allOf(ButtonWithAccessibilityLabel(label),
                                       grey_sufficientlyVisible(), nil);
  if (selected) {
    return grey_allOf(
        matcher, grey_accessibilityTrait(UIAccessibilityTraitSelected), nil);
  }
  return matcher;
}

// Returns the GREYMatcher for the "Add" button.
id<GREYMatcher> AddButton() {
  return grey_allOf(ButtonWithAccessibilityLabel(@"Add an item"),
                    grey_sufficientlyVisible(), nil);
}

// Returns the GREYMatcher for the UIAlertView's message displayed for a call
// that notifies the delegate of a selection.
id<GREYMatcher> UIAlertViewMessageForDelegateCallForSelectionWithArgument(
    NSString* argument) {
  return grey_allOf(
      grey_text(
          [NSString stringWithFormat:
                        @"paymentRequestSelectorViewController:"
                        @"kPaymentRequestSelectorCollectionViewAccessibilityID "
                        @"didSelectItemAtIndex:%@",
                        argument]),
      grey_sufficientlyVisible(), nil);
}

// Returns the GREYMatcher for the UIAlertView's message displayed for a call
// that notifies the delegate of adding an item.
id<GREYMatcher> UIAlertViewMessageForDelegateCallForAddingItem() {
  return grey_allOf(
      grey_text(@"paymentRequestSelectorViewControllerDidSelectAddItem:"
                @"kPaymentRequestSelectorCollectionViewAccessibilityID"),
      grey_sufficientlyVisible(), nil);
}

}  // namespace

// Tests for the payment request selector view controller.
@interface SCPaymentsSelectorTestCase : ShowcaseTestCase
@end

@implementation SCPaymentsSelectorTestCase

- (void)setUp {
  [super setUp];
  Open(@"PaymentRequestSelectorViewController");
}

- (void)tearDown {
  Close();
  [super tearDown];
}

// Tests if all the expected items are present and that none is selected.
- (void)testVerifyItems {
  [[EarlGrey selectElementWithMatcher:HeaderItem()]
      assertWithMatcher:grey_notNil()];

  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"First selectable item", NO)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"Second selectable item", NO)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"Third selectable item", NO)]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"Fourth selectable item", NO)]
      assertWithMatcher:grey_notNil()];

  [[EarlGrey selectElementWithMatcher:AddButton()]
      assertWithMatcher:grey_notNil()];
}

// Tests that selectable items can be selected.
- (void)testCanSelectItems {
  // Tap the first selectable item which is currently not selected.
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"First selectable item", NO)]
      performAction:grey_tap()];

  // Confirm the delegate is informed.
  [[EarlGrey selectElementWithMatcher:
                 UIAlertViewMessageForDelegateCallForSelectionWithArgument(
                     @"0")] assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(
                                          @"protocol_alerter_done")]
      performAction:grey_tap()];

  // Confirm the first selectable item is now selected. Tap it again.
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"First selectable item", YES)]
      performAction:grey_tap()];

  // Confirm the delegate is informed.
  [[EarlGrey selectElementWithMatcher:
                 UIAlertViewMessageForDelegateCallForSelectionWithArgument(
                     @"0")] assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(
                                          @"protocol_alerter_done")]
      performAction:grey_tap()];

  // Confirm the first selectable item is still selected.
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"First selectable item", YES)]
      assertWithMatcher:grey_notNil()];

  // Tap the second selectable item which is currently not selected.
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"Second selectable item", NO)]
      performAction:grey_tap()];

  // Confirm the delegate is informed.
  [[EarlGrey selectElementWithMatcher:
                 UIAlertViewMessageForDelegateCallForSelectionWithArgument(
                     @"1")] assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(
                                          @"protocol_alerter_done")]
      performAction:grey_tap()];

  // Confirm the second selectable item is now selected.
  [[EarlGrey selectElementWithMatcher:SelectableItemWithLabel(
                                          @"Second selectable item", YES)]
      assertWithMatcher:grey_notNil()];
}

// Tests that item can be added.
- (void)testCanAddItem {
  // Tap the add button.
  [[EarlGrey selectElementWithMatcher:AddButton()] performAction:grey_tap()];

  // Confirm the delegate is informed.
  [[EarlGrey
      selectElementWithMatcher:UIAlertViewMessageForDelegateCallForAddingItem()]
      assertWithMatcher:grey_notNil()];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityLabel(
                                          @"protocol_alerter_done")]
      performAction:grey_tap()];
}

@end
