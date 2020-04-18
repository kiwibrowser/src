// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "chrome/browser/ui/cocoa/toolbar/reload_button_cocoa.h"

#include "base/mac/scoped_nsobject.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/image_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "ui/base/test/menu_test_observer.h"
#import "ui/events/test/cocoa_test_event_utils.h"

@interface ReloadButton (TestForwardDeclares)
+ (void)setPendingReloadTimeout:(NSTimeInterval)seconds;
@end

@implementation ReloadButton (TestImplementation)
- (NSMenu*)menuForTesting {
  return menu_;
}
@end

@interface ReloadButtonTarget : NSObject
- (void)anAction:(id)sender;
@end

@implementation ReloadButtonTarget
- (void)anAction:(id)sender {
}
@end

namespace {

class ReloadButtonCocoaTest : public CocoaTest {
 public:
  ReloadButtonCocoaTest() {
    NSRect frame = NSMakeRect(0, 0, 20, 20);
    base::scoped_nsobject<ReloadButton> button(
        [[ReloadButton alloc] initWithFrame:frame]);
    button_ = button;

    // Set things up so unit tests have a reliable baseline.
    [button_ setTag:IDC_RELOAD];
    [button_ awakeFromNib];

    [[test_window() contentView] addSubview:button_];
  }

  bool IsMouseInside() {
    return [[button_ cell] isMouseInside];
  }

  void MouseEnter() {
    [[button_ cell] mouseEntered:cocoa_test_event_utils::EnterEvent()];
  }

  void MouseExit() {
    [[button_ cell] mouseExited:cocoa_test_event_utils::ExitEvent()];
  }

  ReloadButton* button_;  // Weak, owned by test_window().
};

TEST_VIEW(ReloadButtonCocoaTest, button_)

// Test that mouse-tracking is setup and does the right thing.
TEST_F(ReloadButtonCocoaTest, IsMouseInside) {
  EXPECT_FALSE(IsMouseInside());
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  MouseExit();
}

// Verify that multiple clicks do not result in multiple messages to
// the target.
TEST_F(ReloadButtonCocoaTest, IgnoredMultiClick) {
  base::scoped_nsobject<ReloadButtonTarget> target(
      [[ReloadButtonTarget alloc] init]);
  id mock_target = [OCMockObject partialMockForObject:target];
  [button_ setTarget:mock_target];
  [button_ setAction:@selector(anAction:)];

  // Expect the action once.
  [[mock_target expect] anAction:button_];

  const std::pair<NSEvent*,NSEvent*> click_one =
      cocoa_test_event_utils::MouseClickInView(button_, 1);
  const std::pair<NSEvent*,NSEvent*> click_two =
      cocoa_test_event_utils::MouseClickInView(button_, 2);
  [NSApp postEvent:click_one.second atStart:YES];
  [button_ mouseDown:click_one.first];
  [NSApp postEvent:click_two.second atStart:YES];
  [button_ mouseDown:click_two.first];

  [button_ setTarget:nil];
}

TEST_F(ReloadButtonCocoaTest, UpdateTag) {
  [button_ setTag:IDC_STOP];

  [button_ updateTag:IDC_RELOAD];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
  NSString* const reloadToolTip = [button_ toolTip];

  [button_ updateTag:IDC_STOP];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  NSString* const stopToolTip = [button_ toolTip];
  EXPECT_NSNE(reloadToolTip, stopToolTip);

  [button_ updateTag:IDC_RELOAD];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
  EXPECT_NSEQ(reloadToolTip, [button_ toolTip]);
}

// Test that when forcing the mode, it takes effect immediately,
// regardless of whether the mouse is hovering.
TEST_F(ReloadButtonCocoaTest, SetIsLoadingForce) {
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);

  // Changes to stop immediately.
  [button_ setIsLoading:YES force:YES];
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  // Changes to reload immediately.
  [button_ setIsLoading:NO force:YES];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);

  // Changes to stop immediately when the mouse is hovered, and
  // doesn't change when the mouse exits.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:YES force:YES];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  MouseExit();
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  // Changes to reload immediately when the mouse is hovered, and
  // doesn't change when the mouse exits.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:NO force:YES];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
  MouseExit();
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
}

// Test that without force, stop mode is set immediately, but reload
// is affected by the hover status.
TEST_F(ReloadButtonCocoaTest, SetIsLoadingNoForceUnHover) {
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);

  // Changes to stop immediately when the mouse is not hovering.
  [button_ setIsLoading:YES force:NO];
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  // Changes to reload immediately when the mouse is not hovering.
  [button_ setIsLoading:NO force:NO];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);

  // Changes to stop immediately when the mouse is hovered, and
  // doesn't change when the mouse exits.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:YES force:NO];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  MouseExit();
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  // Does not change to reload immediately when the mouse is hovered,
  // changes when the mouse exits.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:NO force:NO];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  MouseExit();
  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
}

// Test that without force, stop mode is set immediately, and reload
// will be set after a timeout.
// TODO(shess): Reenable, http://crbug.com/61485
TEST_F(ReloadButtonCocoaTest, DISABLED_SetIsLoadingNoForceTimeout) {
  // When the event loop first spins, some delayed tracking-area setup
  // is done, which causes -mouseExited: to be called.  Spin it at
  // least once, and dequeue any pending events.
  // TODO(shess): It would be more reasonable to have an MockNSTimer
  // factory for the class to use, which this code could fire
  // directly.
  while ([NSApp nextEventMatchingMask:NSAnyEventMask
                            untilDate:nil
                               inMode:NSDefaultRunLoopMode
                              dequeue:YES]) {
  }

  const NSTimeInterval kShortTimeout = 0.1;
  [ReloadButton setPendingReloadTimeout:kShortTimeout];

  EXPECT_FALSE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);

  // Move the mouse into the button and press it.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:YES force:NO];
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  // Does not change to reload immediately when the mouse is hovered.
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:NO force:NO];
  EXPECT_TRUE(IsMouseInside());
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  EXPECT_TRUE(IsMouseInside());

  // Spin event loop until the timeout passes.
  NSDate* pastTimeout = [NSDate dateWithTimeIntervalSinceNow:2 * kShortTimeout];
  [NSApp nextEventMatchingMask:NSAnyEventMask
                     untilDate:pastTimeout
                        inMode:NSDefaultRunLoopMode
                       dequeue:NO];

  // Mouse is still hovered, button is in reload mode.  If the mouse
  // is no longer hovered, see comment at top of function.
  EXPECT_TRUE(IsMouseInside());
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
}

// Test that pressing stop after reload mode has been requested
// doesn't forward the stop message.
TEST_F(ReloadButtonCocoaTest, StopAfterReloadSet) {
  base::scoped_nsobject<ReloadButtonTarget> target(
      [[ReloadButtonTarget alloc] init]);
  id mock_target = [OCMockObject partialMockForObject:target];
  [button_ setTarget:mock_target];
  [button_ setAction:@selector(anAction:)];

  EXPECT_FALSE(IsMouseInside());

  // Get to stop mode.
  [button_ setIsLoading:YES force:YES];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  EXPECT_TRUE([button_ isEnabled]);

  // Expect the action once.
  [[mock_target expect] anAction:button_];

  // Clicking in stop mode should send the action and transition to
  // reload mode.
  const std::pair<NSEvent*,NSEvent*> click =
      cocoa_test_event_utils::MouseClickInView(button_, 1);
  [NSApp postEvent:click.second atStart:YES];
  [button_ mouseDown:click.first];
  EXPECT_EQ(IDC_RELOAD, [button_ tag]);
  EXPECT_TRUE([button_ isEnabled]);

  // Get back to stop mode.
  [button_ setIsLoading:YES force:YES];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  EXPECT_TRUE([button_ isEnabled]);

  // If hover prevented reload mode immediately taking effect, clicks should do
  // nothing, because the button should be disabled.
  MouseEnter();
  EXPECT_TRUE(IsMouseInside());
  [button_ setIsLoading:NO force:NO];
  EXPECT_EQ(IDC_STOP, [button_ tag]);
  EXPECT_FALSE([button_ isEnabled]);
  [NSApp postEvent:click.second atStart:YES];
  [button_ mouseDown:click.first];
  EXPECT_EQ(IDC_STOP, [button_ tag]);

  [button_ setTarget:nil];
}

TEST_F(ReloadButtonCocoaTest, RightClickMenu) {
  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:[button_ menuForTesting]]);
  [observer setCloseAfterOpening:YES];

  // When the menu is enabled, it should open on right click only.
  [button_ setMenuEnabled:YES];

  NSEvent* event = cocoa_test_event_utils::LeftMouseDownAtPoint(NSZeroPoint);
  [button_ performClick:event];
  EXPECT_FALSE([observer didOpen]);

  event = cocoa_test_event_utils::RightMouseDownAtPoint(NSZeroPoint);
  [button_ rightMouseDown:event];
  EXPECT_TRUE([observer didOpen]);

  [observer setDidOpen:NO];

  // If the menu is disabled, nothing should happen.
  event = cocoa_test_event_utils::RightMouseDownAtPoint(NSZeroPoint);
  [button_ setMenuEnabled:NO];
  [button_ rightMouseDown:event];
  EXPECT_FALSE([observer didOpen]);
}

}  // namespace
