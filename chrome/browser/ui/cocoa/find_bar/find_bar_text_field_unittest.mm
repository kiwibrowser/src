// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field.h"
#import "chrome/browser/ui/cocoa/find_bar/find_bar_text_field_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

// Width of the field so that we don't have to ask |field_| for it all
// the time.
static const CGFloat kWidth(300.0);

class FindBarTextFieldTest : public CocoaTest {
 public:
  FindBarTextFieldTest() {
    // Make sure this is wide enough to play games with the cell
    // decorations.
    NSRect frame = NSMakeRect(0, 0, kWidth, 30);
    base::scoped_nsobject<FindBarTextField> field(
        [[FindBarTextField alloc] initWithFrame:frame]);
    field_ = field.get();

    [field_ setStringValue:@"Test test"];
    [[test_window() contentView] addSubview:field_];
  }

  FindBarTextField* field_;
};

// Basic view tests (AddRemove, Display).
TEST_VIEW(FindBarTextFieldTest, field_);

// Test that we have the right cell class.
TEST_F(FindBarTextFieldTest, CellClass) {
  EXPECT_TRUE([[field_ cell] isKindOfClass:[FindBarTextFieldCell class]]);
}

// Test that we get the same cell from -cell and
// -findBarTextFieldCell.
TEST_F(FindBarTextFieldTest, Cell) {
  FindBarTextFieldCell* cell = [field_ findBarTextFieldCell];
  EXPECT_EQ(cell, [field_ cell]);
  EXPECT_TRUE(cell != nil);
}

// Test that becoming first responder sets things up correctly.
TEST_F(FindBarTextFieldTest, FirstResponder) {
  EXPECT_EQ(nil, [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 0U);
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  EXPECT_FALSE(nil == [field_ currentEditor]);
  EXPECT_EQ([[field_ subviews] count], 1U);
  EXPECT_TRUE([[field_ currentEditor] isDescendantOf:field_]);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(FindBarTextFieldTest, Display) {
  [field_ display];

  // Test focussed drawing.
  [test_window() makePretendKeyWindowAndSetFirstResponder:field_];
  [field_ display];
  [test_window() clearPretendKeyWindowAndFirstResponder];

  // Test display of various cell configurations.
  FindBarTextFieldCell* cell = [field_ findBarTextFieldCell];
  [cell setActiveMatch:4 of:5];
  [field_ display];

  [cell clearResults];
  [field_ display];
}

}  // namespace
