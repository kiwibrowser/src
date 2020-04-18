// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/base_bubble_controller.h"

#import "base/mac/scoped_nsobject.h"
#import "base/mac/scoped_objc_class_swizzler.h"
#import "base/mac/sdk_forward_declarations.h"
#include "base/macros.h"
#import "chrome/browser/ui/cocoa/info_bubble_view.h"
#import "chrome/browser/ui/cocoa/info_bubble_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "ui/base/test/menu_test_observer.h"
#import "ui/events/test/cocoa_test_event_utils.h"

namespace {
const CGFloat kBubbleWindowWidth = 100;
const CGFloat kBubbleWindowHeight = 50;
const CGFloat kAnchorPointX = 400;
const CGFloat kAnchorPointY = 300;

NSWindow* g_key_window = nil;
}  // namespace

// A helper class to swizzle [NSApplication keyWindow].
@interface FakeKeyWindow : NSObject
@property(readonly) NSWindow* keyWindow;
@end

@implementation FakeKeyWindow
- (NSWindow*)keyWindow {
  return g_key_window;
}
@end


class BaseBubbleControllerTest : public CocoaTest {
 public:
  BaseBubbleControllerTest() : controller_(nil) {}

  void SetUp() override {
    bubble_window_.reset([[InfoBubbleWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, kBubbleWindowWidth,
                                       kBubbleWindowHeight)
                  styleMask:NSBorderlessWindowMask
                    backing:NSBackingStoreBuffered
                      defer:NO]);
    [bubble_window_ setAllowedAnimations:0];

    // The bubble controller will release itself when the window closes.
    controller_ = [[BaseBubbleController alloc]
        initWithWindow:bubble_window_
          parentWindow:test_window()
            anchoredAt:NSMakePoint(kAnchorPointX, kAnchorPointY)];
    EXPECT_TRUE([controller_ bubble]);
    EXPECT_EQ(bubble_window_.get(), [controller_ window]);
  }

  void TearDown() override {
    // Close our windows.
    [controller_ close];
    bubble_window_.reset();
    CocoaTest::TearDown();
  }

  // Closing the bubble will autorelease the controller. Give callers a keep-
  // alive to run checks after closing.
  base::scoped_nsobject<BaseBubbleController> ShowBubble() WARN_UNUSED_RESULT {
    base::scoped_nsobject<BaseBubbleController> keep_alive(
        [controller_ retain]);
    EXPECT_FALSE([bubble_window_ isVisible]);
    [controller_ showWindow:nil];
    EXPECT_TRUE([bubble_window_ isVisible]);
    return keep_alive;
  }

  // Fake the key state notification. Because unit_tests is a "daemon" process
  // type, its windows can never become key (nor can the app become active).
  // Instead of the hacks below, one could make a browser_test or transform the
  // process type, but this seems easiest and is best suited to a unit test.
  //
  // Simply post a notification that will cause the controller to call
  // |-windowDidResignKey:|.
  void SimulateKeyStatusChange() {
    NSNotification* notif =
        [NSNotification notificationWithName:NSWindowDidResignKeyNotification
                                      object:[controller_ window]];
    [[NSNotificationCenter defaultCenter] postNotification:notif];
  }

 protected:
  base::scoped_nsobject<InfoBubbleWindow> bubble_window_;
  BaseBubbleController* controller_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseBubbleControllerTest);
};

// Test that kAlignEdgeToAnchorEdge and a left bubble arrow correctly aligns the
// left edge of the buble to the anchor point.
TEST_F(BaseBubbleControllerTest, LeftAlign) {
  [[controller_ bubble] setArrowLocation:info_bubble::kTopLeading];
  [[controller_ bubble] setAlignment:info_bubble::kAlignEdgeToAnchorEdge];
  [controller_ showWindow:nil];

  NSRect frame = [[controller_ window] frame];
  // Make sure the bubble size hasn't changed.
  EXPECT_EQ(frame.size.width, kBubbleWindowWidth);
  EXPECT_EQ(frame.size.height, kBubbleWindowHeight);
  // Make sure the bubble is left aligned.
  EXPECT_EQ(NSMinX(frame), kAnchorPointX);
  EXPECT_GE(NSMaxY(frame), kAnchorPointY);
}

// Test that kAlignEdgeToAnchorEdge and a right bubble arrow correctly aligns
// the right edge of the buble to the anchor point.
TEST_F(BaseBubbleControllerTest, RightAlign) {
  [[controller_ bubble] setArrowLocation:info_bubble::kTopTrailing];
  [[controller_ bubble] setAlignment:info_bubble::kAlignEdgeToAnchorEdge];
  [controller_ showWindow:nil];

  NSRect frame = [[controller_ window] frame];
  // Make sure the bubble size hasn't changed.
  EXPECT_EQ(frame.size.width, kBubbleWindowWidth);
  EXPECT_EQ(frame.size.height, kBubbleWindowHeight);
  // Make sure the bubble is left aligned.
  EXPECT_EQ(NSMaxX(frame), kAnchorPointX);
  EXPECT_GE(NSMaxY(frame), kAnchorPointY);
}

// Test that kAlignArrowToAnchor and a left bubble arrow correctly aligns
// the bubble arrow to the anchor point.
TEST_F(BaseBubbleControllerTest, AnchorAlignLeftArrow) {
  [[controller_ bubble] setArrowLocation:info_bubble::kTopLeading];
  [[controller_ bubble] setAlignment:info_bubble::kAlignArrowToAnchor];
  [controller_ showWindow:nil];

  NSRect frame = [[controller_ window] frame];
  // Make sure the bubble size hasn't changed.
  EXPECT_EQ(frame.size.width, kBubbleWindowWidth);
  EXPECT_EQ(frame.size.height, kBubbleWindowHeight);
  // Make sure the bubble arrow points to the anchor.
  EXPECT_EQ(NSMinX(frame) + info_bubble::kBubbleArrowXOffset +
      roundf(info_bubble::kBubbleArrowWidth / 2.0), kAnchorPointX);
  EXPECT_GE(NSMaxY(frame), kAnchorPointY);
}

// Test that kAlignArrowToAnchor and a right bubble arrow correctly aligns
// the bubble arrow to the anchor point.
TEST_F(BaseBubbleControllerTest, AnchorAlignRightArrow) {
  [[controller_ bubble] setArrowLocation:info_bubble::kTopTrailing];
  [[controller_ bubble] setAlignment:info_bubble::kAlignArrowToAnchor];
  [controller_ showWindow:nil];

  NSRect frame = [[controller_ window] frame];
  // Make sure the bubble size hasn't changed.
  EXPECT_EQ(frame.size.width, kBubbleWindowWidth);
  EXPECT_EQ(frame.size.height, kBubbleWindowHeight);
  // Make sure the bubble arrow points to the anchor.
  EXPECT_EQ(NSMaxX(frame) - info_bubble::kBubbleArrowXOffset -
      floorf(info_bubble::kBubbleArrowWidth / 2.0), kAnchorPointX);
  EXPECT_GE(NSMaxY(frame), kAnchorPointY);
}

// Test that kAlignArrowToAnchor and a center bubble arrow correctly align
// the bubble towards the anchor point.
TEST_F(BaseBubbleControllerTest, AnchorAlignCenterArrow) {
  [[controller_ bubble] setArrowLocation:info_bubble::kTopCenter];
  [[controller_ bubble] setAlignment:info_bubble::kAlignArrowToAnchor];
  [controller_ showWindow:nil];

  NSRect frame = [[controller_ window] frame];
  // Make sure the bubble size hasn't changed.
  EXPECT_EQ(frame.size.width, kBubbleWindowWidth);
  EXPECT_EQ(frame.size.height, kBubbleWindowHeight);
  // Make sure the bubble arrow points to the anchor.
  EXPECT_EQ(NSMidX(frame), kAnchorPointX);
  EXPECT_GE(NSMaxY(frame), kAnchorPointY);
}

// Test that the window is given an initial position before being shown. This
// ensures offscreen initialization is done using correct screen metrics.
TEST_F(BaseBubbleControllerTest, PositionedBeforeShow) {
  // Verify default alignment settings, used when initialized in SetUp().
  EXPECT_EQ(info_bubble::kTopTrailing, [[controller_ bubble] arrowLocation]);
  EXPECT_EQ(info_bubble::kAlignArrowToAnchor, [[controller_ bubble] alignment]);

  // Verify the default frame (positioned relative to the test_window() origin).
  NSRect frame = [[controller_ window] frame];
  EXPECT_EQ(NSMaxX(frame) - info_bubble::kBubbleArrowXOffset -
      floorf(info_bubble::kBubbleArrowWidth / 2.0), kAnchorPointX);
  EXPECT_EQ(NSMaxY(frame), kAnchorPointY);
}

// Tests that when a new window gets key state (and the bubble resigns) that
// the key window changes.
TEST_F(BaseBubbleControllerTest, ResignKeyCloses) {
  base::scoped_nsobject<NSWindow> other_window(
      [[NSWindow alloc] initWithContentRect:NSMakeRect(500, 500, 500, 500)
                                  styleMask:NSTitledWindowMask
                                    backing:NSBackingStoreBuffered
                                      defer:NO]);

  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();
  EXPECT_FALSE([other_window isVisible]);

  [other_window makeKeyAndOrderFront:nil];
  SimulateKeyStatusChange();

  EXPECT_FALSE([bubble_window_ isVisible]);
  EXPECT_TRUE([other_window isVisible]);
}

// Test that clicking outside the window causes the bubble to close if
// shouldCloseOnResignKey is YES.
TEST_F(BaseBubbleControllerTest, LionClickOutsideClosesWithoutContextMenu) {
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();
  NSWindow* window = [controller_ window];

  EXPECT_TRUE([controller_ shouldCloseOnResignKey]);  // Verify default value.
  [controller_ setShouldCloseOnResignKey:NO];
  NSEvent* event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());
  [NSApp sendEvent:event];

  EXPECT_TRUE([window isVisible]);

  event = cocoa_test_event_utils::RightMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());
  [NSApp sendEvent:event];

  EXPECT_TRUE([window isVisible]);

  [controller_ setShouldCloseOnResignKey:YES];
  event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());
  [NSApp sendEvent:event];

  EXPECT_FALSE([window isVisible]);

  [controller_ showWindow:nil]; // Show it again
  EXPECT_TRUE([window isVisible]);
  EXPECT_TRUE([controller_ shouldCloseOnResignKey]);  // Verify.

  event = cocoa_test_event_utils::RightMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());
  [NSApp sendEvent:event];

  EXPECT_FALSE([window isVisible]);
}

// Test that right-clicking the window with displaying a context menu causes
// the bubble  to close.
TEST_F(BaseBubbleControllerTest, LionRightClickOutsideClosesWithContextMenu) {
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();
  NSWindow* window = [controller_ window];

  base::scoped_nsobject<NSMenu> context_menu(
      [[NSMenu alloc] initWithTitle:@""]);
  [context_menu addItemWithTitle:@"ContextMenuTest"
                          action:nil
                   keyEquivalent:@""];

  // Set the menu as the contextual menu of contentView of test_window().
  [[test_window() contentView] setMenu:context_menu];

  base::scoped_nsobject<MenuTestObserver> menu_observer(
      [[MenuTestObserver alloc] initWithMenu:context_menu]);
  [menu_observer setCloseAfterOpening:YES];
  [menu_observer setOpenCallback:^(MenuTestObserver* observer) {
    // Verify bubble's window is closed when contextual menu is open.
    EXPECT_TRUE([observer isOpen]);
    EXPECT_FALSE([window isVisible]);
  }];

  // RightMouseDown in test_window() would close the bubble window and then
  // dispaly the contextual menu.
  NSEvent* event = cocoa_test_event_utils::RightMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());

  EXPECT_FALSE([menu_observer isOpen]);
  EXPECT_FALSE([menu_observer didOpen]);

  [NSApp sendEvent:event];

  // When we got here, menu has already run its RunLoop.
  EXPECT_FALSE([window isVisible]);

  EXPECT_FALSE([menu_observer isOpen]);
  EXPECT_TRUE([menu_observer didOpen]);
}

// Test that the bubble is not dismissed when it has an attached sheet, or when
// a sheet loses key status (since the sheet is not attached when that happens).
TEST_F(BaseBubbleControllerTest, BubbleStaysOpenWithSheet) {
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();

  // Make a dummy NSPanel for the sheet. Don't use [NSOpenPanel openPanel],
  // otherwise a stray FI_TFloatingInputWindow is created which the unit test
  // harness doesn't like.
  base::scoped_nsobject<NSPanel> panel(
      [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 100, 50)
                                 styleMask:NSTitledWindowMask
                                   backing:NSBackingStoreBuffered
                                     defer:NO]);
  EXPECT_FALSE([panel isReleasedWhenClosed]);  // scoped_nsobject releases it.

  // With a NSOpenPanel, we would call -[NSSavePanel beginSheetModalForWindow]
  // here. In 10.9, we would call [NSWindow beginSheet:]. For 10.6, this:
  [[NSApplication sharedApplication] beginSheet:panel
                                 modalForWindow:bubble_window_
                                  modalDelegate:nil
                                 didEndSelector:NULL
                                    contextInfo:NULL];

  EXPECT_TRUE([bubble_window_ isVisible]);
  EXPECT_TRUE([panel isVisible]);
  // Losing key status while there is an attached window should not close the
  // bubble.
  SimulateKeyStatusChange();
  EXPECT_TRUE([bubble_window_ isVisible]);
  EXPECT_TRUE([panel isVisible]);

  // Closing the attached sheet should not close the bubble.
  [[NSApplication sharedApplication] endSheet:panel];
  [panel close];

  EXPECT_FALSE([bubble_window_ attachedSheet]);
  EXPECT_TRUE([bubble_window_ isVisible]);
  EXPECT_FALSE([panel isVisible]);

  // Now that the sheet is gone, a key status change should close the bubble.
  SimulateKeyStatusChange();
  EXPECT_FALSE([bubble_window_ isVisible]);
}

// Tests that a bubble will close when a window enters fullscreen.
TEST_F(BaseBubbleControllerTest, EnterFullscreen) {
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();

  EXPECT_TRUE([bubble_window_ isVisible]);

  // Post the "enter fullscreen" notification.
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:NSWindowWillEnterFullScreenNotification
                        object:test_window()];

  EXPECT_FALSE([bubble_window_ isVisible]);
}

// Tests that a bubble will close when a window exits fullscreen.
TEST_F(BaseBubbleControllerTest, ExitFullscreen) {
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();

  EXPECT_TRUE([bubble_window_ isVisible]);

  // Post the "exit fullscreen" notification.
  NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
  [center postNotificationName:NSWindowWillExitFullScreenNotification
                        object:test_window()];

  EXPECT_FALSE([bubble_window_ isVisible]);
}

// Tests that a bubble will not close when it's becoming a key window.
TEST_F(BaseBubbleControllerTest, StayOnFocus) {
  [controller_ setShouldOpenAsKeyWindow:NO];
  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();

  EXPECT_TRUE([bubble_window_ isVisible]);
  EXPECT_TRUE([controller_ shouldCloseOnResignKey]);  // Verify default value.

  // Make the bubble a key window.
  g_key_window = [controller_ window];
  base::mac::ScopedObjCClassSwizzler swizzler(
      [NSApplication class], [FakeKeyWindow class], @selector(keyWindow));

  // Post the "resign key" notification for another window.
  NSNotification* notif =
      [NSNotification notificationWithName:NSWindowDidResignKeyNotification
                                    object:test_window()];
  [[NSNotificationCenter defaultCenter] postNotification:notif];

  EXPECT_TRUE([bubble_window_ isVisible]);
  g_key_window = nil;
}

// Test that clicking inside a child window of a bubble, does not dismiss the
// bubble, even if shouldCloseOnResignKey is YES.
TEST_F(BaseBubbleControllerTest, MouseDownInChildWindow) {
  [controller_ setShouldCloseOnResignKey:YES];

  base::scoped_nsobject<NSWindow> child_window([[NSWindow alloc]
      initWithContentRect:NSMakeRect(500, 500, 500, 500)
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [bubble_window_ addChildWindow:child_window ordered:NSWindowAbove];

  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();
  ASSERT_TRUE([bubble_window_ isVisible]);

  NSEvent* event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), child_window);
  [NSApp sendEvent:event];
  EXPECT_TRUE([bubble_window_ isVisible]);

  // Clicking outside the bubble on another window should dismiss the bubble.
  event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), test_window());
  [NSApp sendEvent:event];
  EXPECT_FALSE([bubble_window_ isVisible]);
}

// Test that clicking outside the bubble window but on a window with a greater
// level, does not dismiss the bubble, even if shouldCloseOnResignKey is YES.
TEST_F(BaseBubbleControllerTest, MouseDownInPopup) {
  [controller_ setShouldCloseOnResignKey:YES];

  base::scoped_nsobject<NSWindow> other_window([[NSWindow alloc]
      initWithContentRect:NSMakeRect(500, 500, 500, 500)
                styleMask:NSBorderlessWindowMask
                  backing:NSBackingStoreBuffered
                    defer:NO]);
  [other_window setLevel:NSPopUpMenuWindowLevel];

  base::scoped_nsobject<BaseBubbleController> keep_alive = ShowBubble();
  ASSERT_TRUE([bubble_window_ isVisible]);

  NSEvent* event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), other_window);
  [NSApp sendEvent:event];
  EXPECT_TRUE([bubble_window_ isVisible]);

  // Clicking outside the bubble on a normal window should dismiss the bubble.
  [other_window setLevel:NSNormalWindowLevel];
  ASSERT_GT(NSPopUpMenuWindowLevel, NSNormalWindowLevel);

  event = cocoa_test_event_utils::LeftMouseDownAtPointInWindow(
      NSMakePoint(10, 10), other_window);
  [NSApp sendEvent:event];
  EXPECT_FALSE([bubble_window_ isVisible]);
}
