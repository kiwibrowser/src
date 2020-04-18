// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/draggable_button.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface TestableDraggableButton : DraggableButton {
  NSUInteger dragCount_;
  BOOL wasTriggered_;
}
- (void)trigger:(id)sender;
- (BOOL)wasTriggered;
- (NSUInteger)dragCount;
@end

@implementation TestableDraggableButton
- (id)initWithFrame:(NSRect)frame {
  if ((self = [super initWithFrame:frame])) {
    dragCount_ = 0;
    wasTriggered_ = NO;
  }
  return self;
}
- (void)beginDrag:(NSEvent*)theEvent {
  dragCount_++;
}

- (void)trigger:(id)sender {
  wasTriggered_ = YES;
}

- (BOOL)wasTriggered {
  return wasTriggered_;
}

- (NSUInteger)dragCount {
  return dragCount_;
}
@end

class DraggableButtonTest : public CocoaTest {};

// Make sure the basic case of "click" still works.
TEST_F(DraggableButtonTest, DownUp) {
  base::scoped_nsobject<TestableDraggableButton> button(
      [[TestableDraggableButton alloc]
          initWithFrame:NSMakeRect(0, 0, 500, 500)]);
  [[test_window() contentView] addSubview:button.get()];
  [button setTarget:button];
  [button setAction:@selector(trigger:)];
  EXPECT_FALSE([button wasTriggered]);
  NSEvent* downEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(10,10),
                                                NSLeftMouseDown,
                                                0);
  NSEvent* upEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(10,10),
                                                NSLeftMouseUp,
                                                0);
  [NSApp postEvent:upEvent atStart:YES];
  [test_window() sendEvent:downEvent];
  EXPECT_TRUE([button wasTriggered]);  // confirms target/action fired
}

TEST_F(DraggableButtonTest, DraggableHysteresis) {
  base::scoped_nsobject<TestableDraggableButton> button(
      [[TestableDraggableButton alloc]
          initWithFrame:NSMakeRect(0, 0, 500, 500)]);
  [[test_window() contentView] addSubview:button.get()];
  NSEvent* downEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(10,10),
                                                NSLeftMouseDown,
                                                0);
  NSEvent* firstMove =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(11,11),
                                                NSLeftMouseDragged,
                                                0);
  NSEvent* firstUpEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(11,11),
                                                NSLeftMouseUp,
                                                0);
  NSEvent* secondMove =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(100,100),
                                                NSLeftMouseDragged,
                                                0);
  NSEvent* secondUpEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(100,100),
                                                NSLeftMouseUp,
                                                0);
  // If the mouse only moves one pixel in each direction
  // it should not cause a drag.
  [NSApp postEvent:firstUpEvent atStart:YES];
  [NSApp postEvent:firstMove atStart:YES];
  [button mouseDown:downEvent];
  EXPECT_EQ(0U, [button dragCount]);

  // If the mouse moves > 5 pixels in either direciton
  // it should cause a drag.
  [NSApp postEvent:secondUpEvent atStart:YES];
  [NSApp postEvent:secondMove atStart:YES];
  [button mouseDown:downEvent];
  EXPECT_EQ(1U, [button dragCount]);
}

TEST_F(DraggableButtonTest, ResetState) {
  base::scoped_nsobject<TestableDraggableButton> button(
      [[TestableDraggableButton alloc]
          initWithFrame:NSMakeRect(0, 0, 500, 500)]);
  [[test_window() contentView] addSubview:button.get()];
  NSEvent* downEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(10,10),
                                                NSLeftMouseDown,
                                                0);
  NSEvent* moveEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(100,100),
                                                NSLeftMouseDragged,
                                                0);
  NSEvent* upEvent =
      cocoa_test_event_utils::MouseEventAtPoint(NSMakePoint(100,100),
                                                NSLeftMouseUp,
                                                0);
  // If the mouse moves > 5 pixels in either direciton it should cause a drag.
  [NSApp postEvent:upEvent atStart:YES];
  [NSApp postEvent:moveEvent atStart:YES];
  [button mouseDown:downEvent];

  // The button should not be highlighted after the drag finishes.
  EXPECT_FALSE([[button cell] isHighlighted]);
  EXPECT_EQ(1U, [button dragCount]);

  // We should be able to initiate another drag immediately after the first one.
  [NSApp postEvent:upEvent atStart:YES];
  [NSApp postEvent:moveEvent atStart:YES];
  [button mouseDown:downEvent];
  EXPECT_EQ(2U, [button dragCount]);
  EXPECT_FALSE([[button cell] isHighlighted]);
}
