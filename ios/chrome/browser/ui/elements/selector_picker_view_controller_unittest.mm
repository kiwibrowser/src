// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/elements/selector_picker_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/elements/selector_view_controller_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SelectorPickerViewController ()
// The displayed UINavigationBar. Exposed for testing.
@property(nonatomic, retain) UINavigationBar* navigationBar;
// The displayed UIPickerView. Exposed for testing.
@property(nonatomic, retain) UIPickerView* pickerView;
@end

using SelectorPickerViewControllerTest = PlatformTest;

// Test that invoking the right bar button action ("Done") invokes the delegate
// callback with the appropriate selected item.
TEST_F(SelectorPickerViewControllerTest, Done) {
  NSString* option1 = @"Option 1";
  NSString* option2 = @"Option 2";
  NSOrderedSet<NSString*>* options =
      [NSOrderedSet orderedSetWithArray:@[ option1, option2 ]];
  SelectorPickerViewController* selector_picker_view_controller =
      [[SelectorPickerViewController alloc] initWithOptions:options
                                                    default:option1];
  id delegate =
      [OCMockObject mockForProtocol:@protocol(SelectorViewControllerDelegate)];
  [[delegate expect] selectorViewController:selector_picker_view_controller
                            didSelectOption:option2];
  selector_picker_view_controller.delegate = delegate;
  [selector_picker_view_controller loadView];
  [selector_picker_view_controller viewDidLoad];

  [selector_picker_view_controller.pickerView selectRow:1
                                            inComponent:0
                                               animated:NO];
  UIBarButtonItem* rightButton =
      selector_picker_view_controller.navigationBar.topItem.rightBarButtonItem;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [selector_picker_view_controller performSelector:rightButton.action];
#pragma clang diagnostic pop

  EXPECT_OCMOCK_VERIFY(delegate);
}

// Test that invoking the right bar button action ("Cancel") invokes the
// delegate callback with the default item.
TEST_F(SelectorPickerViewControllerTest, Cancel) {
  NSString* option1 = @"Option 1";
  NSString* option2 = @"Option 2";
  NSOrderedSet<NSString*>* options =
      [NSOrderedSet orderedSetWithArray:@[ option1, option2 ]];
  SelectorPickerViewController* selector_picker_view_controller =
      [[SelectorPickerViewController alloc] initWithOptions:options
                                                    default:option2];
  id delegate =
      [OCMockObject mockForProtocol:@protocol(SelectorViewControllerDelegate)];
  [[delegate expect] selectorViewController:selector_picker_view_controller
                            didSelectOption:option2];
  selector_picker_view_controller.delegate = delegate;
  [selector_picker_view_controller loadView];
  [selector_picker_view_controller viewDidLoad];

  [selector_picker_view_controller.pickerView selectRow:1
                                            inComponent:0
                                               animated:NO];
  UIBarButtonItem* leftButton =
      selector_picker_view_controller.navigationBar.topItem.leftBarButtonItem;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
  [selector_picker_view_controller performSelector:leftButton.action];
#pragma clang diagnostic pop
  EXPECT_OCMOCK_VERIFY(delegate);
}

// Test that the picker view is styled appropriately based on parameters
// provided at initialization.
TEST_F(SelectorPickerViewControllerTest, DefaultStyling) {
  NSString* option1 = @"Option 1";
  NSString* option2 = @"Option 2";
  NSString* option3 = @"Option 3";
  NSOrderedSet<NSString*>* options =
      [NSOrderedSet orderedSetWithArray:@[ option1, option2, option3 ]];
  SelectorPickerViewController* selector_picker_view_controller =
      [[SelectorPickerViewController alloc] initWithOptions:options
                                                    default:option2];
  [selector_picker_view_controller loadView];
  [selector_picker_view_controller viewDidLoad];

  // Row 2 (index 1), the default should be selected be bolded.
  EXPECT_EQ(
      1, [selector_picker_view_controller.pickerView selectedRowInComponent:0]);
  UILabel* row2 = base::mac::ObjCCastStrict<UILabel>(
      [selector_picker_view_controller.pickerView viewForRow:1 forComponent:0]);
  EXPECT_NSEQ(option2, row2.text);
  EXPECT_TRUE(row2.font.fontDescriptor.symbolicTraits &
              UIFontDescriptorTraitBold);

  // Rows 1 and 3 should not be bold.
  UILabel* row1 = base::mac::ObjCCastStrict<UILabel>(
      [selector_picker_view_controller.pickerView viewForRow:0 forComponent:0]);
  EXPECT_NSEQ(option1, row1.text);
  EXPECT_FALSE(row1.font.fontDescriptor.symbolicTraits &
               UIFontDescriptorTraitBold);
  UILabel* row3 = base::mac::ObjCCastStrict<UILabel>(
      [selector_picker_view_controller.pickerView viewForRow:2 forComponent:0]);
  EXPECT_NSEQ(option3, row3.text);
  EXPECT_FALSE(row3.font.fontDescriptor.symbolicTraits &
               UIFontDescriptorTraitBold);
}
