// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/history/history_search_view_controller.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/history/history_search_view.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// HistorySearchView category to expose the text field and cancel button.
@interface HistorySearchView (Testing)
@property(nonatomic, strong) UITextField* textField;
@property(nonatomic, strong) UIButton* cancelButton;
@end

// Test fixture for HistorySearchViewController.
class HistorySearchViewControllerTest : public PlatformTest {
 public:
  HistorySearchViewControllerTest() {
    search_view_controller_ = [[HistorySearchViewController alloc] init];
    [search_view_controller_ loadView];
    mock_delegate_ = [OCMockObject
        mockForProtocol:@protocol(HistorySearchViewControllerDelegate)];
    [search_view_controller_ setDelegate:mock_delegate_];
  }

 protected:
  __strong HistorySearchViewController* search_view_controller_;
  __strong id<HistorySearchViewControllerDelegate> mock_delegate_;
};

// Test that pressing the cancel button invokes delegate callback to cancel
// search.
TEST_F(HistorySearchViewControllerTest, CancelButtonPressed) {
  UIButton* cancel_button =
      base::mac::ObjCCastStrict<HistorySearchView>(search_view_controller_.view)
          .cancelButton;
  OCMockObject* mock_delegate = (OCMockObject*)mock_delegate_;
  [[mock_delegate expect]
      historySearchViewControllerDidCancel:search_view_controller_];
  [cancel_button sendActionsForControlEvents:UIControlEventTouchUpInside];
  EXPECT_OCMOCK_VERIFY(mock_delegate_);
}

// Test that invocation of
// textField:shouldChangeCharactersInRange:replacementString: on the text field
// delegate results invokes delegate callback to request search.
TEST_F(HistorySearchViewControllerTest, SearchButtonPressed) {
  UITextField* text_field =
      base::mac::ObjCCastStrict<HistorySearchView>(search_view_controller_.view)
          .textField;
  OCMockObject* mock_delegate = (OCMockObject*)mock_delegate_;
  [[mock_delegate expect] historySearchViewController:search_view_controller_
                              didRequestSearchForTerm:@"a"];
  [text_field.delegate textField:text_field
      shouldChangeCharactersInRange:NSMakeRange(0, 0)
                  replacementString:@"a"];
  EXPECT_OCMOCK_VERIFY(mock_delegate);
}

// Test that disabling HistorySearchViewController disables the search view text
// field.
TEST_F(HistorySearchViewControllerTest, DisableSearchBar) {
  UITextField* text_field =
      base::mac::ObjCCastStrict<HistorySearchView>(search_view_controller_.view)
          .textField;
  DCHECK(text_field);
  EXPECT_TRUE(text_field.enabled);

  search_view_controller_.enabled = NO;
  EXPECT_FALSE(text_field.enabled);

  search_view_controller_.enabled = YES;
  EXPECT_TRUE(text_field.enabled);
}
