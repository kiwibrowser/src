// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/hover_button.h"

#import <Cocoa/Cocoa.h>

#import "ui/base/test/cocoa_helper.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface TestHoverButton : HoverButton
@property(readwrite, nonatomic) NSRect hitbox;
@end

@implementation TestHoverButton
@synthesize hitbox = hitbox_;

- (void)setHitbox:(NSRect)hitbox {
  hitbox_ = hitbox;
  [self updateTrackingAreas];
}
@end

@interface HoverButtonTestTarget : NSObject
@property(nonatomic, copy) void (^actionHandler)(id);
@end

@implementation HoverButtonTestTarget
@synthesize actionHandler = actionHandler_;

- (void)dealloc {
  [actionHandler_ release];
  [super dealloc];
}

- (IBAction)action:(id)sender {
  actionHandler_(sender);
}
@end

@interface HoverButtonTestDragDelegate : NSObject<HoverButtonDragDelegate>
@property(nonatomic, copy) void (^dragHandler)(HoverButton*, NSEvent*);
@end

@implementation HoverButtonTestDragDelegate
@synthesize dragHandler = dragHandler_;

- (void)dealloc {
  [dragHandler_ release];
  [super dealloc];
}

- (void)beginDragFromHoverButton:(HoverButton*)button event:(NSEvent*)event {
  dragHandler_(button, event);
}
@end

namespace {

class HoverButtonTest : public ui::CocoaTest {
 public:
  HoverButtonTest() {
    NSRect frame = NSMakeRect(0, 0, 20, 20);
    base::scoped_nsobject<TestHoverButton> button(
        [[TestHoverButton alloc] initWithFrame:frame]);
    button_ = button;
    target_.reset([[HoverButtonTestTarget alloc] init]);
    button_.target = target_;
    button_.action = @selector(action:);
    [[test_window() contentView] addSubview:button_];
  }

 protected:
  void HoverAndExpect(HoverState hoverState) {
    EXPECT_EQ(kHoverStateNone, button_.hoverState);
    [button_ mouseEntered:cocoa_test_event_utils::EnterEvent()];
    EXPECT_EQ(hoverState, button_.hoverState);
    [button_ mouseExited:cocoa_test_event_utils::ExitEvent()];
    EXPECT_EQ(kHoverStateNone, button_.hoverState);
  }

  bool HandleMouseDown(NSEvent* mouseDownEvent) {
    __block bool action_sent = false;
    target_.get().actionHandler = ^(id sender) {
      action_sent = true;
      EXPECT_EQ(kHoverStateMouseDown, button_.hoverState);
    };
    [NSApp sendEvent:mouseDownEvent];
    target_.get().actionHandler = nil;
    return action_sent;
  }

  TestHoverButton* button_;  // Weak, owned by test_window().
  base::scoped_nsobject<HoverButtonTestTarget> target_;
};

TEST_VIEW(HoverButtonTest, button_)

TEST_F(HoverButtonTest, Hover) {
  EXPECT_EQ(kHoverStateNone, button_.hoverState);

  // Default
  HoverAndExpect(kHoverStateMouseOver);

  // Tracking disabled
  button_.trackingEnabled = NO;
  HoverAndExpect(kHoverStateNone);
  button_.trackingEnabled = YES;

  // Button disabled
  button_.enabled = NO;
  HoverAndExpect(kHoverStateNone);
  button_.enabled = YES;

  // Back to normal
  HoverAndExpect(kHoverStateMouseOver);
}

TEST_F(HoverButtonTest, Click) {
  EXPECT_EQ(kHoverStateNone, button_.hoverState);
  const auto click = cocoa_test_event_utils::MouseClickInView(button_, 1);

  [NSApp postEvent:click.second atStart:YES];
  EXPECT_TRUE(HandleMouseDown(click.first));

  button_.enabled = NO;
  EXPECT_FALSE(HandleMouseDown(click.first));

  EXPECT_EQ(kHoverStateNone, button_.hoverState);
}

TEST_F(HoverButtonTest, CustomHitbox) {
  NSRect hitbox = button_.frame;
  hitbox.size.width += 10;

  NSPoint inside_hit_point =
      NSMakePoint(NSMaxX(button_.frame) + 5, NSMidY(button_.frame));
  NSPoint outside_hit_point =
      NSMakePoint(inside_hit_point.x + 10, inside_hit_point.y);

  {
    NSRect trackingRect = button_.trackingAreas[0].rect;
    EXPECT_FALSE(NSPointInRect(inside_hit_point, trackingRect));
    EXPECT_FALSE(NSPointInRect(outside_hit_point, trackingRect));
    EXPECT_NE(button_, [button_ hitTest:inside_hit_point]);
    EXPECT_EQ(nil, [button_ hitTest:outside_hit_point]);
  }

  button_.hitbox = hitbox;
  {
    NSRect trackingRect = button_.trackingAreas[0].rect;
    EXPECT_TRUE(NSPointInRect(inside_hit_point, trackingRect));
    EXPECT_FALSE(NSPointInRect(outside_hit_point, trackingRect));
    EXPECT_EQ(button_, [button_ hitTest:inside_hit_point]);
    EXPECT_EQ(nil, [button_ hitTest:outside_hit_point]);
  }

  button_.hitbox = NSZeroRect;
  {
    NSRect trackingRect = button_.trackingAreas[0].rect;
    EXPECT_FALSE(NSPointInRect(inside_hit_point, trackingRect));
    EXPECT_FALSE(NSPointInRect(outside_hit_point, trackingRect));
    EXPECT_NE(button_, [button_ hitTest:inside_hit_point]);
    EXPECT_EQ(nil, [button_ hitTest:outside_hit_point]);
  }
}

TEST_F(HoverButtonTest, DragDelegate) {
  base::scoped_nsobject<HoverButtonTestDragDelegate> dragDelegate(
      [[HoverButtonTestDragDelegate alloc] init]);

  __block bool dragged = false;
  dragDelegate.get().dragHandler = ^(HoverButton* button, NSEvent* event) {
    dragged = true;
  };
  button_.dragDelegate = dragDelegate;

  const auto click = cocoa_test_event_utils::MouseClickInView(button_, 1);
  NSPoint targetPoint = click.first.locationInWindow;
  targetPoint.x += 5;  // *Not* enough to trigger a drag.
  [NSApp postEvent:cocoa_test_event_utils::MouseEventAtPointInWindow(
                       targetPoint, NSEventTypeLeftMouseDragged,
                       [button_ window], 1)
           atStart:NO];
  [NSApp postEvent:click.second atStart:NO];
  EXPECT_TRUE(HandleMouseDown(click.first));
  EXPECT_FALSE(dragged);

  targetPoint.x += 1;  // Now it's enough to trigger a drag.
  [NSApp postEvent:cocoa_test_event_utils::MouseEventAtPointInWindow(
                       targetPoint, NSEventTypeLeftMouseDragged,
                       [button_ window], 1)
           atStart:NO];
  [NSApp postEvent:click.second atStart:NO];
  EXPECT_FALSE(HandleMouseDown(click.first));
  EXPECT_TRUE(dragged);
}
}  // namespace
