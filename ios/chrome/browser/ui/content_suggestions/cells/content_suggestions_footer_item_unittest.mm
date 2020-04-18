// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/content_suggestions/cells/content_suggestions_footer_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using ContentSuggestionsFooterItemTest = PlatformTest;

// Tests that configureCell: sets the title of the button and the action.
TEST_F(ContentSuggestionsFooterItemTest, CellIsConfigured) {
  // Setup.
  NSString* title = @"testTitle";
  ContentSuggestionsFooterItem* item =
      [[ContentSuggestionsFooterItem alloc] initWithType:0
                                                   title:title
                                                callback:nil];
  item.loading = YES;
  ContentSuggestionsFooterCell* cell = [[[item cellClass] alloc] init];
  ASSERT_EQ([ContentSuggestionsFooterCell class], [cell class]);
  ASSERT_EQ(nil, [cell.button titleForState:UIControlStateNormal]);
  id mockCell = OCMPartialMock(cell);
  OCMExpect([mockCell setLoading:YES]);

  // Action.
  [item configureCell:cell];

  // Tests.
  EXPECT_EQ(title, [cell.button titleForState:UIControlStateNormal]);
  EXPECT_EQ(cell, item.configuredCell);
  EXPECT_OCMOCK_VERIFY(mockCell);
}

// Tests that the item is the delegate for the cell, and when the cell button is
// tapped, the callback is called.
TEST_F(ContentSuggestionsFooterItemTest, CellTapped) {
  NSString* title = @"testTitle";
  __block BOOL hasBeenCalled = NO;
  __block ContentSuggestionsFooterCell* blockCell = nil;
  __block ContentSuggestionsFooterItem* blockItem = nil;
  void (^block)(ContentSuggestionsFooterItem*, ContentSuggestionsFooterCell*) =
      ^void(ContentSuggestionsFooterItem* item,
            ContentSuggestionsFooterCell* cell) {
        hasBeenCalled = YES;
        blockCell = cell;
        blockItem = item;
      };
  ContentSuggestionsFooterItem* item =
      [[ContentSuggestionsFooterItem alloc] initWithType:0
                                                   title:title
                                                callback:block];
  item.loading = NO;
  ContentSuggestionsFooterCell* cell = [[[item cellClass] alloc] init];
  id mockCell = OCMPartialMock(cell);
  OCMExpect([mockCell setLoading:NO]);

  // Action.
  [item configureCell:cell];

  // Test.
  EXPECT_EQ(title, [cell.button titleForState:UIControlStateNormal]);
  EXPECT_FALSE(hasBeenCalled);
  [cell.button sendActionsForControlEvents:UIControlEventTouchUpInside];
  EXPECT_TRUE(hasBeenCalled);
  EXPECT_EQ(item, blockItem);
  EXPECT_EQ(cell, blockCell);
  EXPECT_EQ(item, cell.delegate);
  EXPECT_OCMOCK_VERIFY(mockCell);
}
}
