// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/cells/autofill_profile_item.h"

#import "ios/chrome/browser/ui/collection_view/cells/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using PaymentRequestAutofillProfileItemTest = PlatformTest;

// Tests that the labels are set properly after a call to |configureCell:|.
TEST_F(PaymentRequestAutofillProfileItemTest, TextLabels) {
  AutofillProfileItem* item = [[AutofillProfileItem alloc] init];

  NSString* name = @"Jon Doe";
  NSString* address = @"123 Main St, Anytown, USA";
  NSString* phoneNumber = @"1234567890";
  NSString* email = @"123-456-7890";
  NSString* notification = @"More information is required";

  item.name = name;
  item.address = address;
  item.phoneNumber = phoneNumber;
  item.email = email;
  item.notification = notification;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[AutofillProfileCell class]]);

  AutofillProfileCell* autofillProfileCell = cell;
  EXPECT_FALSE(autofillProfileCell.nameLabel.text);
  EXPECT_FALSE(autofillProfileCell.addressLabel.text);
  EXPECT_FALSE(autofillProfileCell.phoneNumberLabel.text);
  EXPECT_FALSE(autofillProfileCell.emailLabel.text);
  EXPECT_FALSE(autofillProfileCell.notificationLabel.text);

  [item configureCell:autofillProfileCell];
  EXPECT_NSEQ(name, autofillProfileCell.nameLabel.text);
  EXPECT_NSEQ(address, autofillProfileCell.addressLabel.text);
  EXPECT_NSEQ(phoneNumber, autofillProfileCell.phoneNumberLabel.text);
  EXPECT_NSEQ(email, autofillProfileCell.emailLabel.text);
  EXPECT_NSEQ(notification, autofillProfileCell.notificationLabel.text);
}

}  // namespace
