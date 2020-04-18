// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/settings/cells/sync_switch_item.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

using SyncSwitchItemTest = PlatformTest;

// Tests that the text label and showing status are set properly after a call to
// |configureCell:|.
TEST_F(SyncSwitchItemTest, ConfigureCell) {
  SyncSwitchItem* item = [[SyncSwitchItem alloc] initWithType:0];
  SyncSwitchCell* cell = [[[item cellClass] alloc] init];
  EXPECT_TRUE([cell isMemberOfClass:[SyncSwitchCell class]]);
  EXPECT_NSEQ(nil, cell.textLabel.text);

  NSString* text = @"Test Switch";
  NSString* detailText = @"Test Switch Detail";

  item.text = text;
  item.detailText = detailText;
  item.on = YES;

  EXPECT_FALSE(cell.textLabel.text);
  EXPECT_FALSE(cell.detailTextLabel.text);
  EXPECT_FALSE(cell.switchView.isOn);

  [item configureCell:cell];
  EXPECT_NSEQ(text, cell.textLabel.text);
  EXPECT_NSEQ(detailText, cell.detailTextLabel.text);
  EXPECT_TRUE(cell.switchView.isOn);
}

}  // namespace

// Tests that the text color and enabled state of the switch are set correctly
// by a call to |configureCell:|.
TEST_F(SyncSwitchItemTest, EnabledAndDisabled) {
  SyncSwitchCell* cell = [[SyncSwitchCell alloc] init];
  SyncSwitchItem* item = [[SyncSwitchItem alloc] initWithType:0];
  item.text = @"Test Switch";

  // Text color possibilities.
  UIColor* enabledColor =
      [SyncSwitchCell defaultTextColorForState:UIControlStateNormal];
  UIColor* disabledColor =
      [SyncSwitchCell defaultTextColorForState:UIControlStateDisabled];

  // Enabled and off.
  item.on = NO;
  item.enabled = YES;
  [item configureCell:cell];
  EXPECT_FALSE(cell.switchView.isOn);
  EXPECT_NSEQ(enabledColor, cell.textLabel.textColor);

  // Enabled and on.
  item.on = YES;
  item.enabled = YES;
  [item configureCell:cell];
  EXPECT_TRUE(cell.switchView.isOn);
  EXPECT_NSEQ(enabledColor, cell.textLabel.textColor);

  // Disabled and off.
  item.on = NO;
  item.enabled = NO;
  [item configureCell:cell];
  EXPECT_FALSE(cell.switchView.isOn);
  EXPECT_NSEQ(disabledColor, cell.textLabel.textColor);

  // Disabled and on. Disabling shouldn't change the switch's on value.
  item.on = YES;
  item.enabled = NO;
  [item configureCell:cell];
  EXPECT_TRUE(cell.switchView.isOn);
  EXPECT_NSEQ(disabledColor, cell.textLabel.textColor);
}

TEST_F(SyncSwitchItemTest, PrepareForReuseClearsActions) {
  SyncSwitchCell* cell = [[SyncSwitchCell alloc] init];
  UISwitch* switchView = cell.switchView;
  NSArray* target = [NSArray array];

  EXPECT_EQ(0U, [[switchView allTargets] count]);
  [switchView addTarget:target
                 action:@selector(count)
       forControlEvents:UIControlEventValueChanged];
  EXPECT_EQ(1U, [[switchView allTargets] count]);

  [cell prepareForReuse];
  EXPECT_EQ(0U, [[switchView allTargets] count]);
}
