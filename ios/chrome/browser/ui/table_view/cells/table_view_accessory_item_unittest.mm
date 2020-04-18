// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/table_view/cells/table_view_accessory_item.h"

#include "base/mac/foundation_util.h"
#import "ios/chrome/browser/ui/table_view/chrome_table_view_styler.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using TableViewAccessoryItemTest = PlatformTest;
}

// Tests that the UILabel is set properly after a call to
// |configureCell:| and the image is visible.
TEST_F(TableViewAccessoryItemTest, ItemProperties) {
  NSString* text = @"Cell text";

  TableViewAccessoryItem* item =
      [[TableViewAccessoryItem alloc] initWithType:0];
  item.title = text;
  item.image = [[UIImage alloc] init];

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewAccessoryCell class]]);

  TableViewAccessoryCell* accessoryCell =
      base::mac::ObjCCastStrict<TableViewAccessoryCell>(cell);
  EXPECT_FALSE(accessoryCell.textLabel.text);
  EXPECT_FALSE(accessoryCell.imageView.image);

  [item configureCell:cell withStyler:[[ChromeTableViewStyler alloc] init]];
  EXPECT_NSEQ(text, accessoryCell.titleLabel.text);
  EXPECT_FALSE(accessoryCell.imageView.isHidden);
}

// Tests that the imageView is not visible if no image is set.
TEST_F(TableViewAccessoryItemTest, ItemImageViewHidden) {
  NSString* text = @"Cell text";

  TableViewAccessoryItem* item =
      [[TableViewAccessoryItem alloc] initWithType:0];
  item.title = text;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[TableViewAccessoryCell class]]);

  TableViewAccessoryCell* accessoryCell =
      base::mac::ObjCCastStrict<TableViewAccessoryCell>(cell);
  EXPECT_FALSE(item.image);
  [item configureCell:cell withStyler:[[ChromeTableViewStyler alloc] init]];
  EXPECT_FALSE(item.image);
  EXPECT_TRUE(accessoryCell.imageView.isHidden);
}
