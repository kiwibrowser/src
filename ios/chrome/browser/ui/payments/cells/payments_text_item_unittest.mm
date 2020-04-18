// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/payments_text_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestPaymentsTextItemTest = PlatformTest;

// Tests that the text label and the image are set properly after a call to
// |configureCell:|.
TEST_F(PaymentRequestPaymentsTextItemTest, TextLabelAndImage) {
  PaymentsTextItem* item = [[PaymentsTextItem alloc] init];

  NSString* text = @"Lorem ipsum";
  NSString* detailText = @"Dolor sit amet";
  UIImage* image = CollectionViewTestImage();

  item.text = text;
  item.detailText = detailText;
  item.leadingImage = image;
  item.trailingImage = image;
  item.cellType = PaymentsTextCellTypeCallToAction;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[PaymentsTextCell class]]);

  PaymentsTextCell* paymentsTextCell = cell;
  EXPECT_FALSE(paymentsTextCell.textLabel.text);
  EXPECT_FALSE(paymentsTextCell.detailTextLabel.text);
  EXPECT_FALSE(paymentsTextCell.leadingImageView.image);
  EXPECT_FALSE(paymentsTextCell.trailingImageView.image);
  EXPECT_EQ(PaymentsTextCellTypeNormal, paymentsTextCell.cellType);

  [item configureCell:paymentsTextCell];
  EXPECT_NSEQ(text, paymentsTextCell.textLabel.text);
  EXPECT_NSEQ(detailText, paymentsTextCell.detailTextLabel.text);
  EXPECT_NSEQ(image, paymentsTextCell.leadingImageView.image);
  EXPECT_NSEQ(image, paymentsTextCell.trailingImageView.image);
  EXPECT_EQ(PaymentsTextCellTypeCallToAction, paymentsTextCell.cellType);
}

}  // namespace
