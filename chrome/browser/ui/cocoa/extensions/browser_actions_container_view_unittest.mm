// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/extensions/browser_actions_container_view.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

const CGFloat kContainerHeight = 15.0;
const CGFloat kMinimumContainerWidth = 3.0;
const CGFloat kMaxAllowedWidthForTest = 50.0;

class BrowserActionsContainerViewTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    view_.reset([[BrowserActionsContainerView alloc]
        initWithFrame:NSMakeRect(0, 0, 0, kContainerHeight)]);
  }

  base::scoped_nsobject<BrowserActionsContainerView> view_;
};

TEST_F(BrowserActionsContainerViewTest, BasicTests) {
  EXPECT_TRUE([view_ isHidden]);
}

TEST_F(BrowserActionsContainerViewTest, SetWidthTests) {
  [view_ setMinWidth:kMinimumContainerWidth];
  [view_ setMaxWidth:kMaxAllowedWidthForTest];

  // Try setting below the minimum width.
  [view_ resizeToWidth:kMinimumContainerWidth - 1 animate:NO];
  EXPECT_EQ(kMinimumContainerWidth, NSWidth([view_ frame])) << "Frame width is "
      << "less than the minimum allowed.";

  [view_ resizeToWidth:35.0 animate:NO];
  EXPECT_EQ(35.0, NSWidth([view_ frame]));

  [view_ resizeToWidth:20.0 animate:NO];
  EXPECT_EQ(20.0, NSWidth([view_ frame]));

  // Resize the view with animation. It shouldn't immediately take the new
  // value, but the animationEndFrame should reflect the pending change.
  [view_ resizeToWidth:40.0 animate:YES];
  EXPECT_LE(NSWidth([view_ frame]), 40.0);
  EXPECT_EQ(40.0, NSWidth([view_ animationEndFrame]));

  // The container should be able to be resized while animating (simply taking
  // the newest target width).
  [view_ resizeToWidth:30.0 animate:YES];
  EXPECT_EQ(30.0, NSWidth([view_ animationEndFrame]));

  // Test with no animation again. The animationEndFrame should also reflect
  // the current frame, if no animation is pending.
  [view_ resizeToWidth:35.0 animate:NO];
  EXPECT_EQ(35.0, NSWidth([view_ frame]));
  EXPECT_EQ(35.0, NSWidth([view_ animationEndFrame]));

  [view_ resizeToWidth:kMaxAllowedWidthForTest + 10.0 animate:NO];
  EXPECT_EQ(kMaxAllowedWidthForTest, NSWidth([view_ frame]));
}

}  // namespace
