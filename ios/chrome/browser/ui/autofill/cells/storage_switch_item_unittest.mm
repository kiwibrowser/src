// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/cells/storage_switch_item.h"

#include "base/mac/foundation_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using StorageSwitchItemTest = PlatformTest;

// Tests that the label and switch values are set properly after a call to
// |configureCell:|.
TEST_F(StorageSwitchItemTest, ConfigureCell) {
  StorageSwitchItem* item = [[StorageSwitchItem alloc] initWithType:0];
  item.on = YES;

  id cell = [[[item cellClass] alloc] init];
  ASSERT_TRUE([cell isMemberOfClass:[StorageSwitchCell class]]);

  StorageSwitchCell* switchCell =
      base::mac::ObjCCastStrict<StorageSwitchCell>(cell);
  EXPECT_TRUE(switchCell.textLabel.text);
  EXPECT_FALSE(switchCell.switchView.on);

  [item configureCell:cell];
  EXPECT_TRUE(switchCell.switchView.on);
}

TEST_F(StorageSwitchItemTest, PrepareForReuseClearsActions) {
  StorageSwitchCell* cell = [[StorageSwitchCell alloc] init];
  UIButton* tooltipButton = cell.tooltipButton;
  UISwitch* switchView = cell.switchView;
  NSArray* target = [NSArray array];

  EXPECT_EQ(0U, [[tooltipButton allTargets] count]);
  EXPECT_EQ(0U, [[switchView allTargets] count]);
  [tooltipButton addTarget:target
                    action:@selector(count)
          forControlEvents:UIControlEventTouchUpInside];
  [switchView addTarget:target
                 action:@selector(count)
       forControlEvents:UIControlEventValueChanged];
  EXPECT_EQ(1U, [[tooltipButton allTargets] count]);
  EXPECT_EQ(1U, [[switchView allTargets] count]);

  [cell prepareForReuse];
  EXPECT_EQ(0U, [[tooltipButton allTargets] count]);
  EXPECT_EQ(0U, [[switchView allTargets] count]);
}

}  // namespace
