// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray/tray_event_filter.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/system/tray/system_tray.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"

namespace ash {

namespace {

class TrayEventFilterTest : public AshTestBase {
 public:
  TrayEventFilterTest() = default;
  ~TrayEventFilterTest() override = default;

  gfx::Point outside_point() {
    gfx::Rect tray_bounds = GetPrimarySystemTray()->GetBoundsInScreen();
    return tray_bounds.bottom_right() + gfx::Vector2d(1, 1);
  }
  ui::PointerEvent outside_event() {
    const base::TimeTicks time = base::TimeTicks::Now();
    return ui::PointerEvent(ui::MouseEvent(
        ui::ET_MOUSE_PRESSED, outside_point(), outside_point(), time, 0, 0));
  }

  gfx::Point inside_point() {
    gfx::Rect tray_bounds = GetPrimarySystemTray()->GetBoundsInScreen();
    return tray_bounds.origin();
  }
  ui::PointerEvent inside_event() {
    const base::TimeTicks time = base::TimeTicks::Now();
    return ui::PointerEvent(ui::MouseEvent(ui::ET_MOUSE_PRESSED, inside_point(),
                                           inside_point(), time, 0, 0));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TrayEventFilterTest);
};

TEST_F(TrayEventFilterTest, ClickingOutsideCloseBubble) {
  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking outside should close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(), nullptr);
  EXPECT_FALSE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingInsideDoesNotCloseBubble) {
  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking inside should not close the bubble
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(inside_event(), inside_point(), nullptr);
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnMenuContainerDoesNotCloseBubble) {
  // Create a menu window and place it in the menu container window.
  std::unique_ptr<aura::Window> menu_window = CreateTestWindow();
  menu_window->set_owned_by_parent(false);
  Shell::GetPrimaryRootWindowController()
      ->GetContainer(kShellWindowId_MenuContainer)
      ->AddChild(menu_window.get());

  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on MenuContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 menu_window.get());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnPopupDoesNotCloseBubble) {
  // Set up a popup window.
  std::unique_ptr<views::Widget> popup_widget =
      CreateTestWidget(nullptr, kShellWindowId_StatusContainer, gfx::Rect());
  std::unique_ptr<aura::Window> popup_window =
      CreateTestWindow(gfx::Rect(), aura::client::WINDOW_TYPE_POPUP);
  popup_window->set_owned_by_parent(false);
  popup_widget->GetNativeView()->AddChild(popup_window.get());
  popup_widget->GetNativeView()->SetProperty(aura::client::kAlwaysOnTopKey,
                                             true);

  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on StatusContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 popup_window.get());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

TEST_F(TrayEventFilterTest, ClickingOnKeyboardContainerDoesNotCloseBubble) {
  // Simulate the virtual keyboard being open. In production the virtual
  // keyboard container only exists while the keyboard is open.
  std::unique_ptr<aura::Window> keyboard_container =
      CreateTestWindow(gfx::Rect(), aura::client::WINDOW_TYPE_NORMAL,
                       kShellWindowId_VirtualKeyboardContainer);
  std::unique_ptr<aura::Window> keyboard_window = CreateTestWindow();
  keyboard_window->set_owned_by_parent(false);
  keyboard_container->AddChild(keyboard_window.get());

  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  EXPECT_TRUE(tray->HasSystemBubble());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());

  // Clicking on KeyboardContainer should not close the bubble.
  TrayEventFilter* filter = tray->tray_event_filter();
  filter->OnPointerEventObserved(outside_event(), outside_point(),
                                 keyboard_window.get());
  EXPECT_TRUE(tray->IsSystemBubbleVisible());
}

}  // namespace
}  // namespace ash
