// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hover_image_menu_button.h"

#include "base/mac/foundation_util.h"
#include "base/macros.h"
#import "testing/gtest_mac.h"
#import "ui/base/cocoa/controls/hover_image_menu_button_cell.h"
#import "ui/base/test/cocoa_helper.h"
#include "ui/events/test/cocoa_test_event_utils.h"

namespace ui {

namespace {

// Test initialization and display of the NSPopUpButton that shows the drop-
// down menu. Don't try to show the menu, since it will block the thread.
class HoverImageMenuButtonTest : public CocoaTest {
 public:
  HoverImageMenuButtonTest() {}

  // CocoaTest override:
  void SetUp() override;

 protected:
  base::scoped_nsobject<HoverImageMenuButton> menu_button_;
  base::scoped_nsobject<NSImage> normal_;
  base::scoped_nsobject<NSImage> pressed_;
  base::scoped_nsobject<NSImage> hovered_;

  DISALLOW_COPY_AND_ASSIGN(HoverImageMenuButtonTest);
};

void HoverImageMenuButtonTest::SetUp() {
  menu_button_.reset(
      [[HoverImageMenuButton alloc] initWithFrame:NSMakeRect(0, 0, 50, 30)
                                        pullsDown:YES]);

  normal_.reset([base::mac::ObjCCastStrict<NSImage>(
      [NSImage imageNamed:NSImageNameStatusAvailable]) retain]);
  pressed_.reset([base::mac::ObjCCastStrict<NSImage>(
      [NSImage imageNamed:NSImageNameStatusUnavailable]) retain]);
  hovered_.reset([base::mac::ObjCCastStrict<NSImage>(
      [NSImage imageNamed:NSImageNameStatusPartiallyAvailable]) retain]);
  [[menu_button_ hoverImageMenuButtonCell] setDefaultImage:normal_];
  [[menu_button_ hoverImageMenuButtonCell] setAlternateImage:pressed_];
  [[menu_button_ hoverImageMenuButtonCell] setHoverImage:hovered_];

  CocoaTest::SetUp();
  [[test_window() contentView] addSubview:menu_button_];
}

}  // namespace

TEST_VIEW(HoverImageMenuButtonTest, menu_button_);

// Tests that the correct image is chosen, depending on the cell's state flags.
TEST_F(HoverImageMenuButtonTest, CheckImagesForState) {
  EXPECT_FALSE([[menu_button_ cell] isHovered]);
  EXPECT_FALSE([[menu_button_ cell] isHighlighted]);
  EXPECT_NSEQ(normal_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];

  [[menu_button_ cell] setHovered:YES];
  EXPECT_TRUE([[menu_button_ cell] isHovered]);
  EXPECT_FALSE([[menu_button_ cell] isHighlighted]);
  EXPECT_NSEQ(hovered_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];

  // Highlighted takes precendece over hover.
  [[menu_button_ cell] setHighlighted:YES];
  EXPECT_TRUE([[menu_button_ cell] isHovered]);
  EXPECT_TRUE([[menu_button_ cell] isHighlighted]);
  EXPECT_NSEQ(pressed_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];

  [[menu_button_ cell] setHovered:NO];
  EXPECT_FALSE([[menu_button_ cell] isHovered]);
  EXPECT_TRUE([[menu_button_ cell] isHighlighted]);
  EXPECT_NSEQ(pressed_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];

  [[menu_button_ cell] setHighlighted:NO];
  EXPECT_FALSE([[menu_button_ cell] isHovered]);
  EXPECT_FALSE([[menu_button_ cell] isHighlighted]);
  EXPECT_NSEQ(normal_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];
}

// Tests that calling the various setXImage functions calls setNeedsDisplay.
TEST_F(HoverImageMenuButtonTest, NewImageCausesDisplay) {
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Uses setDefaultImage rather than setImage to ensure the image goes into the
  // NSPopUpButtonCell's menuItem. It is then accessible using [NSCell image].
  EXPECT_NSEQ(normal_, [[menu_button_ cell] image]);
  [[menu_button_ cell] setDefaultImage:pressed_];
  EXPECT_NSEQ(pressed_, [[menu_button_ cell] image]);
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Highlighting the cell requires a redisplay.
  [[menu_button_ cell] setHighlighted:YES];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // setAlternateImage comes from NSButtonCell. Ensure the added setHover*
  // behaviour matches.
  [[menu_button_ cell] setAlternateImage:normal_];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Setting the same image should not cause a redisplay.
  [[menu_button_ cell] setAlternateImage:normal_];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Unhighlighting requires a redisplay.
  [[menu_button_ cell] setHighlighted:NO];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Changing hover state requires a redisplay.
  [[menu_button_ cell] setHovered:YES];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // setHoverImage comes directly from storage in HoverImageMenuButtonCell.
  [[menu_button_ cell] setHoverImage:normal_];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Setting the same image should not cause a redisplay.
  [[menu_button_ cell] setHoverImage:normal_];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Unhover requires a redisplay.
  [[menu_button_ cell] setHovered:NO];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  // Changing the image while not hovered should not require a redisplay.
  [[menu_button_ cell] setHoverImage:pressed_];
  EXPECT_FALSE([menu_button_ needsDisplay]);
}

// Test that the mouse enter and exit is properly handled, to set hover state.
TEST_F(HoverImageMenuButtonTest, SimulateMouseEnterExit) {
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);
  EXPECT_NSEQ(normal_, [[menu_button_ cell] imageToDraw]);

  [menu_button_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  EXPECT_NSEQ(hovered_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);

  [menu_button_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_TRUE([menu_button_ needsDisplay]);
  EXPECT_NSEQ(normal_, [[menu_button_ cell] imageToDraw]);
  [menu_button_ display];
  EXPECT_FALSE([menu_button_ needsDisplay]);
}

}  // namespace ui
