// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_controller.h"

#include "ash/display/screen_orientation_controller.h"
#include "ash/display/screen_orientation_controller_test_api.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/screen_util.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/system/overview/overview_button_tray.h"
#include "ash/system/status_area_widget.h"
#include "ash/system/status_area_widget_test_helper.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/splitview/split_view_divider.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/stl_util.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/base/hit_test.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/window_util.h"

namespace ash {

class SplitViewControllerTest : public AshTestBase {
 public:
  SplitViewControllerTest() = default;
  ~SplitViewControllerTest() override = default;

  // test::AshTestBase:
  void SetUp() override {
    AshTestBase::SetUp();
    // Avoid TabletModeController::OnGetSwitchStates() from disabling tablet
    // mode.
    base::RunLoop().RunUntilIdle();
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(true);
  }

  aura::Window* CreateWindow(
      const gfx::Rect& bounds,
      aura::client::WindowType type = aura::client::WINDOW_TYPE_NORMAL) {
    aura::Window* window = CreateTestWindowInShellWithDelegateAndType(
        new SplitViewTestWindowDelegate, type, -1, bounds);
    return window;
  }

  aura::Window* CreateNonSnappableWindow(const gfx::Rect& bounds) {
    aura::Window* window = CreateWindow(bounds);
    window->SetProperty(aura::client::kResizeBehaviorKey,
                        ui::mojom::kResizeBehaviorNone);
    return window;
  }

  void EndSplitView() { split_view_controller()->EndSplitView(); }

  void ToggleOverview() {
    Shell::Get()->window_selector_controller()->ToggleOverview();
  }

  void LongPressOnOverivewButtonTray() {
    ui::GestureEvent event(0, 0, 0, base::TimeTicks(),
                           ui::GestureEventDetails(ui::ET_GESTURE_LONG_PRESS));
    StatusAreaWidgetTestHelper::GetStatusAreaWidget()
        ->overview_button_tray()
        ->OnGestureEvent(&event);
  }

  std::vector<aura::Window*> GetWindowsInOverviewGrids() {
    return Shell::Get()
        ->window_selector_controller()
        ->GetWindowsListInOverviewGridsForTesting();
  }

  SplitViewController* split_view_controller() {
    return Shell::Get()->split_view_controller();
  }

  SplitViewDivider* split_view_divider() {
    return split_view_controller()->split_view_divider();
  }

  OrientationLockType screen_orientation() {
    return split_view_controller()->GetCurrentScreenOrientation();
  }

  int divider_position() { return split_view_controller()->divider_position(); }

 private:
  class SplitViewTestWindowDelegate : public aura::test::TestWindowDelegate {
   public:
    SplitViewTestWindowDelegate() = default;
    ~SplitViewTestWindowDelegate() override = default;

    // aura::test::TestWindowDelegate:
    void OnWindowDestroying(aura::Window* window) override { window->Hide(); }
    void OnWindowDestroyed(aura::Window* window) override { delete this; }
  };

  DISALLOW_COPY_AND_ASSIGN(SplitViewControllerTest);
};

class TestWindowStateDelegate : public wm::WindowStateDelegate {
 public:
  TestWindowStateDelegate() = default;
  ~TestWindowStateDelegate() override = default;

  // wm::WindowStateDelegate:
  void OnDragStarted(int component) override { drag_in_progress_ = true; }
  void OnDragFinished(bool cancel, const gfx::Point& location) override {
    drag_in_progress_ = false;
  }

  bool drag_in_progress() { return drag_in_progress_; }

 private:
  bool drag_in_progress_ = false;
  DISALLOW_COPY_AND_ASSIGN(TestWindowStateDelegate);
};

// Tests the basic functionalities.
TEST_F(SplitViewControllerTest, Basic) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  EXPECT_EQ(split_view_controller()->state(), SplitViewController::NO_SNAP);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_NE(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(window1->GetBoundsInScreen(),
            split_view_controller()->GetSnappedWindowBoundsInScreen(
                window1.get(), SplitViewController::LEFT));

  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());
  EXPECT_NE(split_view_controller()->right_window(), window1.get());
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(window2->GetBoundsInScreen(),
            split_view_controller()->GetSnappedWindowBoundsInScreen(
                window2.get(), SplitViewController::RIGHT));

  EndSplitView();
  EXPECT_EQ(split_view_controller()->state(), SplitViewController::NO_SNAP);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
}

// Tests that the default snapped window is the first window that gets snapped.
TEST_F(SplitViewControllerTest, DefaultSnappedWindow) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(window1.get(), split_view_controller()->GetDefaultSnappedWindow());

  EndSplitView();
  split_view_controller()->SnapWindow(window2.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(window2.get(), split_view_controller()->GetDefaultSnappedWindow());
}

// Tests that if there are two snapped windows, closing one of them will open
// overview window grid on the closed window side of the screen. If there is
// only one snapped windows, closing the snapped window will end split view mode
// and adjust the overview window grid bounds if the overview mode is active at
// that moment.
TEST_F(SplitViewControllerTest, WindowCloseTest) {
  // 1 - First test one snapped window scenario.
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window0(CreateWindow(bounds));
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  split_view_controller()->SnapWindow(window0.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  // Closing this snapped window should exit split view mode.
  window0.reset();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  // 2 - Then test two snapped windows scenario.
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::LEFT);

  // Closing one of the two snapped windows will not end split view mode.
  window1.reset();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::RIGHT_SNAPPED);
  // Since left window was closed, its default snap position changed to RIGHT.
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::RIGHT);
  // Window grid is showing no recent items, and has no windows, but it is still
  // available.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Now close the other snapped window.
  window2.reset();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  EXPECT_EQ(split_view_controller()->state(), SplitViewController::NO_SNAP);
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // 3 - Then test the scenario with more than two windows.
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window4(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window5(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window3.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window4.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::LEFT);

  // Close one of the snapped windows.
  window4.reset();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::LEFT);
  // Now overview window grid can be opened.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Close the other snapped window.
  window3.reset();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  EXPECT_EQ(split_view_controller()->state(), SplitViewController::NO_SNAP);
  // Test the overview winow grid should still open.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
}

// Tests that if there are two snapped windows, minimizing one of them will open
// overview window grid on the minimized window side of the screen. If there is
// only one snapped windows, minimizing the sanpped window will end split view
// mode and adjust the overview window grid bounds if the overview mode is
// active at that moment.
TEST_F(SplitViewControllerTest, MinimizeWindowTest) {
  const gfx::Rect bounds(0, 0, 400, 400);

  // 1 - First test one snapped window scenario.
  std::unique_ptr<aura::Window> window0(CreateWindow(bounds));
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  split_view_controller()->SnapWindow(window0.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  wm::WMEvent minimize_event(wm::WM_EVENT_MINIMIZE);
  wm::GetWindowState(window0.get())->OnWMEvent(&minimize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  // 2 - Then test the scenario that has 2 or more windows.
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::LEFT);

  // Minimizing one of the two snapped windows will not end split view mode.
  wm::GetWindowState(window1.get())->OnWMEvent(&minimize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::RIGHT_SNAPPED);
  // Since left window was minimized, its default snap position changed to
  // RIGHT.
  EXPECT_EQ(split_view_controller()->default_snap_position(),
            SplitViewController::RIGHT);
  // The overview window grid will open.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Now minimize the other snapped window.
  wm::GetWindowState(window2.get())->OnWMEvent(&minimize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  EXPECT_EQ(split_view_controller()->state(), SplitViewController::NO_SNAP);
  // The overview window grid is still open.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
}

// Tests that if one of the snapped window gets maximized / full-screened, the
// split view mode ends.
TEST_F(SplitViewControllerTest, WindowStateChangeTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  // 1 - First test one snapped window scenario.
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  wm::WMEvent maximize_event(wm::WM_EVENT_MAXIMIZE);
  wm::GetWindowState(window1.get())->OnWMEvent(&maximize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  wm::WMEvent fullscreen_event(wm::WM_EVENT_FULLSCREEN);
  wm::GetWindowState(window1.get())->OnWMEvent(&fullscreen_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  // 2 - Then test two snapped window scenario.
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  // Maximize one of the snapped window will end the split view mode.
  wm::GetWindowState(window1.get())->OnWMEvent(&maximize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  // Full-screen one of the snapped window will also end the split view mode.
  wm::GetWindowState(window1.get())->OnWMEvent(&fullscreen_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  // 3 - Test the scenario that part of the screen is a snapped window and part
  // of the screen is the overview window grid.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  ToggleOverview();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  // Maximize the snapped window will end the split view mode and overview mode.
  wm::GetWindowState(window1.get())->OnWMEvent(&maximize_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  ToggleOverview();
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);

  // Fullscreen the snapped window will end the split view mode and overview
  // mode.
  wm::GetWindowState(window1.get())->OnWMEvent(&fullscreen_event);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
}

// Tests that if split view mode is active, activate another window will snap
// the window to the non-default side of the screen.
TEST_F(SplitViewControllerTest, WindowActivationTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);

  wm::ActivateWindow(window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);

  wm::ActivateWindow(window3.get());
  EXPECT_EQ(split_view_controller()->right_window(), window3.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
}

// Tests that if split view mode and overview mode are active at the same time,
// i.e., half of the screen is occupied by a snapped window and half of the
// screen is occupied by the overview windows grid, the next activatable window
// will be picked to snap when exiting the overview mode.
TEST_F(SplitViewControllerTest, ExitOverviewTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), false);

  ToggleOverview();
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->IsSplitViewModeActive(), true);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());

  ToggleOverview();
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->right_window(), window3.get());
}

// Tests that if split view mode is active when entering overview, the overview
// windows grid should show in the non-default side of the screen, and the
// default snapped window should not be shown in the overview window grid.
TEST_F(SplitViewControllerTest, EnterOverviewTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->GetDefaultSnappedWindow(), window1.get());

  ToggleOverview();
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_FALSE(
      base::ContainsValue(GetWindowsInOverviewGrids(),
                          split_view_controller()->GetDefaultSnappedWindow()));
}

// Tests that the split divider was created when the split view mode is active
// and destroyed when the split view mode is ended. The split divider should be
// always above the two snapped windows.
TEST_F(SplitViewControllerTest, SplitDividerBasicTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  EXPECT_TRUE(!split_view_divider());
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_TRUE(split_view_divider());
  EXPECT_TRUE(split_view_divider()->divider_widget()->IsAlwaysOnTop());
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_TRUE(split_view_divider());
  EXPECT_TRUE(split_view_divider()->divider_widget()->IsAlwaysOnTop());

  // Test that activating an non-snappable window ends the split view mode.
  std::unique_ptr<aura::Window> window3(CreateNonSnappableWindow(bounds));
  wm::ActivateWindow(window3.get());
  EXPECT_FALSE(split_view_divider());
}

// Verifys that the bounds of the two windows in splitview are as expected.
TEST_F(SplitViewControllerTest, SplitDividerWindowBounds) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  ASSERT_TRUE(split_view_divider());

  // Verify with two freshly snapped windows are roughly the same width (off by
  // one pixel at most due to the display maybe being even and the divider being
  // a fixed odd pixel width).
  int window1_width = window1->GetBoundsInScreen().width();
  int window2_width = window2->GetBoundsInScreen().width();
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  const int screen_width =
      screen_util::GetDisplayWorkAreaBoundsInParent(window1.get()).width();
  EXPECT_NEAR(window1_width, window2_width, 1);
  EXPECT_EQ(screen_width,
            window1_width + divider_bounds.width() + window2_width);

  // Drag the divider to a position two thirds of the screen size. Verify window
  // 1 is wider than window 2.
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().DragMouseTo(screen_width * 0.67f, 0);
  window1_width = window1->GetBoundsInScreen().width();
  window2_width = window2->GetBoundsInScreen().width();
  const int old_window1_width = window1_width;
  const int old_window2_width = window2_width;
  EXPECT_GT(window1_width, 2 * window2_width);
  EXPECT_EQ(screen_width,
            window1_width + divider_bounds.width() + window2_width);

  // Drag the divider to a position close to two thirds of the screen size.
  // Verify the divider snaps to two thirds of the screen size, and the windows
  // remain the same size as previously.
  divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().DragMouseTo(screen_width * 0.7f, 0);
  window1_width = window1->GetBoundsInScreen().width();
  window2_width = window2->GetBoundsInScreen().width();
  EXPECT_EQ(window1_width, old_window1_width);
  EXPECT_EQ(window2_width, old_window2_width);

  // Drag the divider to a position one third of the screen size. Verify window
  // 1 is wider than window 2.
  divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().DragMouseTo(screen_width * 0.33f, 0);
  window1_width = window1->GetBoundsInScreen().width();
  window2_width = window2->GetBoundsInScreen().width();
  EXPECT_GT(window2_width, 2 * window1_width);
  EXPECT_EQ(screen_width,
            window1_width + divider_bounds.width() + window2_width);

  // Verify that the left window from dragging the divider to two thirds of the
  // screen size is roughly the same size as the right window after dragging the
  // divider to one third of the screen size, and vice versa.
  EXPECT_NEAR(window1_width, old_window2_width, 1);
  EXPECT_NEAR(window2_width, old_window1_width, 1);
}

// Tests that the bounds of the snapped windows and divider are adjusted when
// the screen display configuration changes.
TEST_F(SplitViewControllerTest, DisplayConfigurationChangeTest) {
  UpdateDisplay("407x400");
  const gfx::Rect bounds(0, 0, 200, 200);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);

  const gfx::Rect bounds_window1 = window1->GetBoundsInScreen();
  const gfx::Rect bounds_window2 = window2->GetBoundsInScreen();
  const gfx::Rect bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test that |window1| and |window2| has the same width and height after snap.
  EXPECT_NEAR(bounds_window1.width(), bounds_window2.width(), 1);
  EXPECT_EQ(bounds_window1.height(), bounds_window2.height());
  EXPECT_EQ(bounds_divider.height(), bounds_window1.height());

  // Test that |window1|, divider, |window2| are aligned properly.
  EXPECT_EQ(bounds_divider.x(), bounds_window1.x() + bounds_window1.width());
  EXPECT_EQ(bounds_window2.x(), bounds_divider.x() + bounds_divider.width());

  // Now change the display configuration.
  UpdateDisplay("507x500");
  const gfx::Rect new_bounds_window1 = window1->GetBoundsInScreen();
  const gfx::Rect new_bounds_window2 = window2->GetBoundsInScreen();
  const gfx::Rect new_bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test that the new bounds are different with the old ones.
  EXPECT_NE(bounds_window1, new_bounds_window1);
  EXPECT_NE(bounds_window2, new_bounds_window2);
  EXPECT_NE(bounds_divider, new_bounds_divider);

  // Test that |window1|, divider, |window2| are still aligned properly.
  EXPECT_EQ(new_bounds_divider.x(),
            new_bounds_window1.x() + new_bounds_window1.width());
  EXPECT_EQ(new_bounds_window2.x(),
            new_bounds_divider.x() + new_bounds_divider.width());
}

// Verify the left and right windows get swapped when SwapWindows is called or
// the divider is double tapped.
TEST_F(SplitViewControllerTest, SwapWindows) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());

  gfx::Rect left_bounds = window1->GetBoundsInScreen();
  gfx::Rect right_bounds = window2->GetBoundsInScreen();

  // Verify that after swapping windows, the windows and their bounds have been
  // swapped.
  split_view_controller()->SwapWindows();
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_EQ(left_bounds, window2->GetBoundsInScreen());
  EXPECT_EQ(right_bounds, window1->GetBoundsInScreen());

  // End split view mode and snap the window to RIGHT first, verify the function
  // SwapWindows() still works properly.
  EndSplitView();
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  split_view_controller()->SnapWindow(window2.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());

  left_bounds = window2->GetBoundsInScreen();
  right_bounds = window1->GetBoundsInScreen();

  split_view_controller()->SwapWindows();
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());
  EXPECT_EQ(left_bounds, window1->GetBoundsInScreen());
  EXPECT_EQ(right_bounds, window2->GetBoundsInScreen());

  // Perform a double tap on the divider center.
  const gfx::Point divider_center =
      split_view_divider()
          ->GetDividerBoundsInScreen(false /* is_dragging */)
          .CenterPoint();
  GetEventGenerator().set_current_location(divider_center);
  GetEventGenerator().DoubleClickLeftButton();

  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_EQ(left_bounds, window2->GetBoundsInScreen());
  EXPECT_EQ(right_bounds, window1->GetBoundsInScreen());
}

// Verifies that by long pressing on the overview button tray, split view gets
// activated iff we have two or more windows in the mru list.
TEST_F(SplitViewControllerTest, LongPressEntersSplitView) {
  // Verify that with no active windows, split view does not get activated.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  wm::ActivateWindow(window1.get());

  // Verify that with only one window, split view does not get activated.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // Verify that with two windows, split view gets activated.
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  wm::ActivateWindow(window2.get());
  LongPressOnOverivewButtonTray();
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
}

// Verify that when in split view mode with either one snapped or two snapped
// windows, split view mode gets exited when the overview button gets a long
// press event.
TEST_F(SplitViewControllerTest, LongPressExitsSplitView) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  wm::ActivateWindow(window2.get());
  wm::ActivateWindow(window1.get());

  // Snap |window1| to the left.
  ToggleOverview();
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  ASSERT_TRUE(split_view_controller()->IsSplitViewModeActive());

  // Verify that by long pressing on the overview button tray with left snapped
  // window, split view mode gets exited and the left window (|window1|) is the
  // current active window.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_EQ(window1.get(), wm::GetActiveWindow());

  // Snap |window1| to the right.
  ToggleOverview();
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  ASSERT_TRUE(split_view_controller()->IsSplitViewModeActive());

  // Verify that by long pressing on the overview button tray with right snapped
  // window, split view mode gets exited and the right window (|window1|) is the
  // current active window.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_EQ(window1.get(), wm::GetActiveWindow());

  // Snap two windows and activate the left window, |window1|.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  wm::ActivateWindow(window1.get());
  ASSERT_TRUE(split_view_controller()->IsSplitViewModeActive());

  // Verify that by long pressing on the overview button tray with two snapped
  // windows, split view mode gets exited.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(window1.get(), wm::GetActiveWindow());

  // Snap two windows and activate the right window, |window2|.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  wm::ActivateWindow(window2.get());
  ASSERT_TRUE(split_view_controller()->IsSplitViewModeActive());

  // Verify that by long pressing on the overview button tray with two snapped
  // windows, split view mode gets exited, and the activated window in splitview
  // is the current active window.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(window2.get(), wm::GetActiveWindow());
}

// Verify that if a window with a transient child which is not snappable is
// activated, and the the overview tray is long pressed, we will enter splitview
// with the transient parent snapped.
TEST_F(SplitViewControllerTest, LongPressEntersSplitViewWithTransientChild) {
  // Add two windows with one being a transient child of the first.
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> parent(CreateWindow(bounds));
  std::unique_ptr<aura::Window> child(
      CreateWindow(bounds, aura::client::WINDOW_TYPE_POPUP));
  ::wm::AddTransientChild(parent.get(), child.get());
  ::wm::ActivateWindow(parent.get());
  ::wm::ActivateWindow(child.get());

  // Verify that long press on the overview button will not enter split view
  // mode, as there needs to be two non-transient child windows.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // Add a third window. Focus the transient child.
  std::unique_ptr<aura::Window> third_window(CreateWindow(bounds));
  ::wm::ActivateWindow(third_window.get());
  ::wm::ActivateWindow(parent.get());
  ::wm::ActivateWindow(child.get());

  // Verify that long press will snap the focused transient child's parent.
  LongPressOnOverivewButtonTray();
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->GetDefaultSnappedWindow(), parent.get());
}

TEST_F(SplitViewControllerTest, LongPressExitsSplitViewWithTransientChild) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> left_window(CreateWindow(bounds));
  std::unique_ptr<aura::Window> right_window(CreateWindow(bounds));
  wm::ActivateWindow(left_window.get());
  wm::ActivateWindow(right_window.get());

  ToggleOverview();
  split_view_controller()->SnapWindow(left_window.get(),
                                      SplitViewController::LEFT);
  split_view_controller()->SnapWindow(right_window.get(),
                                      SplitViewController::RIGHT);
  ASSERT_TRUE(split_view_controller()->IsSplitViewModeActive());

  // Add a transient child to |right_window|, and activate it.
  aura::Window* transient_child =
      aura::test::CreateTestWindowWithId(0, right_window.get());
  ::wm::AddTransientChild(right_window.get(), transient_child);
  wm::ActivateWindow(transient_child);

  // Verify that by long pressing on the overview button tray, split view mode
  // gets exited and the window which contained |transient_child| is the
  // current active window.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_EQ(right_window.get(), wm::GetActiveWindow());
}

// Verify that split view mode get activated when long pressing on the overview
// button while in overview mode iff we have more than two windows in the mru
// list.
TEST_F(SplitViewControllerTest, LongPressInOverviewMode) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  wm::ActivateWindow(window1.get());

  ToggleOverview();
  ASSERT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  ASSERT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // Nothing happens if there is only one window.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // Verify that with two windows, a long press on the overview button tray will
  // enter splitview.
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  wm::ActivateWindow(window2.get());
  ToggleOverview();
  ASSERT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  ASSERT_FALSE(split_view_controller()->IsSplitViewModeActive());

  LongPressOnOverivewButtonTray();
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(window2.get(), split_view_controller()->left_window());
}

TEST_F(SplitViewControllerTest, LongPressWithUnsnappableWindow) {
  // Add one unsnappable window and two regular windows.
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> unsnappable_window(
      CreateNonSnappableWindow(bounds));
  ASSERT_FALSE(split_view_controller()->IsSplitViewModeActive());
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  wm::ActivateWindow(window2.get());
  wm::ActivateWindow(window3.get());
  wm::ActivateWindow(unsnappable_window.get());
  ASSERT_EQ(unsnappable_window.get(), wm::GetActiveWindow());

  // Verify split view is not activated when long press occurs when active
  // window is unsnappable.
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // Verify split view is not activated when long press occurs in overview mode
  // and the most recent window is unsnappable.
  ToggleOverview();
  ASSERT_TRUE(
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList().size() > 0);
  ASSERT_EQ(unsnappable_window.get(),
            Shell::Get()->mru_window_tracker()->BuildWindowForCycleList()[0]);
  LongPressOnOverivewButtonTray();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
}

// Test the rotation functionalities in split view mode.
TEST_F(SplitViewControllerTest, RotationTest) {
  UpdateDisplay("807x407");
  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  display::test::ScopedSetInternalDisplayId set_internal(display_manager,
                                                         display_id);
  ScreenOrientationControllerTestApi test_api(
      Shell::Get()->screen_orientation_controller());

  // Set the screen orientation to LANDSCAPE_PRIMARY.
  test_api.SetDisplayRotation(display::Display::ROTATE_0,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kLandscapePrimary);

  const gfx::Rect bounds(0, 0, 200, 200);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  gfx::Rect bounds_window1 = window1->GetBoundsInScreen();
  gfx::Rect bounds_window2 = window2->GetBoundsInScreen();
  gfx::Rect bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test |window1|, divider and |window2| are aligned horizontally.
  // |window1| is on the left, then the divider, and then |window2|.
  EXPECT_EQ(bounds_divider.x(), bounds_window1.x() + bounds_window1.width());
  EXPECT_EQ(bounds_window2.x(), bounds_divider.x() + bounds_divider.width());
  EXPECT_EQ(bounds_window1.height(), bounds_divider.height());
  EXPECT_EQ(bounds_window1.height(), bounds_window2.height());

  // Rotate the screen by 270 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_270,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kPortraitPrimary);

  bounds_window1 = window1->GetBoundsInScreen();
  bounds_window2 = window2->GetBoundsInScreen();
  bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test that |window1|, divider, |window2| are now aligned vertically.
  // |window1| is on the top, then the divider, and then |window2|.
  EXPECT_EQ(bounds_divider.y(), bounds_window1.y() + bounds_window1.height());
  EXPECT_EQ(bounds_window2.y(), bounds_divider.y() + bounds_divider.height());
  EXPECT_EQ(bounds_window1.width(), bounds_divider.width());
  EXPECT_EQ(bounds_window1.width(), bounds_window2.width());

  // Rotate the screen by 180 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_180,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kLandscapeSecondary);

  bounds_window1 = window1->GetBoundsInScreen();
  bounds_window2 = window2->GetBoundsInScreen();
  bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test that |window1|, divider, |window2| are now aligned horizontally.
  // |window2| is on the left, then the divider, and then |window1|.
  EXPECT_EQ(bounds_divider.x(), bounds_window2.x() + bounds_window2.width());
  EXPECT_EQ(bounds_window1.x(), bounds_divider.x() + bounds_divider.width());
  EXPECT_EQ(bounds_window1.height(), bounds_divider.height());
  EXPECT_EQ(bounds_window1.height(), bounds_window2.height());

  // Rotate the screen by 90 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_90,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kPortraitSecondary);
  bounds_window1 = window1->GetBoundsInScreen();
  bounds_window2 = window2->GetBoundsInScreen();
  bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test that |window1|, divider, |window2| are now aligned vertically.
  // |window2| is on the top, then the divider, and then |window1|.
  EXPECT_EQ(bounds_divider.y(), bounds_window2.y() + bounds_window2.height());
  EXPECT_EQ(bounds_window1.y(), bounds_divider.y() + bounds_divider.height());
  EXPECT_EQ(bounds_window1.width(), bounds_divider.width());
  EXPECT_EQ(bounds_window1.width(), bounds_window2.width());

  // Rotate the screen back to 0 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_0,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kLandscapePrimary);
  bounds_window1 = window1->GetBoundsInScreen();
  bounds_window2 = window2->GetBoundsInScreen();
  bounds_divider =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);

  // Test |window1|, divider and |window2| are aligned horizontally.
  // |window1| is on the left, then the divider, and then |window2|.
  EXPECT_EQ(bounds_divider.x(), bounds_window1.x() + bounds_window1.width());
  EXPECT_EQ(bounds_window2.x(), bounds_divider.x() + bounds_divider.width());
  EXPECT_EQ(bounds_window1.height(), bounds_divider.height());
  EXPECT_EQ(bounds_window1.height(), bounds_window2.height());
}

// Test that if the split view mode is active when exiting tablet mode, we
// should also end split view mode.
TEST_F(SplitViewControllerTest, ExitTabletModeEndSplitView) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());

  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
}

// Tests that if a window's minimum size is larger than half of the display work
// area's size, it can't be snapped.
TEST_F(SplitViewControllerTest, SnapWindowWithMinimumSizeTest) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));

  const gfx::Rect display_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());
  aura::test::TestWindowDelegate* delegate =
      static_cast<aura::test::TestWindowDelegate*>(window1->delegate());
  delegate->set_minimum_size(
      gfx::Size(display_bounds.width() * 0.5f, display_bounds.height()));
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));

  delegate->set_minimum_size(
      gfx::Size(display_bounds.width() * 0.67f, display_bounds.height()));
  EXPECT_FALSE(split_view_controller()->CanSnap(window1.get()));
}

// Tests that the snapped window can not be moved outside of work area when its
// minimum size is larger than its current desired resizing bounds.
TEST_F(SplitViewControllerTest, ResizingSnappedWindowWithMinimumSizeTest) {
  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::DisplayManager* display_manager = Shell::Get()->display_manager();
  display::test::ScopedSetInternalDisplayId set_internal(display_manager,
                                                         display_id);
  ScreenOrientationControllerTestApi test_api(
      Shell::Get()->screen_orientation_controller());

  const gfx::Rect bounds(0, 0, 300, 200);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  aura::test::TestWindowDelegate* delegate1 =
      static_cast<aura::test::TestWindowDelegate*>(window1->delegate());

  // Set the screen orientation to LANDSCAPE_PRIMARY
  test_api.SetDisplayRotation(display::Display::ROTATE_0,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kLandscapePrimary);

  gfx::Rect display_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  delegate1->set_minimum_size(
      gfx::Size(display_bounds.width() * 0.4f, display_bounds.height()));
  EXPECT_TRUE(window1->layer()->GetTargetTransform().IsIdentity());

  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false);
  split_view_controller()->StartResize(divider_bounds.CenterPoint());
  gfx::Point resize_point(display_bounds.width() * 0.33f, 0);
  split_view_controller()->Resize(resize_point);

  gfx::Rect snapped_window_bounds =
      split_view_controller()->GetSnappedWindowBoundsInScreen(
          window1.get(), SplitViewController::LEFT);
  // The snapped window bounds can't be pushed outside of the display area.
  EXPECT_EQ(snapped_window_bounds.x(), display_bounds.x());
  EXPECT_EQ(snapped_window_bounds.width(),
            window1->delegate()->GetMinimumSize().width());
  EXPECT_FALSE(window1->layer()->GetTargetTransform().IsIdentity());
  split_view_controller()->EndResize(resize_point);
  EndSplitView();

  // Rotate the screen by 270 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_270,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kPortraitPrimary);

  display_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());
  delegate1->set_minimum_size(
      gfx::Size(display_bounds.width(), display_bounds.height() * 0.4f));
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_TRUE(window1->layer()->GetTargetTransform().IsIdentity());
  divider_bounds = split_view_divider()->GetDividerBoundsInScreen(false);
  split_view_controller()->StartResize(divider_bounds.CenterPoint());
  resize_point.SetPoint(0, display_bounds.height() * 0.33f);
  split_view_controller()->Resize(resize_point);

  snapped_window_bounds =
      split_view_controller()->GetSnappedWindowBoundsInScreen(
          window1.get(), SplitViewController::LEFT);
  EXPECT_EQ(snapped_window_bounds.y(), display_bounds.y());
  EXPECT_EQ(snapped_window_bounds.height(),
            window1->delegate()->GetMinimumSize().height());
  EXPECT_FALSE(window1->layer()->GetTargetTransform().IsIdentity());
  split_view_controller()->EndResize(resize_point);
  EndSplitView();

  // Rotate the screen by 180 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_180,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kLandscapeSecondary);

  display_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());
  delegate1->set_minimum_size(
      gfx::Size(display_bounds.width() * 0.4f, display_bounds.height()));
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  EXPECT_TRUE(window1->layer()->GetTargetTransform().IsIdentity());

  divider_bounds = split_view_divider()->GetDividerBoundsInScreen(false);
  split_view_controller()->StartResize(divider_bounds.CenterPoint());
  resize_point.SetPoint(display_bounds.width() * 0.33f, 0);
  split_view_controller()->Resize(resize_point);

  snapped_window_bounds =
      split_view_controller()->GetSnappedWindowBoundsInScreen(
          window1.get(), SplitViewController::RIGHT);
  EXPECT_EQ(snapped_window_bounds.x(), display_bounds.x());
  EXPECT_EQ(snapped_window_bounds.width(),
            window1->delegate()->GetMinimumSize().width());
  EXPECT_FALSE(window1->layer()->GetTargetTransform().IsIdentity());
  split_view_controller()->EndResize(resize_point);
  EndSplitView();

  // Rotate the screen by 90 degree.
  test_api.SetDisplayRotation(display::Display::ROTATE_90,
                              display::Display::RotationSource::ACTIVE);
  EXPECT_EQ(test_api.GetCurrentOrientation(),
            OrientationLockType::kPortraitSecondary);

  display_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());
  delegate1->set_minimum_size(
      gfx::Size(display_bounds.width(), display_bounds.height() * 0.4f));
  EXPECT_TRUE(split_view_controller()->CanSnap(window1.get()));
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  EXPECT_TRUE(window1->layer()->GetTargetTransform().IsIdentity());

  divider_bounds = split_view_divider()->GetDividerBoundsInScreen(false);
  split_view_controller()->StartResize(divider_bounds.CenterPoint());
  resize_point.SetPoint(0, display_bounds.height() * 0.33f);
  split_view_controller()->Resize(resize_point);

  snapped_window_bounds =
      split_view_controller()->GetSnappedWindowBoundsInScreen(
          window1.get(), SplitViewController::RIGHT);
  EXPECT_EQ(snapped_window_bounds.y(), display_bounds.y());
  EXPECT_EQ(snapped_window_bounds.height(),
            window1->delegate()->GetMinimumSize().height());
  EXPECT_FALSE(window1->layer()->GetTargetTransform().IsIdentity());
  split_view_controller()->EndResize(resize_point);
  EndSplitView();
}

// Tests that the divider should not be moved to a position that is smaller than
// the snapped window's minimum size after resizing.
TEST_F(SplitViewControllerTest,
       DividerPositionOnResizingSnappedWindowWithMinimumSizeTest) {
  const gfx::Rect bounds(0, 0, 200, 200);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  aura::test::TestWindowDelegate* delegate1 =
      static_cast<aura::test::TestWindowDelegate*>(window1->delegate());
  ui::test::EventGenerator& generator(GetEventGenerator());
  EXPECT_EQ(OrientationLockType::kLandscapePrimary, screen_orientation());
  gfx::Rect workarea_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());

  // Snap the divider to one third position when there is only left window with
  // minimum size larger than one third of the display's width. The divider
  // should be snapped to the middle position after dragging.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  delegate1->set_minimum_size(
      gfx::Size(workarea_bounds.width() * 0.4f, workarea_bounds.height()));
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false);
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.33f, 0));
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());

  // Snap the divider to two third position, it should be kept at there after
  // dragging.
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.67f, 0));
  EXPECT_GT(divider_position(), 0.5f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.67f * workarea_bounds.width());
  EndSplitView();

  // Snap the divider to two third position when there is only right window with
  // minium size larger than one third of the display's width. The divider
  // should be snapped to the middle position after dragging.
  delegate1->set_minimum_size(
      gfx::Size(workarea_bounds.width() * 0.4f, workarea_bounds.height()));
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  divider_bounds = split_view_divider()->GetDividerBoundsInScreen(false);
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.67f, 0));
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());

  // Snap the divider to one third position, it should be kept at there after
  // dragging.
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.33f, 0));
  EXPECT_GT(divider_position(), 0);
  EXPECT_LE(divider_position(), 0.33f * workarea_bounds.width());
  EndSplitView();

  // Snap the divider to one third position when there are both left and right
  // snapped windows with the same minimum size larger than one third of the
  // display's width. The divider should be snapped to the middle position after
  // dragging.
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  aura::test::TestWindowDelegate* delegate2 =
      static_cast<aura::test::TestWindowDelegate*>(window2->delegate());
  delegate2->set_minimum_size(
      gfx::Size(workarea_bounds.width() * 0.4f, workarea_bounds.height()));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  divider_bounds = split_view_divider()->GetDividerBoundsInScreen(false);
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.33f, 0));
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());

  // Snap the divider to two third position, it should be snapped to the middle
  // position after dragging.
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.67f, 0));
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());
  EndSplitView();
}

// Tests that the divider and snapped windows bounds should be updated if
// snapping a new window with minimum size, which is larger than the bounds
// of its snap position.
TEST_F(SplitViewControllerTest,
       DividerPositionWithWindowMinimumSizeOnSnapTest) {
  const gfx::Rect bounds(0, 0, 200, 300);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  const gfx::Rect workarea_bounds =
      split_view_controller()->GetDisplayWorkAreaBoundsInScreen(window1.get());

  // Divider should be moved to the middle at the beginning.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  ASSERT_TRUE(split_view_divider());
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());

  // Drag the divider to two-third position.
  ui::test::EventGenerator& generator(GetEventGenerator());
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false);
  generator.set_current_location(divider_bounds.CenterPoint());
  generator.DragMouseTo(gfx::Point(workarea_bounds.width() * 0.67f, 0));
  EXPECT_GT(divider_position(), 0.5f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.67f * workarea_bounds.width());

  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  aura::test::TestWindowDelegate* delegate2 =
      static_cast<aura::test::TestWindowDelegate*>(window2->delegate());
  delegate2->set_minimum_size(
      gfx::Size(workarea_bounds.width() * 0.4f, workarea_bounds.height()));
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_GT(divider_position(), 0.33f * workarea_bounds.width());
  EXPECT_LE(divider_position(), 0.5f * workarea_bounds.width());
}

// Test that if display configuration changes in lock screen, the split view
// mode doesn't end.
TEST_F(SplitViewControllerTest, DoNotEndSplitViewInLockScreen) {
  display::test::DisplayManagerTestApi(display_manager())
      .SetFirstDisplayAsInternalDisplay();
  UpdateDisplay("800x400");
  const gfx::Rect bounds(0, 0, 200, 300);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);

  // Now lock the screen.
  Shell::Get()->session_controller()->LockScreenAndFlushForTest();
  // Change display configuration. Split view mode is still active.
  UpdateDisplay("400x800");
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);

  // Now unlock the screen.
  GetSessionControllerClient()->UnlockScreen();
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
}

// Test that when split view and overview are both active when a new window is
// added to the window hierarchy, overview is not ended.
TEST_F(SplitViewControllerTest, NewWindowTest) {
  const gfx::Rect bounds(0, 0, 200, 300);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  ToggleOverview();
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Now new a window. Test it won't end the overview mode
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
}

// Tests that when split view ends because of a transition from tablet mode to
// laptop mode during a resize operation, drags are properly completed.
TEST_F(SplitViewControllerTest, ExitTabletModeDuringResizeCompletesDrags) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  auto* w1_state = wm::GetWindowState(window1.get());
  auto* w2_state = wm::GetWindowState(window2.get());

  // Setup delegates
  auto* window_state_delegate1 = new TestWindowStateDelegate();
  auto* window_state_delegate2 = new TestWindowStateDelegate();
  w1_state->SetDelegate(base::WrapUnique(window_state_delegate1));
  w2_state->SetDelegate(base::WrapUnique(window_state_delegate2));

  // Set up windows.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);

  // Start a drag but don't release the mouse button.
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  const int screen_width =
      screen_util::GetDisplayWorkAreaBoundsInParent(window1.get()).width();
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().PressLeftButton();
  GetEventGenerator().MoveMouseTo(screen_width * 0.67f, 0);

  // Drag is started for both windows.
  EXPECT_TRUE(window_state_delegate1->drag_in_progress());
  EXPECT_TRUE(window_state_delegate2->drag_in_progress());
  EXPECT_NE(nullptr, w1_state->drag_details());
  EXPECT_NE(nullptr, w2_state->drag_details());

  // End tablet mode.
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(false);

  // Drag is ended for both windows.
  EXPECT_EQ(nullptr, w1_state->drag_details());
  EXPECT_EQ(nullptr, w2_state->drag_details());
  EXPECT_FALSE(window_state_delegate1->drag_in_progress());
  EXPECT_FALSE(window_state_delegate2->drag_in_progress());
}

// Tests that when a single window is present in split view mode is minimized
// during a resize operation, then drags are properly completed.
TEST_F(SplitViewControllerTest,
       MinimizeSingleWindowDuringResizeCompletesDrags) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  auto* w1_state = wm::GetWindowState(window1.get());

  // Setup delegate
  auto* window_state_delegate1 = new TestWindowStateDelegate();
  w1_state->SetDelegate(base::WrapUnique(window_state_delegate1));

  // Set up window.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);

  // Start a drag but don't release the mouse button.
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  const int screen_width =
      screen_util::GetDisplayWorkAreaBoundsInParent(window1.get()).width();
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().PressLeftButton();
  GetEventGenerator().MoveMouseTo(screen_width * 0.67f, 0);

  // Drag is started.
  EXPECT_TRUE(window_state_delegate1->drag_in_progress());
  EXPECT_NE(nullptr, w1_state->drag_details());

  // Minimize the window.
  wm::WMEvent minimize_event(wm::WM_EVENT_MINIMIZE);
  wm::GetWindowState(window1.get())->OnWMEvent(&minimize_event);

  // Drag is ended.
  EXPECT_FALSE(window_state_delegate1->drag_in_progress());
  EXPECT_EQ(nullptr, w1_state->drag_details());
}

// Tests that when two windows are present in split view mode and one of them
// is minimized during a resize, then drags are properly completed.
TEST_F(SplitViewControllerTest,
       MinimizeOneOfTwoWindowsDuringResizeCompletesDrags) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  auto* w1_state = wm::GetWindowState(window1.get());
  auto* w2_state = wm::GetWindowState(window2.get());

  // Setup delegates
  auto* window_state_delegate1 = new TestWindowStateDelegate();
  auto* window_state_delegate2 = new TestWindowStateDelegate();
  w1_state->SetDelegate(base::WrapUnique(window_state_delegate1));
  w2_state->SetDelegate(base::WrapUnique(window_state_delegate2));

  // Set up windows.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);

  // Start a drag but don't release the mouse button.
  gfx::Rect divider_bounds =
      split_view_divider()->GetDividerBoundsInScreen(false /* is_dragging */);
  const int screen_width =
      screen_util::GetDisplayWorkAreaBoundsInParent(window1.get()).width();
  GetEventGenerator().set_current_location(divider_bounds.CenterPoint());
  GetEventGenerator().PressLeftButton();
  GetEventGenerator().MoveMouseTo(screen_width * 0.67f, 0);

  // Drag is started for both windows.
  EXPECT_TRUE(window_state_delegate1->drag_in_progress());
  EXPECT_TRUE(window_state_delegate2->drag_in_progress());
  EXPECT_NE(nullptr, w1_state->drag_details());
  EXPECT_NE(nullptr, w2_state->drag_details());

  // Minimize the left window.
  wm::WMEvent minimize_event(wm::WM_EVENT_MINIMIZE);
  wm::GetWindowState(window1.get())->OnWMEvent(&minimize_event);

  // Drag is ended just for the left window.
  EXPECT_FALSE(window_state_delegate1->drag_in_progress());
  EXPECT_TRUE(window_state_delegate2->drag_in_progress());
  EXPECT_EQ(nullptr, w1_state->drag_details());
  EXPECT_NE(nullptr, w2_state->drag_details());
}

// Test that when a snapped window's resizablity property change from resizable
// to unresizable, the split view mode is ended.
TEST_F(SplitViewControllerTest, ResizabilityChangeTest) {
  const gfx::Rect bounds(0, 0, 200, 300);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());

  window1->SetProperty(aura::client::kResizeBehaviorKey,
                       ui::mojom::kResizeBehaviorNone);
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
}

// Tests that shadows on windows disappear when the window is snapped, and
// reappear when unsnapped.
TEST_F(SplitViewControllerTest, ShadowDisappearsWhenSnapped) {
  const gfx::Rect bounds(200, 200);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));

  ::wm::ShadowController* shadow_controller = Shell::Get()->shadow_controller();
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window1.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window2.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window3.get()));

  // Snap |window1| to the left. Its shadow should disappear.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  EXPECT_FALSE(shadow_controller->IsShadowVisibleForWindow(window1.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window2.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window3.get()));

  // Snap |window2| to the right. Its shadow should also disappear.
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_FALSE(shadow_controller->IsShadowVisibleForWindow(window1.get()));
  EXPECT_FALSE(shadow_controller->IsShadowVisibleForWindow(window2.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window3.get()));

  // Snap |window3| to the right. Its shadow should disappear and |window2|'s
  // shadow should reappear.
  split_view_controller()->SnapWindow(window3.get(),
                                      SplitViewController::RIGHT);
  EXPECT_FALSE(shadow_controller->IsShadowVisibleForWindow(window1.get()));
  EXPECT_TRUE(shadow_controller->IsShadowVisibleForWindow(window2.get()));
  EXPECT_FALSE(shadow_controller->IsShadowVisibleForWindow(window3.get()));
}

// Test the tab-dragging related functionalities in tablet mode. Tab(s) can be
// dragged out of a window and then put in split view mode or merge into another
// window.
class SplitViewTabDraggingTest : public SplitViewControllerTest {
 public:
  SplitViewTabDraggingTest() = default;
  ~SplitViewTabDraggingTest() override = default;

 protected:
  // Starts tab dragging on |dragged_window|. |source_window| indicates which
  // window the drag originates from. Returns the newly created WindowResizer
  // for the |dragged_window|.
  std::unique_ptr<WindowResizer> StartDrag(aura::Window* dragged_window,
                                           aura::Window* source_window) {
    SetIsInTabDragging(dragged_window, /*is_dragging=*/true, source_window);
    return CreateResizerForTest(dragged_window,
                                dragged_window->bounds().origin(), HTCAPTION);
  }

  // Drags the window to |end_position| and ends the drag. Also deletes the
  // WindowResizer that's installed on the dragged window.
  void DragWindowTo(std::unique_ptr<WindowResizer> resizer,
                    const gfx::Point& end_position) {
    resizer->Drag(end_position, 0);
    resizer->CompleteDrag();
    SetIsInTabDragging(resizer->GetTarget(), /*is_dragging=*/false);
  }

  void DragWindowWithOffset(std::unique_ptr<WindowResizer> resizer,
                            int delta_x,
                            int delta_y) {
    gfx::Point location = resizer->GetInitialLocation();
    location.set_x(location.x() + delta_x);
    location.set_y(location.y() + delta_y);
    resizer->Drag(location, 0);
    resizer->CompleteDrag();
    SetIsInTabDragging(resizer->GetTarget(), /*is_dragging=*/false);
  }

  std::unique_ptr<WindowResizer> CreateResizerForTest(
      aura::Window* window,
      const gfx::Point& point_in_parent,
      int window_component,
      ::wm::WindowMoveSource source = ::wm::WINDOW_MOVE_SOURCE_TOUCH) {
    return CreateWindowResizer(window, point_in_parent, window_component,
                               source);
  }

  // Sets if |dragged_window| is currently in tab-dragging process.
  // |source_window| is the window that the drag originates from. This method is
  // used to simulate the start/stop of a window's tab-dragging by setting the
  // two window properties, which are usually set in TabDragController::
  // UpdateTabDraggingInfo() function.
  void SetIsInTabDragging(aura::Window* dragged_window,
                          bool is_dragging,
                          aura::Window* source_window = nullptr) {
    if (!is_dragging) {
      dragged_window->ClearProperty(kIsDraggingTabsKey);
      dragged_window->ClearProperty(kTabDraggingSourceWindowKey);
    } else {
      dragged_window->SetProperty(kIsDraggingTabsKey, is_dragging);
      dragged_window->SetProperty(kTabDraggingSourceWindowKey, source_window);
    }
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SplitViewTabDraggingTest);
};

// Test that in tablet mode, we only allow dragging that happens on window
// caption area and tab-dragging is in process.
TEST_F(SplitViewTabDraggingTest, OnlyAllowDraggingOnTabs) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window(CreateWindow(bounds));
  std::unique_ptr<WindowResizer> resizer =
      CreateResizerForTest(window.get(), gfx::Point(), HTCAPTION);
  EXPECT_FALSE(resizer.get());

  SetIsInTabDragging(window.get(), /*is_dragging=*/true);
  // Only dragging on HTCAPTION area is allowed.
  resizer = CreateResizerForTest(window.get(), gfx::Point(), HTLEFT);
  EXPECT_FALSE(resizer.get());
  resizer = CreateResizerForTest(window.get(), gfx::Point(), HTRIGHT);
  EXPECT_FALSE(resizer.get());
  resizer = CreateResizerForTest(window.get(), gfx::Point(), HTCAPTION);
  EXPECT_TRUE(resizer.get());

  SetIsInTabDragging(window.get(), /*is_dragging=*/false);
  resizer = CreateResizerForTest(window.get(), gfx::Point(), HTCAPTION);
  EXPECT_FALSE(resizer.get());
}

// Test that in tablet mode, if the dragging is from mouse event, the mouse
// cursor should be properly locked.
TEST_F(SplitViewTabDraggingTest, LockCursor) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window(CreateWindow(bounds));
  SetIsInTabDragging(window.get(), /*is_dragging=*/true);
  EXPECT_FALSE(Shell::Get()->cursor_manager()->IsCursorLocked());

  std::unique_ptr<WindowResizer> resizer = CreateResizerForTest(
      window.get(), gfx::Point(), HTCAPTION, ::wm::WINDOW_MOVE_SOURCE_MOUSE);
  ASSERT_TRUE(resizer.get());
  EXPECT_TRUE(Shell::Get()->cursor_manager()->IsCursorLocked());

  resizer.reset();
  EXPECT_FALSE(Shell::Get()->cursor_manager()->IsCursorLocked());
}

// Test that in tablet mode, if a window is in tab-dragging process, its
// backdrop is disabled during dragging process.
TEST_F(SplitViewTabDraggingTest, NoBackDropDuringDragging) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window(CreateWindow(bounds));
  EXPECT_EQ(window->GetProperty(kBackdropWindowMode),
            BackdropWindowMode::kAuto);

  std::unique_ptr<WindowResizer> resizer =
      StartDrag(window.get(), window.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_EQ(window->GetProperty(kBackdropWindowMode),
            BackdropWindowMode::kDisabled);

  resizer->Drag(gfx::Point(), 0);
  EXPECT_EQ(window->GetProperty(kBackdropWindowMode),
            BackdropWindowMode::kDisabled);

  resizer->CompleteDrag();
  EXPECT_EQ(window->GetProperty(kBackdropWindowMode),
            BackdropWindowMode::kAuto);
}

// Test that in tablet mode, the window that is in tab-dragging process should
// not be shown in overview mode.
TEST_F(SplitViewTabDraggingTest, DoNotShowDraggedWindowInOverview) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  WindowSelector* window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_TRUE(window_selector->IsWindowInOverview(window1.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window2.get()));
  Shell::Get()->window_selector_controller()->ToggleOverview();

  StartDrag(window1.get(), window1.get());

  Shell::Get()->window_selector_controller()->ToggleOverview();
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_FALSE(window_selector->IsWindowInOverview(window1.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window2.get()));
}

// Test that if a window is in tab-dragging process, the split divider is placed
// below the current dragged window.
TEST_F(SplitViewTabDraggingTest, DividerIsBelowDraggedWindow) {
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  views::Widget* split_divider_widget =
      split_view_controller()->split_view_divider()->divider_widget();
  EXPECT_TRUE(split_divider_widget->IsAlwaysOnTop());

  std::unique_ptr<WindowResizer> resizer =
      StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_FALSE(split_divider_widget->IsAlwaysOnTop());

  resizer->Drag(gfx::Point(), 0);
  EXPECT_FALSE(split_divider_widget->IsAlwaysOnTop());

  resizer->CompleteDrag();
  EXPECT_TRUE(split_divider_widget->IsAlwaysOnTop());
}

// Test the functionalities that are related to dragging a maximized window's
// tabs. See the expected behaviors described in go/tab-dragging-in-tablet-mode.
TEST_F(SplitViewTabDraggingTest, DragMaximizedWindow) {
  UpdateDisplay("600x600");
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  wm::GetWindowState(window1.get())->Maximize();
  wm::GetWindowState(window2.get())->Maximize();
  EXPECT_TRUE(wm::GetWindowState(window1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(window2.get())->IsMaximized());

  // 1. If the dragged window is the source window:
  std::unique_ptr<WindowResizer> resizer =
      StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  // 1.a. Drag the window to move a small amount of distance will maximize the
  // window again.
  DragWindowWithOffset(std::move(resizer), 10, 10);
  EXPECT_TRUE(wm::GetWindowState(window1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(window2.get())->IsMaximized());
  // 1.b. Drag the window long enough (pass one fourth of the screen vertical
  // height) to snap the window to splitscreen.
  resizer = StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  DragWindowTo(std::move(resizer), gfx::Point(0, 300));
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Maximize the snapped window to end split view mode and overview mode.
  wm::GetWindowState(window1.get())->Maximize();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());

  // 2. If the dragged window is not the source window:
  resizer = StartDrag(window1.get(), window2.get());
  ASSERT_TRUE(resizer.get());
  // 2.a. Drag the window a small amount of distance will maximize the window.
  DragWindowWithOffset(std::move(resizer), 10, 10);
  EXPECT_TRUE(wm::GetWindowState(window1.get())->IsMaximized());
  EXPECT_TRUE(wm::GetWindowState(window2.get())->IsMaximized());
  // 2.b. Drag the window long enough to snap the window. The source window will
  // snap to the other side of the splitscreen.
  resizer = StartDrag(window1.get(), window2.get());
  ASSERT_TRUE(resizer.get());
  DragWindowTo(std::move(resizer), gfx::Point(0, 300));
  EXPECT_TRUE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());

  EndSplitView();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());

  // 3. If the dragged window is destroyed during dragging (may happen due to
  // all its tabs are attached into another window), nothing changes.
  resizer = StartDrag(window1.get(), window2.get());
  ASSERT_TRUE(resizer.get());
  resizer->Drag(gfx::Point(0, 300), 0);
  window1.reset();
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_TRUE(wm::GetWindowState(window2.get())->IsMaximized());
}

// Test the functionalities that are related to dragging a snapped window in
// splitscreen. There are always two snapped window when the drag starts (i.e.,
// the overview mode is not active). See the expected behaviors described in
// go/tab-dragging-in-tablet-mode.
TEST_F(SplitViewTabDraggingTest, DragSnappedWindow) {
  UpdateDisplay("600x600");
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));

  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());

  // 1. If the dragged window is the source window:
  std::unique_ptr<WindowResizer> resizer =
      StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::RIGHT_SNAPPED);
  // In this case overview grid will be opened, containing |window3|.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  WindowSelector* window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_FALSE(window_selector->IsWindowInOverview(window1.get()));
  EXPECT_FALSE(window_selector->IsWindowInOverview(window2.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window3.get()));
  // 1.a. If the window is only dragged for a small distance, the window will
  // be put back to its original position. Overview mode will be ended.
  DragWindowWithOffset(std::move(resizer), 10, 10);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_EQ(split_view_controller()->right_window(), window2.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  // 1.b. If the window is dragged long enough, it can replace the other split
  // window.
  resizer = StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  DragWindowTo(std::move(resizer), gfx::Point(500, 300));
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::RIGHT_SNAPPED);
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_FALSE(window_selector->IsWindowInOverview(window1.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window2.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window3.get()));
  // Snap |window2| again to test 1.c.
  split_view_controller()->SnapWindow(window2.get(), SplitViewController::LEFT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  // 1.c. If the dragged window is destroyed during dragging (may happen due to
  // all its tabs are attached into another window), nothing changes.
  resizer = StartDrag(window1.get(), window1.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  ASSERT_TRUE(resizer.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  resizer->Drag(gfx::Point(100, 100), 0);
  window1.reset();
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());

  // Recreate |window1| and snap it to test the following senarioes.
  window1.reset(CreateWindow(bounds));
  split_view_controller()->SnapWindow(window1.get(),
                                      SplitViewController::RIGHT);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());

  // 2. If the dragged window is not the source window:
  // In this case, |window3| can be regarded as a window that originates from
  // |window2|.
  resizer = StartDrag(window3.get(), window2.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  // 2.a. If the window is only dragged for a small amount of distance, it will
  // replace the same side of the split window that it originates from.
  DragWindowWithOffset(std::move(resizer), 10, 10);
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window3.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  // 2.b. If the window is dragged long enough, it can replace the other side of
  // the split window.
  resizer = StartDrag(window2.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window3.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  DragWindowTo(std::move(resizer), gfx::Point(0, 300));
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
}

// Test the functionalities that are related to dragging a snapped window while
// overview grid is open on the other side of the screen. See the expected
// behaviors described in go/tab-dragging-in-tablet-mode.
TEST_F(SplitViewTabDraggingTest, DragSnappedWindowWhileOverviewOpen) {
  UpdateDisplay("600x600");
  const gfx::Rect bounds(0, 0, 400, 400);
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));

  // Prepare the testing senario:
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  ToggleOverview();
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());

  // 1. If the dragged window is the source window:
  std::unique_ptr<WindowResizer> resizer =
      StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  // Overivew mode is still active, but split view mode is ended due to dragging
  // the only snapped window.
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  // 1.a. If the window is only dragged for a small amount of distance
  DragWindowWithOffset(std::move(resizer), 10, 10);
  EXPECT_TRUE(wm::GetWindowState(window1.get())->IsMaximized());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  // 1.b. If the window is dragged long enough, it can be snappped again.
  // Prepare the testing senario first.
  split_view_controller()->SnapWindow(window1.get(), SplitViewController::LEFT);
  split_view_controller()->SnapWindow(window2.get(),
                                      SplitViewController::RIGHT);
  ToggleOverview();
  resizer = StartDrag(window1.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  EXPECT_FALSE(split_view_controller()->IsSplitViewModeActive());
  DragWindowTo(std::move(resizer), gfx::Point(0, 300));
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window1.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  WindowSelector* window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_TRUE(window_selector->IsWindowInOverview(window2.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window3.get()));

  // 2. If the dragged window is not the source window:
  // Prepare the testing senario first.
  resizer = StartDrag(window2.get(), window1.get());
  ASSERT_TRUE(resizer.get());
  // 2.a. The dragged window can replace the only snapped window in the split
  // screen. After that, the old snapped window will be put back in overview.
  DragWindowTo(std::move(resizer), gfx::Point(0, 300));
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::LEFT_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_TRUE(Shell::Get()->window_selector_controller()->IsSelecting());
  window_selector =
      Shell::Get()->window_selector_controller()->window_selector();
  EXPECT_TRUE(window_selector->IsWindowInOverview(window1.get()));
  EXPECT_TRUE(window_selector->IsWindowInOverview(window3.get()));
  // 2.b. The dragged window can snap to the other side of the splitscreen,
  // causing overview mode to end.
  resizer = StartDrag(window1.get(), window2.get());
  ASSERT_TRUE(resizer.get());
  DragWindowTo(std::move(resizer), gfx::Point(500, 300));
  EXPECT_EQ(split_view_controller()->state(),
            SplitViewController::BOTH_SNAPPED);
  EXPECT_EQ(split_view_controller()->left_window(), window2.get());
  EXPECT_EQ(split_view_controller()->right_window(), window1.get());
  EXPECT_FALSE(Shell::Get()->window_selector_controller()->IsSelecting());
}

}  // namespace ash
