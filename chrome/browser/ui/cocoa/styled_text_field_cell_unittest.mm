// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/styled_text_field_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/test/styled_text_field_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

// Width of the field so that we don't have to ask |field_| for it all
// the time.
const CGFloat kWidth(300.0);

class StyledTextFieldCellTest : public CocoaTest {
 public:
  StyledTextFieldCellTest() {
    // Make sure this is wide enough to play games with the cell
    // decorations.
    const NSRect frame = NSMakeRect(0, 0, kWidth, 30);

    base::scoped_nsobject<StyledTextFieldTestCell> cell(
        [[StyledTextFieldTestCell alloc] initTextCell:@"Testing"]);
    cell_ = cell.get();
    [cell_ setEditable:YES];
    [cell_ setBordered:YES];

    base::scoped_nsobject<NSTextField> view(
        [[NSTextField alloc] initWithFrame:frame]);
    view_ = view.get();
    [view_ setCell:cell_];

    [[test_window() contentView] addSubview:view_];
  }

  NSTextField* view_;
  StyledTextFieldTestCell* cell_;
};

// Basic view tests (AddRemove, Display).
TEST_VIEW(StyledTextFieldCellTest, view_);

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(StyledTextFieldCellTest, FocusedDisplay) {
  [view_ display];

  // Test focused drawing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:view_];
  [view_ display];
  [test_window() clearPretendKeyWindowAndFirstResponder];

  // Test display of various cell configurations.
  [cell_ setLeftMargin:5];
  [view_ display];

  [cell_ setRightMargin:15];
  [view_ display];
}

// The editor frame should be slightly inset from the text frame.
TEST_F(StyledTextFieldCellTest, DrawingRectForBounds) {
  const NSRect bounds = [view_ bounds];
  NSRect textFrame = [cell_ textFrameForFrame:bounds];
  NSRect drawingRect = [cell_ drawingRectForBounds:bounds];

  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(textFrame, NSInsetRect(drawingRect, 1, 1)));

  [cell_ setLeftMargin:10];
  textFrame = [cell_ textFrameForFrame:bounds];
  drawingRect = [cell_ drawingRectForBounds:bounds];
  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(textFrame, NSInsetRect(drawingRect, 1, 1)));

  [cell_ setRightMargin:20];
  textFrame = [cell_ textFrameForFrame:bounds];
  drawingRect = [cell_ drawingRectForBounds:bounds];
  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(NSInsetRect(textFrame, 1, 1), drawingRect));

  [cell_ setLeftMargin:0];
  textFrame = [cell_ textFrameForFrame:bounds];
  drawingRect = [cell_ drawingRectForBounds:bounds];
  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(NSInsetRect(textFrame, 1, 1), drawingRect));
}

}  // namespace
