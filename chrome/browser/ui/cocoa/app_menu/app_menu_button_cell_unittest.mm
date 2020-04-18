// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/app_menu/app_menu_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

@interface TestAppMenuButton : NSButton
@end
@implementation TestAppMenuButton
+ (Class)cellClass {
  return [AppMenuButtonCell class];
}
@end

namespace {

class AppMenuButtonCellTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();

    NSRect frame = NSMakeRect(10, 10, 50, 19);
    button_.reset([[TestAppMenuButton alloc] initWithFrame:frame]);
    [button_ setBezelStyle:NSSmallSquareBezelStyle];
    [[button_ cell] setControlSize:NSSmallControlSize];
    [button_ setTitle:@"Allays"];
    [button_ setButtonType:NSMomentaryPushInButton];
  }

  base::scoped_nsobject<NSButton> button_;
};

TEST_F(AppMenuButtonCellTest, Draw) {
  ASSERT_TRUE(button_.get());
  [[test_window() contentView] addSubview:button_.get()];
  [button_ setNeedsDisplay:YES];
}

TEST_F(AppMenuButtonCellTest, DrawHighlight) {
  ASSERT_TRUE(button_.get());
  [[test_window() contentView] addSubview:button_.get()];
  [button_ highlight:YES];
  [button_ setNeedsDisplay:YES];
}

}  // namespace
