// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/styled_text_field.h"
#import "chrome/browser/ui/cocoa/styled_text_field_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/test/styled_text_field_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

namespace {

// Width of the field so that we don't have to ask |field_| for it all
// the time.
static const CGFloat kWidth(300.0);

class StyledTextFieldTest : public CocoaTest {
 public:
  StyledTextFieldTest() {
    // Make sure this is wide enough to play games with the cell
    // decorations.
    NSRect frame = NSMakeRect(0, 0, kWidth, 30);

    base::scoped_nsobject<StyledTextFieldTestCell> cell(
        [[StyledTextFieldTestCell alloc] initTextCell:@"Testing"]);
    cell_ = cell.get();
    [cell_ setEditable:YES];
    [cell_ setBordered:YES];

    base::scoped_nsobject<StyledTextField> field(
        [[StyledTextField alloc] initWithFrame:frame]);
    field_ = field.get();
    [field_ setCell:cell_];

    [[test_window() contentView] addSubview:field_];
  }

  // Helper to return the field-editor frame being used w/in |field_|.
  NSRect EditorFrame() {
    EXPECT_TRUE([field_ currentEditor]);
    EXPECT_EQ([[field_ subviews] count], 1U);
    if ([[field_ subviews] count] > 0) {
      return [[[field_ subviews] objectAtIndex:0] frame];
    } else {
      // Return something which won't work so the caller can soldier
      // on.
      return NSZeroRect;
    }
  }

  StyledTextField* field_;
  StyledTextFieldTestCell* cell_;
};

// Basic view tests (AddRemove, Display).
TEST_VIEW(StyledTextFieldTest, field_);

// Test that we get the same cell from -cell and
// -styledTextFieldCell.
TEST_F(StyledTextFieldTest, Cell) {
  StyledTextFieldCell* cell = [field_ styledTextFieldCell];
  EXPECT_EQ(cell, [field_ cell]);
  EXPECT_TRUE(cell != nil);
}

// Test that becoming first responder sets things up correctly.
TEST_F(StyledTextFieldTest, FirstResponder) {
  EXPECT_EQ(nil, [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 0U);
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_FALSE(nil == [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 1U);
  EXPECT_TRUE([[field_ currentEditor] isDescendantOf:field_]);
}

TEST_F(StyledTextFieldTest, AvailableDecorationWidth) {
  // A fudge factor to account for how much space the border takes up.
  // The test shouldn't be too dependent on the field's internals, but
  // it also shouldn't let deranged cases fall through the cracks
  // (like nothing available with no text, or everything available
  // with some text).
  const CGFloat kBorderWidth = 20.0;

  // With no contents, almost the entire width is available for
  // decorations.
  [field_ setStringValue:@""];
  CGFloat availableWidth = [field_ availableDecorationWidth];
  EXPECT_LE(availableWidth, kWidth);
  EXPECT_GT(availableWidth, kWidth - kBorderWidth);

  // With minor contents, most of the remaining width is available for
  // decorations.
  NSDictionary* attributes =
      [NSDictionary dictionaryWithObject:[field_ font]
                                  forKey:NSFontAttributeName];
  NSString* string = @"Hello world";
  const NSSize size([string sizeWithAttributes:attributes]);
  [field_ setStringValue:string];
  availableWidth = [field_ availableDecorationWidth];
  EXPECT_LE(availableWidth, kWidth - size.width);
  EXPECT_GT(availableWidth, kWidth - size.width - kBorderWidth);

  // With huge contents, nothing at all is left for decorations.
  string = @"A long string which is surely wider than field_ can hold.";
  [field_ setStringValue:string];
  availableWidth = [field_ availableDecorationWidth];
  EXPECT_LT(availableWidth, 0.0);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(StyledTextFieldTest, Display) {
  [field_ display];

  // Test focused drawing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  [field_ display];
}

// Test that the field editor gets the same bounds when focus is delivered by
// the standard focusing machinery, or by -resetFieldEditorFrameIfNeeded.
TEST_F(StyledTextFieldTest, ResetFieldEditorBase) {
  // Capture the editor frame resulting from the standard focus machinery.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame(EditorFrame());

  // Setting a hint should result in a strictly smaller editor frame.
  EXPECT_EQ(0, [cell_ leftMargin]);
  EXPECT_EQ(0, [cell_ rightMargin]);
  [cell_ setLeftMargin:10];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
  EXPECT_TRUE(NSContainsRect(baseEditorFrame, EditorFrame()));

  // Resetting the margin and using -resetFieldEditorFrameIfNeeded should result
  // in the same frame as the standard focus machinery.
  [cell_ setLeftMargin:0];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSEQ(baseEditorFrame, EditorFrame());
}

// Test that the field editor gets the same bounds when focus is delivered by
// the standard focusing machinery, or by -resetFieldEditorFrameIfNeeded.
TEST_F(StyledTextFieldTest, ResetFieldEditorLeftMargin) {
  const CGFloat kLeftMargin = 20;

  // Start the cell off with a non-zero left margin.
  [cell_ setLeftMargin:kLeftMargin];
  [cell_ setRightMargin:0];

  // Capture the editor frame resulting from the standard focus machinery.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame(EditorFrame());

  // Clearing the margin should result in a strictly larger editor frame.
  [cell_ setLeftMargin:0];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
  EXPECT_TRUE(NSContainsRect(EditorFrame(), baseEditorFrame));

  // Setting the same margin and using -resetFieldEditorFrameIfNeeded should
  // result in the same frame as the standard focus machinery.
  [cell_ setLeftMargin:kLeftMargin];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSEQ(baseEditorFrame, EditorFrame());
}

// Test that the field editor gets the same bounds when focus is delivered by
// the standard focusing machinery, or by -resetFieldEditorFrameIfNeeded.
TEST_F(StyledTextFieldTest, ResetFieldEditorRightMargin) {
  const CGFloat kRightMargin = 20;

  // Start the cell off with a non-zero right margin.
  [cell_ setLeftMargin:0];
  [cell_ setRightMargin:kRightMargin];

  // Capture the editor frame resulting from the standard focus machinery.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  const NSRect baseEditorFrame(EditorFrame());

  // Clearing the margin should result in a strictly larger editor frame.
  [cell_ setRightMargin:0];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSNE(baseEditorFrame, EditorFrame());
  EXPECT_TRUE(NSContainsRect(EditorFrame(), baseEditorFrame));

  // Setting the same margin and using -resetFieldEditorFrameIfNeeded should
  // result in the same frame as the standard focus machinery.
  [cell_ setRightMargin:kRightMargin];
  [field_ resetFieldEditorFrameIfNeeded];
  EXPECT_NSEQ(baseEditorFrame, EditorFrame());
}

}  // namespace
