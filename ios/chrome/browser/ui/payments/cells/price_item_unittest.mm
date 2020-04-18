// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/price_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestPriceItemTest = PlatformTest;

// Tests that the labels are set properly after a call to |configureCell:|.
TEST_F(PaymentRequestPriceItemTest, TextLabels) {
  PriceItem* priceItem = [[PriceItem alloc] init];

  NSString* item = @"Total";
  NSString* notification = @"Updated";
  NSString* price = @"USD $60.0";

  priceItem.item = item;
  priceItem.notification = notification;
  priceItem.price = price;

  id cell = [[[priceItem cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[PriceCell class]]);

  PriceCell* priceCell = cell;
  EXPECT_FALSE(priceCell.itemLabel.text);
  EXPECT_FALSE(priceCell.notificationLabel.text);
  EXPECT_FALSE(priceCell.priceLabel.text);

  [priceItem configureCell:priceCell];
  EXPECT_NSEQ(item, priceCell.itemLabel.text);
  EXPECT_NSEQ(notification, priceCell.notificationLabel.text);
  EXPECT_NSEQ(price, priceCell.priceLabel.text);
}

// Tests that the labels are provided with the correct amount of space.
TEST_F(PaymentRequestPriceItemTest, TextLabelTargetWidths) {
  // Make the cell 164 wide so that after allocating 4 * kHorizontalPadding (16)
  // space for the margins and area between the labels, there is 100 available.
  // Accordingly, in each of the cases below where the sum of the desired label
  // widths exceeds 100, the sum of the constraints should equal 100.
  PriceCell* cell =
      [[PriceCell alloc] initWithFrame:CGRectMake(0, 0, 164, 100)];

  CGRect itemLabelRect = CGRectZero;
  CGRect notificationLabelRect = CGRectZero;
  CGRect priceLabelRect = CGRectZero;

  // If there is enough room for all the labels they should be allowed their
  // full widths.
  itemLabelRect.size.width = 70;
  notificationLabelRect.size.width = 10;
  priceLabelRect.size.width = 20;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(70, [cell itemLabelTargetWidth]);
  EXPECT_EQ(10, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(20, [cell priceLabelTargetWidth]);

  itemLabelRect.size.width = 20;
  notificationLabelRect.size.width = 70;
  priceLabelRect.size.width = 10;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(20, [cell itemLabelTargetWidth]);
  EXPECT_EQ(70, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(10, [cell priceLabelTargetWidth]);

  itemLabelRect.size.width = 10;
  notificationLabelRect.size.width = 20;
  priceLabelRect.size.width = 70;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(10, [cell itemLabelTargetWidth]);
  EXPECT_EQ(20, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(70, [cell priceLabelTargetWidth]);

  // But once they exceed the available width, priority is given to the price
  // label. It can take up to 50% of the available width before getting clipped.
  // The item label gets clipped first, but never gets clipped to shorter than
  // 50% of the remaining width.
  itemLabelRect.size.width = 60;
  notificationLabelRect.size.width = 20;
  priceLabelRect.size.width = 50;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(30, [cell itemLabelTargetWidth]);
  EXPECT_EQ(20, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);

  // If that's not enough, the notification label gets clipped too.
  itemLabelRect.size.width = 60;
  notificationLabelRect.size.width = 40;
  priceLabelRect.size.width = 50;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(25, [cell itemLabelTargetWidth]);
  EXPECT_EQ(25, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);

  // Unless the price label needs to take up more than 50% of the available
  // width, in which case, it gets clipped too.
  itemLabelRect.size.width = 60;
  notificationLabelRect.size.width = 20;
  priceLabelRect.size.width = 70;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(30, [cell itemLabelTargetWidth]);
  EXPECT_EQ(20, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);

  itemLabelRect.size.width = 60;
  notificationLabelRect.size.width = 40;
  priceLabelRect.size.width = 70;
  cell.itemLabel.frame = itemLabelRect;
  cell.notificationLabel.frame = notificationLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(25, [cell itemLabelTargetWidth]);
  EXPECT_EQ(25, [cell notificationLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);

  // Test the scenario where the notification label is nil. Make the cell 148
  // wide so that after allocating 3 * kHorizontalPadding (16) space for the
  // margins and area between the labels, there is 100 available.
  cell = [[PriceCell alloc] initWithFrame:CGRectMake(0, 0, 148, 100)];
  cell.notificationLabel.frame = CGRectZero;

  // If there is enough room for both item and price labels they should be
  // allowed their full widths.
  itemLabelRect.size.width = 90;
  priceLabelRect.size.width = 10;
  cell.itemLabel.frame = itemLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(90, [cell itemLabelTargetWidth]);
  EXPECT_EQ(10, [cell priceLabelTargetWidth]);

  itemLabelRect.size.width = 10;
  priceLabelRect.size.width = 90;
  cell.itemLabel.frame = itemLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(10, [cell itemLabelTargetWidth]);
  EXPECT_EQ(90, [cell priceLabelTargetWidth]);

  // But once they exceed the available width, start clipping the item label.
  itemLabelRect.size.width = 90;
  priceLabelRect.size.width = 50;
  cell.itemLabel.frame = itemLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(50, [cell itemLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);

  // Unless the price label wants to take up more than 50% of the available
  // width, in which case, it gets clipped too.
  itemLabelRect.size.width = 90;
  priceLabelRect.size.width = 60;
  cell.itemLabel.frame = itemLabelRect;
  cell.priceLabel.frame = priceLabelRect;
  EXPECT_EQ(50, [cell itemLabelTargetWidth]);
  EXPECT_EQ(50, [cell priceLabelTargetWidth]);
}

}  // namespace
