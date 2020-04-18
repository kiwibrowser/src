// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/gradient_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface GradientButtonCell (HoverValueTesting)
- (void)performOnePulseStep;
@end

namespace {

class GradientButtonCellTest : public CocoaTest {
 public:
  GradientButtonCellTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<NSButton> view(
        [[NSButton alloc] initWithFrame:frame]);
    view_ = view.get();
    base::scoped_nsobject<GradientButtonCell> cell(
        [[GradientButtonCell alloc] initTextCell:@"Testing"]);
    [view_ setCell:cell.get()];
    [[test_window() contentView] addSubview:view_];
  }

  NSButton* view_;
};

TEST_VIEW(GradientButtonCellTest, view_)

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(GradientButtonCellTest, DisplayWithHover) {
  [[view_ cell] setHoverAlpha:0.0];
  [view_ display];
  [[view_ cell] setHoverAlpha:0.5];
  [view_ display];
  [[view_ cell] setHoverAlpha:1.0];
  [view_ display];
}

// Test hover, mostly to ensure nothing leaks or crashes.
TEST_F(GradientButtonCellTest, Hover) {
  GradientButtonCell* cell = [view_ cell];
  [cell setMouseInside:YES animate:NO];
  EXPECT_EQ([[view_ cell] hoverAlpha], 1.0);

  [cell setMouseInside:NO animate:YES];
  CGFloat alpha1 = [cell hoverAlpha];
  [cell performOnePulseStep];
  CGFloat alpha2 = [cell hoverAlpha];
  EXPECT_TRUE(alpha2 < alpha1);
}

// Tracking rects
TEST_F(GradientButtonCellTest, TrackingRects) {
  GradientButtonCell* cell = [view_ cell];
  EXPECT_FALSE([cell showsBorderOnlyWhileMouseInside]);
  EXPECT_FALSE([cell isMouseInside]);

  [cell setShowsBorderOnlyWhileMouseInside:YES];
  [cell mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_TRUE([cell isMouseInside]);
  [cell mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_FALSE([cell isMouseInside]);

  [cell setShowsBorderOnlyWhileMouseInside:NO];
  EXPECT_FALSE([cell isMouseInside]);

  [cell setShowsBorderOnlyWhileMouseInside:YES];
  [cell setShowsBorderOnlyWhileMouseInside:YES];
  [cell setShowsBorderOnlyWhileMouseInside:NO];
  [cell setShowsBorderOnlyWhileMouseInside:NO];
}

TEST_F(GradientButtonCellTest, ContinuousPulseOnOff) {
  GradientButtonCell* cell = [view_ cell];

  // On/off
  EXPECT_FALSE([cell isPulseStuckOn]);
  [cell setPulseIsStuckOn:YES];
  EXPECT_TRUE([cell isPulseStuckOn]);
  EXPECT_FALSE([cell pulsing]);
  [cell setPulseIsStuckOn:NO];
  EXPECT_FALSE([cell isPulseStuckOn]);

  // On/safeOff
  [cell setPulseIsStuckOn:YES];
  EXPECT_TRUE([cell isPulseStuckOn]);
}

// More for valgrind; we don't confirm state change does anything useful.
TEST_F(GradientButtonCellTest, PulseState) {
  GradientButtonCell* cell = [view_ cell];

  [cell setMouseInside:YES animate:YES];
  // Allow for immediate state changes to keep test unflaky
  EXPECT_TRUE(([cell pulseState] == gradient_button_cell::kPulsingOn) ||
              ([cell pulseState] == gradient_button_cell::kPulsedOn));

  [cell setMouseInside:NO animate:YES];
  // Allow for immediate state changes to keep test unflaky
  EXPECT_TRUE(([cell pulseState] == gradient_button_cell::kPulsingOff) ||
              ([cell pulseState] == gradient_button_cell::kPulsedOff));
}

// Test drawing when first responder, mostly to ensure nothing leaks or
// crashes.
TEST_F(GradientButtonCellTest, FirstResponder) {
  [test_window() makePretendKeyWindowAndSetFirstResponder:view_];
  [view_ display];
}

}  // namespace
