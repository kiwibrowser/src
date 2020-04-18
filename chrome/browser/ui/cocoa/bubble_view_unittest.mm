// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/bubble_view.h"
#include "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "testing/gtest_mac.h"

class BubbleViewTest : public CocoaTest {
 public:
  BubbleViewTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 50);
    base::scoped_nsobject<BubbleView> view(
        [[BubbleView alloc] initWithFrame:frame]);
    [view setThemeProvider:test_window()];
    view_ = view.get();
    [[test_window() contentView] addSubview:view_];
    [view_ setContent:@"Hi there, I'm a bubble view"];
  }

  BubbleView* view_;
};

TEST_VIEW(BubbleViewTest, view_);

// Test with no themeProvider.
TEST_F(BubbleViewTest, NilThemeProvider) {
  NSRect frame = NSMakeRect(0, 0, 50, 50);
  base::scoped_nsobject<BubbleView> view(
      [[BubbleView alloc] initWithFrame:frame]);
  [[test_window() contentView] addSubview:view.get()];
  [view display];
}

// Make sure things don't go haywire when given invalid or long strings.
TEST_F(BubbleViewTest, SetContent) {
  [view_ setContent:nil];
  EXPECT_TRUE([view_ content] == nil);
  [view_ setContent:@""];
  EXPECT_NSEQ(@"", [view_ content]);
  NSString* str = @"This is a really really long string that's just too long";
  [view_ setContent:str];
  EXPECT_NSEQ(str, [view_ content]);
}

TEST_F(BubbleViewTest, CornerFlags) {
  // Set some random flags just to check.
  [view_ setCornerFlags:kRoundedTopRightCorner | kRoundedTopLeftCorner];
  EXPECT_EQ([view_ cornerFlags],
            (unsigned long)kRoundedTopRightCorner | kRoundedTopLeftCorner);
  // Set no flags (all 4 draw corners are square).
  [view_ setCornerFlags:0];
  EXPECT_EQ([view_ cornerFlags], 0UL);
  // Set all bits. Meaningless past the first 4, but harmless to set too many.
  [view_ setCornerFlags:0xFFFFFFFF];
  EXPECT_EQ([view_ cornerFlags], 0xFFFFFFFF);
}
