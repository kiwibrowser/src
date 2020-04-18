// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/controls/hyperlink_button_cell.h"

#import <Cocoa/Cocoa.h>

#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "ui/base/test/cocoa_helper.h"
#include "ui/events/test/cocoa_test_event_utils.h"

namespace ui {

namespace {

class HyperlinkButtonCellTest : public CocoaTest {
 public:
  HyperlinkButtonCellTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<NSButton> view(
        [[NSButton alloc] initWithFrame:frame]);
    view_ = view.get();
    base::scoped_nsobject<HyperlinkButtonCell> cell(
        [[HyperlinkButtonCell alloc] initTextCell:@"Testing"]);
    cell_ = cell.get();
    [view_ setCell:cell_];
    [[test_window() contentView] addSubview:view_];
  }

  void TestCellCustomization(HyperlinkButtonCell* cell) {
    EXPECT_FALSE([cell isBordered]);
    EXPECT_EQ(NSNoCellMask, [cell_ highlightsBy]);
    EXPECT_TRUE([cell showsBorderOnlyWhileMouseInside]);
    EXPECT_TRUE([cell textColor]);
  }

 protected:
  bool HasUnderlineAttribute(NSDictionary* attributes) {
    NSNumber* number = base::mac::ObjCCastStrict<NSNumber>(
        [attributes objectForKey:NSUnderlineStyleAttributeName]);
    return [number unsignedIntegerValue] != 0;
  }

  NSButton* view_;
  HyperlinkButtonCell* cell_;
};

TEST_VIEW(HyperlinkButtonCellTest, view_)

// Tests the three designated intializers.
TEST_F(HyperlinkButtonCellTest, Initializers) {
  TestCellCustomization(cell_);  // |-initTextFrame:|
  base::scoped_nsobject<HyperlinkButtonCell> cell(
      [[HyperlinkButtonCell alloc] init]);
  TestCellCustomization(cell.get());

  // Need to create a dummy archiver to test |-initWithCoder:|.
  NSData* emptyData = [NSKeyedArchiver archivedDataWithRootObject:@""];
  NSCoder* coder =
    [[[NSKeyedUnarchiver alloc] initForReadingWithData:emptyData] autorelease];
  cell.reset([[HyperlinkButtonCell alloc] initWithCoder:coder]);
  TestCellCustomization(cell);
}

// Test set color.
TEST_F(HyperlinkButtonCellTest, SetTextColor) {
  NSColor* textColor = [NSColor redColor];
  EXPECT_NE(textColor, [cell_ textColor]);
  [cell_ setTextColor:textColor];
  EXPECT_EQ(textColor, [cell_ textColor]);
}

// Test mouse events.
TEST_F(HyperlinkButtonCellTest, MouseHover) {
  [[NSCursor disappearingItemCursor] push];  // Set a known state.
  [cell_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ([NSCursor pointingHandCursor], [NSCursor currentCursor]);
  [cell_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_EQ([NSCursor disappearingItemCursor], [NSCursor currentCursor]);
  [NSCursor pop];
}

// Test mouse events when button is disabled. {
TEST_F(HyperlinkButtonCellTest, MouseHoverWhenDisabled) {
  [cell_ setEnabled:NO];

  [[NSCursor disappearingItemCursor] push];  // Set a known state.
  [cell_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_EQ([NSCursor disappearingItemCursor], [NSCursor currentCursor]);

  [cell_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_EQ([NSCursor disappearingItemCursor], [NSCursor currentCursor]);
  [NSCursor pop];
  [NSCursor pop];
}

// Test underline on hover.
TEST_F(HyperlinkButtonCellTest, UnderlineOnHover) {
  // Default is for no underline.
  EXPECT_FALSE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_FALSE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_FALSE(HasUnderlineAttribute([cell_ linkAttributes]));

  // Setting behavior to ALWAYS will result in always having an underline.
  [cell_ setUnderlineBehavior:hyperlink_button_cell::UnderlineBehavior::ALWAYS];
  EXPECT_TRUE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_TRUE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_TRUE(HasUnderlineAttribute([cell_ linkAttributes]));

  // Setting behavior to ON_HOVER will result in only underlining when the
  // mouse is hovering over the link.
  [cell_ setUnderlineBehavior:
      hyperlink_button_cell::UnderlineBehavior::ON_HOVER];
  EXPECT_FALSE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
  EXPECT_TRUE(HasUnderlineAttribute([cell_ linkAttributes]));
  [cell_ mouseExited:cocoa_test_event_utils::ExitEvent()];
  EXPECT_FALSE(HasUnderlineAttribute([cell_ linkAttributes]));
}

TEST_F(HyperlinkButtonCellTest, Copy) {
  base::scoped_nsobject<HyperlinkButtonCell> cell1([[HyperlinkButtonCell alloc]
      initTextCell:@"Cell"]);
  [cell1 setTextColor:[NSColor redColor]];

  base::scoped_nsobject<HyperlinkButtonCell> cell2([cell1 copy]);
  EXPECT_NSEQ([cell1 textColor], [cell2 textColor]);
  [cell1 setTextColor:[NSColor purpleColor]];
  [cell2 setTextColor:[NSColor greenColor]];
}

}  // namespace

}  // namespace ui
