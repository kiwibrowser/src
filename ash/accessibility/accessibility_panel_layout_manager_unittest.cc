// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accessibility/accessibility_panel_layout_manager.h"

#include <memory>

#include "ash/shelf/shelf_constants.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {

// Shorten the name for better line wrapping.
constexpr int kPanelHeight = AccessibilityPanelLayoutManager::kPanelHeight;

AccessibilityPanelLayoutManager* GetLayoutManager() {
  aura::Window* container =
      Shell::GetContainer(Shell::GetPrimaryRootWindow(),
                          kShellWindowId_AccessibilityPanelContainer);
  return static_cast<AccessibilityPanelLayoutManager*>(
      container->layout_manager());
}

// Simulates Chrome creating the ChromeVoxPanel widget.
std::unique_ptr<views::Widget> CreateChromeVoxPanel() {
  std::unique_ptr<views::Widget> widget = std::make_unique<views::Widget>();
  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  params.parent = Shell::GetContainer(
      root_window, kShellWindowId_AccessibilityPanelContainer);
  params.activatable = views::Widget::InitParams::ACTIVATABLE_NO;
  params.bounds = gfx::Rect(0, 0, root_window->bounds().width(),
                            root_window->bounds().height());
  widget->Init(params);
  return widget;
}

using AccessibilityPanelLayoutManagerTest = AshTestBase;

TEST_F(AccessibilityPanelLayoutManagerTest, Basics) {
  AccessibilityPanelLayoutManager* layout_manager = GetLayoutManager();
  ASSERT_TRUE(layout_manager);

  // The layout manager doesn't track anything at startup.
  EXPECT_FALSE(layout_manager->panel_window_for_test());

  // Simulate chrome creating the ChromeVox widget. The layout manager starts
  // managing it.
  std::unique_ptr<views::Widget> widget = CreateChromeVoxPanel();
  widget->Show();
  EXPECT_EQ(widget->GetNativeWindow(), layout_manager->panel_window_for_test());

  // The layout manager doesn't track anything after the widget closes.
  widget.reset();
  EXPECT_FALSE(layout_manager->panel_window_for_test());
}

TEST_F(AccessibilityPanelLayoutManagerTest, Shutdown) {
  // Simulate chrome creating the ChromeVox widget.
  std::unique_ptr<views::Widget> widget = CreateChromeVoxPanel();
  widget->Show();

  // Don't close the window.
  widget.release();

  // Ash should not crash if the window is still open at shutdown.
}

TEST_F(AccessibilityPanelLayoutManagerTest, InitialBounds) {
  display::Screen* screen = display::Screen::GetScreen();
  gfx::Rect initial_work_area = screen->GetPrimaryDisplay().work_area();

  // Simulate Chrome creating the ChromeVox window, but don't show it yet.
  std::unique_ptr<views::Widget> widget = CreateChromeVoxPanel();

  // The layout manager has not adjusted the work area yet.
  EXPECT_EQ(screen->GetPrimaryDisplay().work_area(), initial_work_area);

  // Showing the panel causes the layout manager to adjust the panel bounds and
  // the display work area.
  widget->Show();
  gfx::Rect expected_bounds(0, 0, screen->GetPrimaryDisplay().bounds().width(),
                            kPanelHeight);
  EXPECT_EQ(widget->GetNativeWindow()->bounds(), expected_bounds);
  gfx::Rect expected_work_area = initial_work_area;
  expected_work_area.Inset(0, kPanelHeight, 0, 0);
  EXPECT_EQ(screen->GetPrimaryDisplay().work_area(), expected_work_area);
}

TEST_F(AccessibilityPanelLayoutManagerTest, PanelFullscreen) {
  AccessibilityPanelLayoutManager* layout_manager = GetLayoutManager();
  display::Screen* screen = display::Screen::GetScreen();

  std::unique_ptr<views::Widget> widget = CreateChromeVoxPanel();
  widget->Show();

  gfx::Rect expected_work_area = screen->GetPrimaryDisplay().work_area();

  // When the panel is fullscreen it fills the display and does not change the
  // work area.
  layout_manager->SetPanelFullscreen(true);
  EXPECT_EQ(widget->GetNativeWindow()->bounds(),
            screen->GetPrimaryDisplay().bounds());
  EXPECT_EQ(screen->GetPrimaryDisplay().work_area(), expected_work_area);

  // Restoring the panel to default size restores the bounds and does not change
  // the work area.
  layout_manager->SetPanelFullscreen(false);
  gfx::Rect expected_bounds(0, 0, screen->GetPrimaryDisplay().bounds().width(),
                            kPanelHeight);
  EXPECT_EQ(widget->GetNativeWindow()->bounds(), expected_bounds);
  EXPECT_EQ(screen->GetPrimaryDisplay().work_area(), expected_work_area);
}

TEST_F(AccessibilityPanelLayoutManagerTest, DisplayBoundsChange) {
  std::unique_ptr<views::Widget> widget = CreateChromeVoxPanel();
  widget->Show();

  // When the display resolution changes the panel still sits at the top of the
  // screen.
  UpdateDisplay("1234,567");
  display::Screen* screen = display::Screen::GetScreen();
  gfx::Rect expected_bounds(0, 0, screen->GetPrimaryDisplay().bounds().width(),
                            kPanelHeight);
  EXPECT_EQ(widget->GetNativeWindow()->bounds(), expected_bounds);

  gfx::Rect expected_work_area = screen->GetPrimaryDisplay().bounds();
  expected_work_area.Inset(0, kPanelHeight, 0, kShelfSize);
  EXPECT_EQ(screen->GetPrimaryDisplay().work_area(), expected_work_area);
}

}  // namespace
}  // namespace ash
