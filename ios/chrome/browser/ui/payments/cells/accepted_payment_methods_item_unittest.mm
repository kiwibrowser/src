// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/accepted_payment_methods_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestAcceptedPaymentMethodsItemTest = PlatformTest;

// Tests that the label and the images are set properly after a call to
// |configureCell:|.
TEST_F(PaymentRequestAcceptedPaymentMethodsItemTest, TextLabelsAndImages) {
  AcceptedPaymentMethodsItem* item = [[AcceptedPaymentMethodsItem alloc] init];

  NSString* message = @"Lorem ipsum dolor sit amet";
  NSArray* methodTypeIcons =
      @[ CollectionViewTestImage(), CollectionViewTestImage() ];

  item.message = message;
  item.methodTypeIcons = methodTypeIcons;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[AcceptedPaymentMethodsCell class]]);

  AcceptedPaymentMethodsCell* acceptedPaymentMethodsCell = cell;
  EXPECT_FALSE(acceptedPaymentMethodsCell.messageLabel.text);
  EXPECT_EQ(nil, acceptedPaymentMethodsCell.methodTypeIconViews);

  [item configureCell:acceptedPaymentMethodsCell];
  EXPECT_NSEQ(message, acceptedPaymentMethodsCell.messageLabel.text);
  ASSERT_EQ(2UL, acceptedPaymentMethodsCell.methodTypeIconViews.count);
  EXPECT_NSEQ(methodTypeIcons[0],
              acceptedPaymentMethodsCell.methodTypeIconViews[0].image);
  EXPECT_NSEQ(methodTypeIcons[1],
              acceptedPaymentMethodsCell.methodTypeIconViews[1].image);
}

}  // namespace
