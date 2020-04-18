// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface FindBarTextFieldCell (ExposedForTesting)
- (NSAttributedString*)resultsAttributedString;
@end

@implementation FindBarTextFieldCell (ExposedForTesting)
- (NSAttributedString*)resultsAttributedString {
  return resultsString_.get();
}
@end

namespace {

// Width of the field so that we don't have to ask |field_| for it all
// the time.
const CGFloat kWidth(300.0);

class FindBarTextFieldCellTest : public CocoaTest {
 public:
  FindBarTextFieldCellTest() {
    // Make sure this is wide enough to play games with the cell
    // decorations.
    const NSRect frame = NSMakeRect(0, 0, kWidth, 30);

    base::scoped_nsobject<FindBarTextFieldCell> cell(
        [[FindBarTextFieldCell alloc] initTextCell:@"Testing"]);
    cell_ = cell;
    [cell_ setEditable:YES];
    [cell_ setBordered:YES];

    base::scoped_nsobject<NSTextField> view(
        [[NSTextField alloc] initWithFrame:frame]);
    view_ = view;
    [view_ setCell:cell_];

    [[test_window() contentView] addSubview:view_];
  }

  NSTextField* view_;
  FindBarTextFieldCell* cell_;
};

// Basic view tests (AddRemove, Display).
TEST_VIEW(FindBarTextFieldCellTest, view_);

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(FindBarTextFieldCellTest, FocusedDisplay) {
  [view_ display];

  // Test focused drawing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:view_];
  [view_ display];
  [test_window() clearPretendKeyWindowAndFirstResponder];

  // Test display of various cell configurations.
  [cell_ setActiveMatch:4 of:30];
  [view_ display];

  [cell_ setActiveMatch:0 of:0];
  [view_ display];

  [cell_ clearResults];
  [view_ display];
}

// Verify that setting and clearing the find results changes the results string
// appropriately.
TEST_F(FindBarTextFieldCellTest, SetAndClearFindResults) {
  [cell_ setActiveMatch:10 of:30];
  base::scoped_nsobject<NSAttributedString> tenString(
      [[cell_ resultsAttributedString] copy]);
  EXPECT_GT([tenString length], 0U);

  [cell_ setActiveMatch:0 of:0];
  base::scoped_nsobject<NSAttributedString> zeroString(
      [[cell_ resultsAttributedString] copy]);
  EXPECT_GT([zeroString length], 0U);
  EXPECT_FALSE([tenString isEqualToAttributedString:zeroString]);

  [cell_ clearResults];
  EXPECT_EQ(0U, [[cell_ resultsAttributedString] length]);
}

TEST_F(FindBarTextFieldCellTest, TextFrame) {
  const NSRect bounds = [view_ bounds];
  NSRect textFrame = [cell_ textFrameForFrame:bounds];
  NSRect cursorFrame = [cell_ textCursorFrameForFrame:bounds];

  // At default settings, everything goes to the text area.
  EXPECT_FALSE(NSIsEmptyRect(textFrame));
  EXPECT_TRUE(NSContainsRect(bounds, textFrame));
  EXPECT_EQ(NSMinX(bounds), NSMinX(textFrame));
  EXPECT_EQ(NSMaxX(bounds), NSMaxX(textFrame));
  EXPECT_NSEQ(cursorFrame, textFrame);

  // Setting an active match leaves text frame to left.
  [cell_ setActiveMatch:4 of:5];
  textFrame = [cell_ textFrameForFrame:bounds];
  cursorFrame = [cell_ textCursorFrameForFrame:bounds];
  EXPECT_FALSE(NSIsEmptyRect(textFrame));
  EXPECT_TRUE(NSContainsRect(bounds, textFrame));
  EXPECT_LT(NSMaxX(textFrame), NSMaxX(bounds));
  EXPECT_NSEQ(cursorFrame, textFrame);
}

// The editor frame should be slightly inset from the text frame.
TEST_F(FindBarTextFieldCellTest, DrawingRectForBounds) {
  const NSRect bounds = [view_ bounds];
  NSRect textFrame = [cell_ textFrameForFrame:bounds];
  NSRect drawingRect = [cell_ drawingRectForBounds:bounds];

  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(textFrame, NSInsetRect(drawingRect, 1, 1)));

  [cell_ setActiveMatch:4 of:5];
  textFrame = [cell_ textFrameForFrame:bounds];
  drawingRect = [cell_ drawingRectForBounds:bounds];
  EXPECT_FALSE(NSIsEmptyRect(drawingRect));
  EXPECT_TRUE(NSContainsRect(textFrame, NSInsetRect(drawingRect, 1, 1)));
}

}  // namespace
