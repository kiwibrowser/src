// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "base/mac/scoped_nsobject.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/toolbar/toolbar_button_cocoa.h"
#import "testing/gtest_mac.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface TestableToolbarButton : ToolbarButton {
 @private
  NSInteger numOfClick_;
  NSInteger lastCommand_;
}

@property(assign, nonatomic) NSInteger numOfClick;
@property(assign, nonatomic) NSInteger lastCommand;

- (id)initWithFrame:(NSRect)frame;
- (void)doAction:(id)sender;
@end

@implementation TestableToolbarButton

@synthesize numOfClick = numOfClick_;
@synthesize lastCommand = lastCommand_;

- (id)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    lastCommand_ = IDC_STOP;
  }
  return self;
}

- (void)doAction:(id)sender {
  lastCommand_ = [sender tag];
  if (lastCommand_ == [self tag])
    ++numOfClick_;
}

- (BOOL)shouldHandleEvent:(NSEvent*)theEvent {
  return handleMiddleClick_;
}

@end

namespace {

class ToolbarButtonTest : public CocoaTest {
 public:
  ToolbarButtonTest() {
    NSRect frame = NSMakeRect(0, 0, 20, 20);
    base::scoped_nsobject<TestableToolbarButton> button(
        [[TestableToolbarButton alloc] initWithFrame:frame]);
    button_ = button.get();

    [button_ setTag:IDC_HOME];
    [button_ setTarget:button_];
    [button_ setAction:@selector(doAction:)];
    [[test_window() contentView] addSubview:button_];

    NSRect bounds = [button_ bounds];
    NSPoint mid_point = NSMakePoint(NSMidX(bounds), NSMidY(bounds));
    NSPoint out_point = NSMakePoint(bounds.origin.x - 10,
                                    bounds.origin.y - 10);
    left_down_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSLeftMouseDown,
                                                  0);
    left_up_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSLeftMouseUp,
                                                  0);
    right_down_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSRightMouseDown,
                                                  0);
    right_up_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSRightMouseUp,
                                                  0);
    other_down_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSOtherMouseDown,
                                                  0);
    other_dragged_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSOtherMouseDragged,
                                                  0);
    other_up_in_view =
        cocoa_test_event_utils::MouseEventAtPoint(mid_point,
                                                  NSOtherMouseUp,
                                                  0);
    other_down_out_view =
        cocoa_test_event_utils::MouseEventAtPoint(out_point,
                                                  NSOtherMouseDown,
                                                  0);
    other_dragged_out_view =
        cocoa_test_event_utils::MouseEventAtPoint(out_point,
                                                  NSOtherMouseDragged,
                                                  0);
    other_up_out_view =
        cocoa_test_event_utils::MouseEventAtPoint(out_point,
                                                  NSOtherMouseUp,
                                                  0);
  }

  TestableToolbarButton* button_;
  NSEvent* left_down_in_view;
  NSEvent* left_up_in_view;
  NSEvent* right_down_in_view;
  NSEvent* right_up_in_view;
  NSEvent* other_down_in_view;
  NSEvent* other_dragged_in_view;
  NSEvent* other_up_in_view;
  NSEvent* other_down_out_view;
  NSEvent* other_dragged_out_view;
  NSEvent* other_up_out_view;
};

TEST_VIEW(ToolbarButtonTest, button_)

TEST_F(ToolbarButtonTest, DoesNotSwallowClicksOnNO) {
  // Middle button being down doesn't swallow right button clicks. But
  // ToolbarButton doesn't handle right button events.
  [button_ otherMouseDown:other_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ rightMouseDown:right_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ rightMouseUp:right_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button being down doesn't swallows left button clicks.
  [button_ otherMouseDown:other_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [NSApp postEvent:left_up_in_view atStart:YES];
  [button_ mouseDown:left_down_in_view];
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, WithoutMouseDownOnNO) {
  // Middle button mouse up without leading mouse down in the view.
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button mouse dragged in the view and up without leading mouse down.
  [button_ otherMouseDragged:other_dragged_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, MouseClickOnNO) {
  // Middle button clicking in the view.
  [button_ otherMouseDown:other_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button clicking outside of the view.
  [button_ otherMouseDown:other_down_out_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_out_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, MouseDraggingOnNO) {
  // Middle button being down in the view and up outside of the view.
  [button_ otherMouseDown:other_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseDragged:other_dragged_out_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_out_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button being down on the button, move to outside and move on it
  // again, then up on the button.
  [button_ otherMouseDown:other_down_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseDragged:other_dragged_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, WithoutMouseDownOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button mouse up without leading mouse down in the view.
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button mouse dragged in the view and up without leading mouse down.
  [button_ otherMouseDragged:other_dragged_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  [button_ otherMouseUp:other_up_in_view];
  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, MouseClickInsideOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button clicking in the view.
  [NSApp postEvent:other_up_in_view atStart:YES];
  [button_ otherMouseDown:other_down_in_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, MouseClickOutsideOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button clicking outside of the view.
  [NSApp postEvent:other_up_out_view atStart:YES];
  [button_ otherMouseDown:other_down_out_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, MouseDraggingOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button being down in the view and up outside of the view.
  [NSApp postEvent:other_up_out_view atStart:YES];
  [NSApp postEvent:other_dragged_out_view atStart:YES];
  [button_ otherMouseDown:other_down_in_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(0, [button_ numOfClick]);
  EXPECT_EQ(IDC_STOP, [button_ lastCommand]);

  // Middle button being down on the button, move to outside and move on it
  // again, then up on the button.
  [NSApp postEvent:other_up_in_view atStart:YES];
  [NSApp postEvent:other_dragged_in_view atStart:YES];
  [NSApp postEvent:other_dragged_out_view atStart:YES];
  [button_ otherMouseDown:other_down_in_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, DoesSwallowRightClickOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button being down should swallow right button clicks.
  [NSApp postEvent:other_up_in_view atStart:YES];
  [NSApp postEvent:right_up_in_view atStart:YES];
  [NSApp postEvent:right_down_in_view atStart:YES];
  [button_ otherMouseDown:other_down_in_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
}

TEST_F(ToolbarButtonTest, DoesSwallowLeftClickOnYES) {
  // Enable middle button handling.
  [button_ setHandleMiddleClick:YES];

  // Middle button being down swallows left button clicks.
  [NSApp postEvent:other_up_in_view atStart:YES];
  [NSApp postEvent:left_up_in_view atStart:YES];
  [NSApp postEvent:left_down_in_view atStart:YES];
  [button_ otherMouseDown:other_down_in_view];

  EXPECT_EQ(NSOffState, [button_ state]);
  EXPECT_EQ(1, [button_ numOfClick]);
  EXPECT_EQ(IDC_HOME, [button_ lastCommand]);
}

}  // namespace
