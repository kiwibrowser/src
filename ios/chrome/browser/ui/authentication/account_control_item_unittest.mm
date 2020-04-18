// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/account_control_item.h"

#import "ios/chrome/browser/ui/colors/MDCPalette+CrAdditions.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using AccountControlItemTest = PlatformTest;

// Tests that the cell is properly configured with image and texts after a call
// to |configureCell:|. All other cell decorations are default.
TEST_F(AccountControlItemTest, ConfigureCellDefault) {
  AccountControlItem* item = [[AccountControlItem alloc] initWithType:0];
  UIImage* image = [[UIImage alloc] init];
  NSString* mainText = @"Main text";
  NSString* detailText = @"Detail text";

  item.image = image;
  item.text = mainText;
  item.detailText = detailText;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[AccountControlCell class]]);

  AccountControlCell* accountCell = cell;
  [accountCell prepareForReuse];
  EXPECT_FALSE(accountCell.imageView.image);
  EXPECT_FALSE(accountCell.textLabel.text);
  EXPECT_FALSE(accountCell.detailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, accountCell.accessoryType);
  EXPECT_NSEQ([[MDCPalette greyPalette] tint700],
              accountCell.detailTextLabel.textColor);

  [item configureCell:cell];
  EXPECT_NSEQ(image, accountCell.imageView.image);
  EXPECT_NSEQ(mainText, accountCell.textLabel.text);
  EXPECT_NSEQ(detailText, accountCell.detailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, accountCell.accessoryType);
  EXPECT_NSEQ([[MDCPalette greyPalette] tint700],
              accountCell.detailTextLabel.textColor);
}

// Tests that the cell is properly configured with error and an accessory after
// a call to |configureCell:|.
TEST_F(AccountControlItemTest, ConfigureCellWithErrorAndAccessory) {
  AccountControlItem* item = [[AccountControlItem alloc] initWithType:0];
  UIImage* image = [[UIImage alloc] init];
  NSString* mainText = @"Main text";
  NSString* detailText = @"Detail text";

  item.image = image;
  item.text = mainText;
  item.detailText = detailText;
  item.accessoryType = MDCCollectionViewCellAccessoryCheckmark;
  item.shouldDisplayError = YES;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[AccountControlCell class]]);

  AccountControlCell* accountCell = cell;
  [accountCell prepareForReuse];
  EXPECT_FALSE(accountCell.imageView.image);
  EXPECT_FALSE(accountCell.textLabel.text);
  EXPECT_FALSE(accountCell.detailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryNone, accountCell.accessoryType);
  EXPECT_NSEQ([[MDCPalette greyPalette] tint700],
              accountCell.detailTextLabel.textColor);

  [item configureCell:cell];
  EXPECT_NSEQ(image, accountCell.imageView.image);
  EXPECT_NSEQ(mainText, accountCell.textLabel.text);
  EXPECT_NSEQ(detailText, accountCell.detailTextLabel.text);
  EXPECT_EQ(MDCCollectionViewCellAccessoryCheckmark, accountCell.accessoryType);
  EXPECT_NSEQ([[MDCPalette cr_redPalette] tint700],
              accountCell.detailTextLabel.textColor);
}
