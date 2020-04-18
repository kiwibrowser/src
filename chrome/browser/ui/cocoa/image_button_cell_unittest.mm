// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "chrome/grit/theme_resources.h"

namespace {

class ImageButtonCellTest : public CocoaTest {
 public:
  ImageButtonCellTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<NSButton> view(
        [[NSButton alloc] initWithFrame:frame]);
    view_ = view.get();
    base::scoped_nsobject<ImageButtonCell> cell(
        [[ImageButtonCell alloc] initTextCell:@""]);
    [view_ setCell:cell.get()];
    [[test_window() contentView] addSubview:view_];
  }

  void SetImages() {
    [[view_ cell] setImageID:IDR_BACK
              forButtonState:image_button_cell::kDefaultState];
    [[view_ cell] setImageID:IDR_BACK_H
              forButtonState:image_button_cell::kHoverState];
    [[view_ cell] setImageID:IDR_BACK_P
              forButtonState:image_button_cell::kPressedState];
    [[view_ cell] setImageID:IDR_BACK_D
              forButtonState:image_button_cell::kDisabledState];
  }

  NSButton* view_;
};

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(ImageButtonCellTest, DisplayWithHover) {
  SetImages();
  EXPECT_FALSE([[view_ cell] isMouseInside]);
  [view_ display];
  [[view_ cell] setIsMouseInside:YES];
  [view_ display];

  // Unset the hover image and draw.
  [[view_ cell] setImageID:0
            forButtonState:image_button_cell::kHoverState];
  [view_ display];
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(ImageButtonCellTest, DisplayWithPressed) {
  SetImages();
  EXPECT_FALSE([[view_ cell] isHighlighted]);
  [view_ display];
  [[view_ cell] setHighlighted:YES];
  [view_ display];

  // Unset the pressed image and draw.
  [[view_ cell] setImageID:0
            forButtonState:image_button_cell::kPressedState];
  [view_ display];
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(ImageButtonCellTest, DisplayWithDisabled) {
  SetImages();
  EXPECT_TRUE([[view_ cell] isEnabled]);
  [view_ display];
  [[view_ cell] setEnabled:NO];
  [view_ display];

  // Unset the disabled image and draw.
  [[view_ cell] setImageID:0
            forButtonState:image_button_cell::kDisabledState];
  [view_ display];
}

TEST_F(ImageButtonCellTest, NewImageCausesDisplay) {
  [[view_ cell] setImageID:IDR_FORWARD
            forButtonState:image_button_cell::kDefaultState];
  [view_ display];
  EXPECT_FALSE([view_ needsDisplay]);

  [[view_ cell] setImageID:IDR_FORWARD_D
            forButtonState:image_button_cell::kDefaultState];
  EXPECT_TRUE([view_ needsDisplay]);
}

} // namespace
