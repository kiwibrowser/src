// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/payment_method_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestPaymentMethodItemTest = PlatformTest;

// Tests that the labels and image are set properly after a call to
// |configureCell:|.
TEST_F(PaymentRequestPaymentMethodItemTest, TextLabels) {
  PaymentMethodItem* item = [[PaymentMethodItem alloc] init];

  NSString* methodID = @"BobPay - ****-6789";
  NSString* methodDetail = @"Bobs Your Uncle III, esq.";
  NSString* methodAddress = @"123 Bob St, Halifax, NS";
  NSString* notification = @"More information is required.";
  UIImage* methodTypeIcon = CollectionViewTestImage();
  MDCCollectionViewCellAccessoryType accessoryType =
      MDCCollectionViewCellAccessoryDisclosureIndicator;

  item.methodID = methodID;
  item.methodDetail = methodDetail;
  item.methodAddress = methodAddress;
  item.notification = notification;
  item.methodTypeIcon = methodTypeIcon;
  item.accessoryType = accessoryType;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[PaymentMethodCell class]]);

  PaymentMethodCell* paymentMethodCell = cell;
  EXPECT_FALSE(paymentMethodCell.methodIDLabel.text);
  EXPECT_FALSE(paymentMethodCell.methodDetailLabel.text);
  EXPECT_FALSE(paymentMethodCell.methodAddressLabel.text);
  EXPECT_FALSE(paymentMethodCell.notificationLabel.text);
  EXPECT_EQ(nil, paymentMethodCell.methodTypeIconView.image);
  EXPECT_EQ(paymentMethodCell.accessoryType,
            MDCCollectionViewCellAccessoryNone);

  [item configureCell:paymentMethodCell];
  EXPECT_NSEQ(methodID, paymentMethodCell.methodIDLabel.text);
  EXPECT_NSEQ(methodDetail, paymentMethodCell.methodDetailLabel.text);
  EXPECT_NSEQ(methodAddress, paymentMethodCell.methodAddressLabel.text);
  EXPECT_NSEQ(notification, paymentMethodCell.notificationLabel.text);
  EXPECT_NSEQ(methodTypeIcon, paymentMethodCell.methodTypeIconView.image);
  EXPECT_EQ(paymentMethodCell.accessoryType,
            MDCCollectionViewCellAccessoryDisclosureIndicator);
}

}  // namespace
