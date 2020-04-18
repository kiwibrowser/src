// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/menu_button.h"

#import "base/mac/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/clickhold_button_cell.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "ui/base/test/menu_test_observer.h"
#include "ui/events/test/cocoa_test_event_utils.h"

namespace {

class MenuButtonTest : public CocoaTest {
 public:
  MenuButtonTest() {
    NSRect frame = NSMakeRect(0, 0, 50, 30);
    base::scoped_nsobject<MenuButton> button(
        [[MenuButton alloc] initWithFrame:frame]);
    button_ = button;
    base::scoped_nsobject<ClickHoldButtonCell> cell(
        [[ClickHoldButtonCell alloc] initTextCell:@"Testing"]);
    [button_ setCell:cell];
    [[test_window() contentView] addSubview:button_];
  }

  base::scoped_nsobject<NSMenu> CreateMenu() {
    base::scoped_nsobject<NSMenu> menu([[NSMenu alloc] initWithTitle:@""]);
    [menu insertItemWithTitle:@"" action:nil keyEquivalent:@"" atIndex:0];
    [menu insertItemWithTitle:@"foo" action:nil keyEquivalent:@"" atIndex:1];
    [menu insertItemWithTitle:@"bar" action:nil keyEquivalent:@"" atIndex:2];
    [menu insertItemWithTitle:@"baz" action:nil keyEquivalent:@"" atIndex:3];
    return menu;
  }

  NSEvent* MouseEvent(NSEventType type) {
    NSPoint location;
    if (button_) {
      // Offset the button's origin to ensure clicks aren't routed to the
      // border, which may not trigger.
      location = [button_ frame].origin;
      location.x += 10;
      location.y += 10;
    } else {
      location.x = location.y = 0;
    }

    return cocoa_test_event_utils::MouseEventAtPointInWindow(location, type,
                                                             test_window(), 1);
  }

  MenuButton* button_;  // Weak, owned by test_window().
};

TEST_VIEW(MenuButtonTest, button_);

// Test assigning a menu, again mostly to ensure nothing leaks or crashes.
TEST_F(MenuButtonTest, MenuAssign) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  [button_ setAttachedMenu:menu];
  EXPECT_TRUE([button_ attachedMenu]);
}

TEST_F(MenuButtonTest, OpenOnClick) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button_ setAttachedMenu:menu];
  [button_ setOpenMenuOnClick:YES];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  // Should open the menu.
  [button_ performClick:nil];

  EXPECT_TRUE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

void OpenOnClickAndHold(MenuButtonTest* test, BOOL does_open_on_click_hold) {
  base::scoped_nsobject<NSMenu> menu(test->CreateMenu());
  ASSERT_TRUE(menu);

  MenuButton* button = test->button_;

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button setAttachedMenu:menu];
  [button setOpenMenuOnClick:NO];
  [button setOpenMenuOnClickHold:does_open_on_click_hold];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  // Post a click-hold-drag event sequence in reverse. The final drag events
  // needs to have a location that is far enough away from the initial drag
  // point to cause the menu to open.
  NSEvent* event = cocoa_test_event_utils::MouseEventAtPointInWindow(
      NSMakePoint(600, 600), NSLeftMouseUp, test->test_window(), 0);
  [NSApp postEvent:event atStart:YES];

  event = cocoa_test_event_utils::MouseEventAtPointInWindow(
      NSMakePoint(500, 500), NSLeftMouseDragged, test->test_window(), 0);
  [NSApp postEvent:event atStart:YES];

  [NSApp postEvent:test->MouseEvent(NSLeftMouseDragged) atStart:YES];

  // This will cause the tracking loop to start.
  [button mouseDown:test->MouseEvent(NSLeftMouseDown)];

  EXPECT_EQ(does_open_on_click_hold, [observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

// Test classic Mac menu behavior.
TEST_F(MenuButtonTest, OpenOnClickAndHold) {
  OpenOnClickAndHold(this, YES);
}

TEST_F(MenuButtonTest, OpenOnClickAndHoldDisabled) {
  OpenOnClickAndHold(this, NO);
}

TEST_F(MenuButtonTest, OpenOnRightClick) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button_ setAttachedMenu:menu];
  [button_ setOpenMenuOnClick:YES];
  // Right click is enabled.
  [button_ setOpenMenuOnRightClick:YES];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  // Should open the menu.
  NSEvent* event = MouseEvent(NSRightMouseDown);
  [button_ rightMouseDown:event];

  EXPECT_TRUE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

TEST_F(MenuButtonTest, DontOpenOnRightClickWithoutSetRightClick) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button_ setAttachedMenu:menu];
  [button_ setOpenMenuOnClick:YES];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  // Should not open the menu.
  NSEvent* event = MouseEvent(NSRightMouseDown);
  [button_ rightMouseDown:event];

  EXPECT_FALSE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);

  // Should open the menu in this case.
  [button_ performClick:nil];

  EXPECT_TRUE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

TEST_F(MenuButtonTest, OpenOnAccessibilityPerformAction) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button_ setAttachedMenu:menu];
  NSCell* buttonCell = [button_ cell];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  [button_ setOpenMenuOnClick:YES];

  // NSAccessibilityShowMenuAction should not be available but
  // NSAccessibilityPressAction should.
  NSArray* actionNames = [buttonCell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityPressAction]);

  [button_ setOpenMenuOnClick:NO];

  // NSAccessibilityPressAction should not be the one used to open the menu now.
  actionNames = [buttonCell accessibilityActionNames];
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityShowMenuAction]);
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityPressAction]);

  [buttonCell accessibilityPerformAction:NSAccessibilityShowMenuAction];
  EXPECT_TRUE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

TEST_F(MenuButtonTest, OpenOnAccessibilityPerformActionWithSetRightClick) {
  base::scoped_nsobject<NSMenu> menu(CreateMenu());
  ASSERT_TRUE(menu);

  base::scoped_nsobject<MenuTestObserver> observer(
      [[MenuTestObserver alloc] initWithMenu:menu]);
  ASSERT_TRUE(observer);
  [observer setCloseAfterOpening:YES];

  [button_ setAttachedMenu:menu];
  NSCell* buttonCell = [button_ cell];

  EXPECT_FALSE([observer isOpen]);
  EXPECT_FALSE([observer didOpen]);

  [button_ setOpenMenuOnClick:YES];

  // Command to open the menu should not be available.
  NSArray* actionNames = [buttonCell accessibilityActionNames];
  EXPECT_FALSE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [button_ setOpenMenuOnRightClick:YES];

  // Command to open the menu should now become available.
  actionNames = [buttonCell accessibilityActionNames];
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [button_ setOpenMenuOnClick:NO];

  // Command should still be available, even when both click + hold and right
  // click are turned on.
  actionNames = [buttonCell accessibilityActionNames];
  EXPECT_TRUE([actionNames containsObject:NSAccessibilityShowMenuAction]);

  [buttonCell accessibilityPerformAction:NSAccessibilityShowMenuAction];
  EXPECT_TRUE([observer didOpen]);
  EXPECT_FALSE([observer isOpen]);
}

}  // namespace
