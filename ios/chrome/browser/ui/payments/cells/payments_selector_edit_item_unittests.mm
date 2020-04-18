// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/payments_selector_edit_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/collection_view_detail_item.h"
#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestPaymentsSelectorEditItemTest = PlatformTest;

// Tests that the UILabels are set properly after a call to |configureCell:|.
TEST_F(PaymentRequestPaymentsSelectorEditItemTest, TextLabels) {
  PaymentsSelectorEditItem* item = [[PaymentsSelectorEditItem alloc] init];
  NSString* name = @"Name text";
  NSString* value = @"Value text";

  item.name = name;
  item.value = value;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[CollectionViewDetailCell class]]);

  CollectionViewDetailCell* detailCell = cell;
  EXPECT_FALSE(detailCell.textLabel.text);
  EXPECT_FALSE(detailCell.detailTextLabel.text);

  [item configureCell:cell];
  EXPECT_NSEQ(name, detailCell.textLabel.text);
  EXPECT_NSEQ(value, detailCell.detailTextLabel.text);

  item.required = YES;
  NSString* requiredName = [NSString stringWithFormat:@"%@*", name];
  [item configureCell:cell];
  EXPECT_NSEQ(requiredName, detailCell.textLabel.text);
}

}  // namespace
