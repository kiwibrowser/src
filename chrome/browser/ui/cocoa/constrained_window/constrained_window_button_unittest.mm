// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/constrained_window/constrained_window_button.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/events/test/cocoa_test_event_utils.h"

class ConstrainedWindowButtonTest : public CocoaTest {
 public:
  ConstrainedWindowButtonTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    button_.reset([[ConstrainedWindowButton alloc] initWithFrame:frame]);
    [button_ setTitle:@"Abcdefg"];
    [button_ sizeToFit];
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  base::scoped_nsobject<ConstrainedWindowButton> button_;
};

TEST_VIEW(ConstrainedWindowButtonTest, button_)

// Test hover, mostly to ensure nothing leaks or crashes.
TEST_F(ConstrainedWindowButtonTest, DisplayWithHover) {
  [[button_ cell] setIsMouseInside:NO];
  [button_ display];
  [[button_ cell] setIsMouseInside:YES];
  [button_ display];
}

// Test disabled, mostly to ensure nothing leaks or crashes.
TEST_F(ConstrainedWindowButtonTest, DisplayWithDisable) {
  [button_ setEnabled:YES];
  [button_ display];
  [button_ setEnabled:NO];
  [button_ display];
}

// Test pushed, mostly to ensure nothing leaks or crashes.
TEST_F(ConstrainedWindowButtonTest, DisplayWithPushed) {
  [[button_ cell] setHighlighted:NO];
  [button_ display];
  [[button_ cell] setHighlighted:YES];
  [button_ display];
}

// Tracking rects
TEST_F(ConstrainedWindowButtonTest, TrackingRects) {
  ConstrainedWindowButtonCell* cell = [button_ cell];
  EXPECT_FALSE([cell isMouseInside]);

  [button_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_TRUE([cell isMouseInside]);
  [button_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_FALSE([cell isMouseInside]);
}
