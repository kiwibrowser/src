// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using CollectionViewDetailItemTest = PlatformTest;

// Tests that the UILabels are set properly after a call to |configureCell:|.
TEST_F(CollectionViewDetailItemTest, TextLabels) {
  CollectionViewDetailItem* item =
      [[CollectionViewDetailItem alloc] initWithType:0];
  NSString* mainText = @"Main text";
  NSString* detailText = @"Detail text";

  item.text = mainText;
  item.detailText = detailText;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[CollectionViewDetailCell class]]);

  CollectionViewDetailCell* detailCell = cell;
  EXPECT_FALSE(detailCell.textLabel.text);
  EXPECT_FALSE(detailCell.detailTextLabel.text);

  [item configureCell:cell];
  EXPECT_NSEQ(mainText, detailCell.textLabel.text);
  EXPECT_NSEQ(detailText, detailCell.detailTextLabel.text);
}

using CollectionViewDetailCellTest = PlatformTest;

// Tests that each of the two text labels is provided with the correct amount
// of space.
TEST_F(CollectionViewDetailCellTest, TextLabelTargetWidths) {
  // Make the cell 148 wide so that after allocating 3 * kHorizontalPadding (16)
  // space for the margins and area between the labels, there is 100 available.
  // Accordingly, in each of the cases below where the sum of the desired label
  // widths exceeds 100, the sum of the constraints should equal 100.
  CollectionViewDetailCell* cell = [[CollectionViewDetailCell alloc]
      initWithFrame:CGRectMake(0, 0, 148, 100)];

  CGRect textLabelRect = CGRectZero;
  CGRect detailTextLabelRect = CGRectZero;

  // If there is enough room for both, each should be allowed its full width.
  textLabelRect.size.width = 50;
  detailTextLabelRect.size.width = 40;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(50, [cell textLabelTargetWidth]);
  EXPECT_EQ(40, [cell detailTextLabelTargetWidth]);

  // Even if there's only exactly enough room for both.
  textLabelRect.size.width = 30;
  detailTextLabelRect.size.width = 70;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(30, [cell textLabelTargetWidth]);
  EXPECT_EQ(70, [cell detailTextLabelTargetWidth]);

  // And even if one of the labels is really big.
  textLabelRect.size.width = 5;
  detailTextLabelRect.size.width = 95;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(5, [cell textLabelTargetWidth]);
  EXPECT_EQ(95, [cell detailTextLabelTargetWidth]);

  // But once they exceed the available width, start clipping the detail label.
  textLabelRect.size.width = 51;
  detailTextLabelRect.size.width = 50;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(51, [cell textLabelTargetWidth]);
  EXPECT_EQ(49, [cell detailTextLabelTargetWidth]);

  textLabelRect.size.width = 25;
  detailTextLabelRect.size.width = 90;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(25, [cell textLabelTargetWidth]);
  EXPECT_EQ(75, [cell detailTextLabelTargetWidth]);

  textLabelRect.size.width = 70;
  detailTextLabelRect.size.width = 50;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(70, [cell textLabelTargetWidth]);
  EXPECT_EQ(30, [cell detailTextLabelTargetWidth]);

  // On the other hand, if the text label wants more than 75% of the width, clip
  // it instead.
  textLabelRect.size.width = 90;
  detailTextLabelRect.size.width = 20;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(80, [cell textLabelTargetWidth]);
  EXPECT_EQ(20, [cell detailTextLabelTargetWidth]);

  // If both want more than their guaranteed minimum (75 and 25), give them each
  // the minimum.
  textLabelRect.size.width = 90;
  detailTextLabelRect.size.width = 50;
  cell.textLabel.frame = textLabelRect;
  cell.detailTextLabel.frame = detailTextLabelRect;
  EXPECT_EQ(75, [cell textLabelTargetWidth]);
  EXPECT_EQ(25, [cell detailTextLabelTargetWidth]);
}

}  // namespace
