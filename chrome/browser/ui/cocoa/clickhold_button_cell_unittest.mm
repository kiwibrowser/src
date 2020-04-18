// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/clickhold_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface ClickHoldButtonCellTestTarget : NSObject {
 @private
  BOOL didOpen_;
}
- (void)open;
- (BOOL)didOpen;
@end

@implementation ClickHoldButtonCellTestTarget
- (instancetype)init {
  if ((self = [super init]))
    didOpen_ = NO;

  return self;
}

- (void)open {
  didOpen_ = YES;
}

- (BOOL)didOpen {
  return didOpen_;
}
@end

namespace {

class ClickHoldButtonCellTest : public CocoaTest {
 public:
  ClickHoldButtonCellTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<NSButton> view(
        [[NSButton alloc] initWithFrame:frame]);
    view_ = view.get();
    base::scoped_nsobject<ClickHoldButtonCell> cell(
        [[ClickHoldButtonCell alloc] initTextCell:@"Testing"]);
    [view_ setCell:cell.get()];
    [[test_window() contentView] addSubview:view_];
  }

  NSButton* view_;
};

TEST_VIEW(ClickHoldButtonCellTest, view_)

// Test default values; make sure they are what they should be.
TEST_F(ClickHoldButtonCellTest, Defaults) {
  ClickHoldButtonCell* cell = static_cast<ClickHoldButtonCell*>([view_ cell]);
  ASSERT_TRUE([cell isKindOfClass:[ClickHoldButtonCell class]]);

  EXPECT_FALSE([cell enableClickHold]);

  NSTimeInterval clickHoldTimeout = [cell clickHoldTimeout];
  EXPECT_GE(clickHoldTimeout, 0.15);  // Check for a "Cocoa-ish" value.
  EXPECT_LE(clickHoldTimeout, 0.35);

  EXPECT_FALSE([cell trackOnlyInRect]);
  EXPECT_TRUE([cell activateOnDrag]);
}

// TODO(viettrungluu): (1) Enable click-hold and figure out how to test the
// tracking loop (i.e., |-trackMouse:...|), which is the nontrivial part.
// (2) Test various initialization code paths (in particular, loading from nib).

TEST_F(ClickHoldButtonCellTest, AccessibilityShowMenu) {
  ClickHoldButtonCell* cell = static_cast<ClickHoldButtonCell*>([view_ cell]);
  ASSERT_TRUE([cell isKindOfClass:[ClickHoldButtonCell class]]);

  ClickHoldButtonCellTestTarget* cellTarget =
      [[[ClickHoldButtonCellTestTarget alloc] init] autorelease];
  ASSERT_NSNE(nil, cellTarget);
  ASSERT_TRUE([cellTarget respondsToSelector:@selector(open)]);
  ASSERT_FALSE([cellTarget didOpen]);

  [cell setEnableClickHold:NO];

  NSArray* actionNames = [cell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [cell setEnableClickHold:YES];

  // There is no action set.
  actionNames = [cell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  // clickHoldAction should be independent from accessibilityShowMenuAction
  // since different operations, e.g. releasing vs. not releasing a mouse
  // button, may need to be performed for each occasion.
  [cell setClickHoldAction:@selector(open)];
  [cell setClickHoldTarget:cellTarget];

  actionNames = [cell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);
  EXPECT_FALSE([cellTarget didOpen]);

  // Even when accessibilityShowMenuAction is set, no action should be available
  // if neither enableClickHold nor enableRightClickHold are true.
  [cell setEnableClickHold:NO];
  [cell setAccessibilityShowMenuAction:@selector(open)];
  [cell setAccessibilityShowMenuTarget:cellTarget];

  actionNames = [cell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  // Now the action should be available.
  [cell setEnableClickHold:YES];

  actionNames = [cell accessibilityActionNames];
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [cell setEnableClickHold:NO];
  [cell setEnableRightClick:YES];

  actionNames = [cell accessibilityActionNames];
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [cell accessibilityPerformAction:NSAccessibilityShowMenuAction];
  EXPECT_TRUE([cellTarget didOpen]);
}

}  // namespace
