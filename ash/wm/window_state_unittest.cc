// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_state.h"

#include <utility>

#include "ash/public/cpp/window_properties.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_util.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/display/screen.h"
#include "ui/events/test/event_generator.h"

using ash::mojom::WindowStateType;

namespace ash {
namespace wm {
namespace {

class AlwaysMaximizeTestState : public WindowState::State {
 public:
  explicit AlwaysMaximizeTestState(WindowStateType initial_state_type)
      : state_type_(initial_state_type) {}
  ~AlwaysMaximizeTestState() override = default;

  // WindowState::State overrides:
  void OnWMEvent(WindowState* window_state, const WMEvent* event) override {
    // We don't do anything here.
  }
  WindowStateType GetType() const override { return state_type_; }
  void AttachState(WindowState* window_state,
                   WindowState::State* previous_state) override {
    // We always maximize.
    if (state_type_ != mojom::WindowStateType::MAXIMIZED) {
      window_state->Maximize();
      state_type_ = mojom::WindowStateType::MAXIMIZED;
    }
  }
  void DetachState(WindowState* window_state) override {}

 private:
  WindowStateType state_type_;

  DISALLOW_COPY_AND_ASSIGN(AlwaysMaximizeTestState);
};

}  // namespace

using WindowStateTest = AshTestBase;

// Test that a window gets properly snapped to the display's edges in a
// multi monitor environment.
TEST_F(WindowStateTest, SnapWindowBasic) {
  UpdateDisplay("0+0-500x400, 0+500-600x400");
  const gfx::Rect kPrimaryDisplayWorkAreaBounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  const gfx::Rect kSecondaryDisplayWorkAreaBounds =
      GetSecondaryDisplay().work_area();

  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithBounds(gfx::Rect(100, 100, 100, 100)));
  WindowState* window_state = GetWindowState(window.get());
  const WMEvent snap_left(WM_EVENT_SNAP_LEFT);
  window_state->OnWMEvent(&snap_left);
  gfx::Rect expected = gfx::Rect(kPrimaryDisplayWorkAreaBounds.x(),
                                 kPrimaryDisplayWorkAreaBounds.y(),
                                 kPrimaryDisplayWorkAreaBounds.width() / 2,
                                 kPrimaryDisplayWorkAreaBounds.height());
  EXPECT_EQ(expected.ToString(), window->GetBoundsInScreen().ToString());

  const WMEvent snap_right(WM_EVENT_SNAP_RIGHT);
  window_state->OnWMEvent(&snap_right);
  expected.set_x(kPrimaryDisplayWorkAreaBounds.right() - expected.width());
  EXPECT_EQ(expected.ToString(), window->GetBoundsInScreen().ToString());

  // Move the window to the secondary display.
  window->SetBoundsInScreen(gfx::Rect(600, 0, 100, 100), GetSecondaryDisplay());

  window_state->OnWMEvent(&snap_right);
  expected = gfx::Rect(kSecondaryDisplayWorkAreaBounds.x() +
                           kSecondaryDisplayWorkAreaBounds.width() / 2,
                       kSecondaryDisplayWorkAreaBounds.y(),
                       kSecondaryDisplayWorkAreaBounds.width() / 2,
                       kSecondaryDisplayWorkAreaBounds.height());
  EXPECT_EQ(expected.ToString(), window->GetBoundsInScreen().ToString());

  window_state->OnWMEvent(&snap_left);
  expected.set_x(kSecondaryDisplayWorkAreaBounds.x());
  EXPECT_EQ(expected.ToString(), window->GetBoundsInScreen().ToString());
}

// Test how the minimum and maximum size specified by the aura::WindowDelegate
// affect snapping.
TEST_F(WindowStateTest, SnapWindowMinimumSize) {
  UpdateDisplay("0+0-600x900");
  const gfx::Rect kWorkAreaBounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

  aura::test::TestWindowDelegate delegate;
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithDelegate(
      &delegate, -1, gfx::Rect(0, 100, kWorkAreaBounds.width() - 1, 100)));

  // It should be possible to snap a window with a minimum size.
  delegate.set_minimum_size(gfx::Size(kWorkAreaBounds.width() - 1, 0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_TRUE(window_state->CanSnap());
  const WMEvent snap_right(WM_EVENT_SNAP_RIGHT);
  window_state->OnWMEvent(&snap_right);
  gfx::Rect expected =
      gfx::Rect(kWorkAreaBounds.x() + 1, kWorkAreaBounds.y(),
                kWorkAreaBounds.width() - 1, kWorkAreaBounds.height());
  EXPECT_EQ(expected.ToString(), window->GetBoundsInScreen().ToString());

  // It should not be possible to snap a window with a maximum size, or if it
  // cannot be maximized.
  delegate.set_maximum_size(gfx::Size(kWorkAreaBounds.width() - 1, 0));
  EXPECT_FALSE(window_state->CanSnap());
  delegate.set_maximum_size(gfx::Size(0, kWorkAreaBounds.height() - 1));
  EXPECT_FALSE(window_state->CanSnap());
  delegate.set_maximum_size(gfx::Size());
  window->SetProperty(aura::client::kResizeBehaviorKey,
                      ui::mojom::kResizeBehaviorCanResize);
  EXPECT_FALSE(window_state->CanSnap());
}

// Test that the minimum size specified by aura::WindowDelegate gets respected.
TEST_F(WindowStateTest, TestRespectMinimumSize) {
  UpdateDisplay("0+0-1024x768");

  aura::test::TestWindowDelegate delegate;
  const gfx::Size minimum_size(gfx::Size(500, 300));
  delegate.set_minimum_size(minimum_size);

  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithDelegate(
      &delegate, -1, gfx::Rect(0, 100, 100, 100)));

  // Check that the window has the correct minimum size.
  EXPECT_EQ(minimum_size.ToString(), window->bounds().size().ToString());

  // Set the size to something bigger - that should work.
  gfx::Rect bigger_bounds(700, 500, 700, 500);
  window->SetBounds(bigger_bounds);
  EXPECT_EQ(bigger_bounds.ToString(), window->bounds().ToString());

  // Set the size to something smaller - that should only resize to the smallest
  // possible size.
  gfx::Rect smaller_bounds(700, 500, 100, 100);
  window->SetBounds(smaller_bounds);
  EXPECT_EQ(minimum_size.ToString(), window->bounds().size().ToString());
}

// Test that the minimum window size specified by aura::WindowDelegate does not
// exceed the screen size.
TEST_F(WindowStateTest, TestIgnoreTooBigMinimumSize) {
  UpdateDisplay("0+0-1024x768");
  const gfx::Size work_area_size =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area().size();
  const gfx::Size illegal_size(1280, 960);
  const gfx::Rect illegal_bounds(gfx::Point(0, 0), illegal_size);

  aura::test::TestWindowDelegate delegate;
  const gfx::Size minimum_size(illegal_size);
  delegate.set_minimum_size(minimum_size);

  // The creation should force the window to respect the screen size.
  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithDelegate(&delegate, -1, illegal_bounds));
  EXPECT_EQ(work_area_size.ToString(), window->bounds().size().ToString());

  // Trying to set the size to something bigger then the screen size should be
  // ignored.
  window->SetBounds(illegal_bounds);
  EXPECT_EQ(work_area_size.ToString(), window->bounds().size().ToString());

  // Maximizing the window should not allow it to go bigger than that either.
  WindowState* window_state = GetWindowState(window.get());
  window_state->Maximize();
  EXPECT_EQ(work_area_size.ToString(), window->bounds().size().ToString());
}

// Tests UpdateSnappedWidthRatio. (1) It should have ratio reset when window
// enters snapped state; (2) it should update ratio on bounds event when
// snapped.
TEST_F(WindowStateTest, UpdateSnapWidthRatioTest) {
  UpdateDisplay("0+0-900x600");
  const gfx::Rect kWorkAreaBounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  aura::test::TestWindowDelegate delegate;
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithDelegate(
      &delegate, -1, gfx::Rect(100, 100, 100, 100)));
  delegate.set_window_component(HTRIGHT);
  WindowState* window_state = GetWindowState(window.get());
  const WMEvent cycle_snap_left(WM_EVENT_CYCLE_SNAP_LEFT);
  window_state->OnWMEvent(&cycle_snap_left);
  EXPECT_EQ(mojom::WindowStateType::LEFT_SNAPPED, window_state->GetStateType());
  gfx::Rect expected =
      gfx::Rect(kWorkAreaBounds.x(), kWorkAreaBounds.y(),
                kWorkAreaBounds.width() / 2, kWorkAreaBounds.height());
  EXPECT_EQ(expected, window->GetBoundsInScreen());
  EXPECT_EQ(0.5f, *window_state->snapped_width_ratio());

  // Drag to change snapped window width.
  const int kIncreasedWidth = 225;
  ui::test::EventGenerator& generator = GetEventGenerator();
  generator.MoveMouseTo(window->bounds().right(), window->bounds().y());
  generator.PressLeftButton();
  generator.MoveMouseTo(window->bounds().right() + kIncreasedWidth,
                        window->bounds().y());
  generator.ReleaseLeftButton();
  expected.set_width(expected.width() + kIncreasedWidth);
  EXPECT_EQ(expected, window->GetBoundsInScreen());
  EXPECT_EQ(mojom::WindowStateType::LEFT_SNAPPED, window_state->GetStateType());
  EXPECT_EQ(0.75f, *window_state->snapped_width_ratio());

  // Another cycle snap left event will restore window state to normal.
  window_state->OnWMEvent(&cycle_snap_left);
  EXPECT_EQ(mojom::WindowStateType::NORMAL, window_state->GetStateType());
  EXPECT_FALSE(window_state->snapped_width_ratio());

  // Another cycle snap left event will snap window and reset snapped width
  // ratio.
  window_state->OnWMEvent(&cycle_snap_left);
  EXPECT_EQ(mojom::WindowStateType::LEFT_SNAPPED, window_state->GetStateType());
  EXPECT_EQ(0.5f, *window_state->snapped_width_ratio());
}

// Test that snapping left/right preserves the restore bounds.
TEST_F(WindowStateTest, RestoreBounds) {
  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithBounds(gfx::Rect(100, 100, 100, 100)));
  WindowState* window_state = GetWindowState(window.get());

  EXPECT_TRUE(window_state->IsNormalStateType());

  // 1) Start with restored window with restore bounds set.
  gfx::Rect restore_bounds = window->GetBoundsInScreen();
  restore_bounds.set_width(restore_bounds.width() + 1);
  window_state->SetRestoreBoundsInScreen(restore_bounds);
  const WMEvent snap_left(WM_EVENT_SNAP_LEFT);
  window_state->OnWMEvent(&snap_left);
  const WMEvent snap_right(WM_EVENT_SNAP_RIGHT);
  window_state->OnWMEvent(&snap_right);
  EXPECT_NE(restore_bounds.ToString(), window->GetBoundsInScreen().ToString());
  EXPECT_EQ(restore_bounds.ToString(),
            window_state->GetRestoreBoundsInScreen().ToString());
  window_state->Restore();
  EXPECT_EQ(restore_bounds.ToString(), window->GetBoundsInScreen().ToString());

  // 2) Start with restored bounds set as a result of maximizing the window.
  window_state->Maximize();
  gfx::Rect maximized_bounds = window->GetBoundsInScreen();
  EXPECT_NE(maximized_bounds.ToString(), restore_bounds.ToString());
  EXPECT_EQ(restore_bounds.ToString(),
            window_state->GetRestoreBoundsInScreen().ToString());

  window_state->OnWMEvent(&snap_left);
  EXPECT_NE(restore_bounds.ToString(), window->GetBoundsInScreen().ToString());
  EXPECT_NE(maximized_bounds.ToString(),
            window->GetBoundsInScreen().ToString());
  EXPECT_EQ(restore_bounds.ToString(),
            window_state->GetRestoreBoundsInScreen().ToString());

  window_state->Restore();
  EXPECT_EQ(restore_bounds.ToString(), window->GetBoundsInScreen().ToString());
}

// Test that maximizing an auto managed window, then snapping it puts the window
// at the snapped bounds and not at the auto-managed (centered) bounds.
TEST_F(WindowStateTest, AutoManaged) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  window_state->SetWindowPositionManaged(true);
  window->Hide();
  window->SetBounds(gfx::Rect(100, 100, 100, 100));
  window->Show();

  window_state->Maximize();
  const WMEvent snap_right(WM_EVENT_SNAP_RIGHT);
  window_state->OnWMEvent(&snap_right);

  const gfx::Rect kWorkAreaBounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  gfx::Rect expected_snapped_bounds(
      kWorkAreaBounds.x() + kWorkAreaBounds.width() / 2, kWorkAreaBounds.y(),
      kWorkAreaBounds.width() / 2, kWorkAreaBounds.height());
  EXPECT_EQ(expected_snapped_bounds.ToString(),
            window->GetBoundsInScreen().ToString());

  // The window should still be auto managed despite being right maximized.
  EXPECT_TRUE(window_state->GetWindowPositionManaged());
}

// Test that the replacement of a State object works as expected.
TEST_F(WindowStateTest, SimpleStateSwap) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->IsMaximized());
  window_state->SetStateObject(std::unique_ptr<WindowState::State>(
      new AlwaysMaximizeTestState(window_state->GetStateType())));
  EXPECT_TRUE(window_state->IsMaximized());
}

// Test that the replacement of a state object, following a restore with the
// original one restores the window to its original state.
TEST_F(WindowStateTest, StateSwapRestore) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->IsMaximized());
  std::unique_ptr<WindowState::State> old(
      window_state->SetStateObject(std::unique_ptr<WindowState::State>(
          new AlwaysMaximizeTestState(window_state->GetStateType()))));
  EXPECT_TRUE(window_state->IsMaximized());
  window_state->SetStateObject(std::move(old));
  EXPECT_FALSE(window_state->IsMaximized());
}

// Tests that a window that had same bounds as the work area shrinks after the
// window is maximized and then restored.
TEST_F(WindowStateTest, RestoredWindowBoundsShrink) {
  UpdateDisplay("0+0-600x900");
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->IsMaximized());
  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();

  window->SetBounds(work_area);
  window_state->Maximize();
  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(work_area.ToString(), window->bounds().ToString());

  window_state->Restore();
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_NE(work_area.ToString(), window->bounds().ToString());
  EXPECT_TRUE(work_area.Contains(window->bounds()));
}

TEST_F(WindowStateTest, DoNotResizeMaximizedWindowInFullscreen) {
  std::unique_ptr<aura::Window> maximized(CreateTestWindowInShellWithId(0));
  std::unique_ptr<aura::Window> fullscreen(CreateTestWindowInShellWithId(1));
  WindowState* maximized_state = GetWindowState(maximized.get());
  maximized_state->Maximize();
  ASSERT_TRUE(maximized_state->IsMaximized());
  EXPECT_EQ(gfx::Rect(0, 0, 800, 552).ToString(),
            maximized->GetBoundsInScreen().ToString());

  // Entering fullscreen mode will not update the maximized window's size
  // under fullscreen.
  WMEvent fullscreen_event(WM_EVENT_FULLSCREEN);
  WindowState* fullscreen_state = GetWindowState(fullscreen.get());
  fullscreen_state->OnWMEvent(&fullscreen_event);
  ASSERT_TRUE(fullscreen_state->IsFullscreen());
  ASSERT_TRUE(maximized_state->IsMaximized());
  EXPECT_EQ(gfx::Rect(0, 0, 800, 552).ToString(),
            maximized->GetBoundsInScreen().ToString());

  // Updating display size will update the maximum window size.
  UpdateDisplay("900x700");
  EXPECT_EQ("0,0 900x700", maximized->GetBoundsInScreen().ToString());
  fullscreen.reset();

  // Exiting fullscreen will update the maximized window to the work area.
  EXPECT_EQ(gfx::Rect(0, 0, 900, 652).ToString(),
            maximized->GetBoundsInScreen().ToString());
}

TEST_F(WindowStateTest, TrustedPinned) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->IsTrustedPinned());
  wm::PinWindow(window.get(), true /* trusted */);
  EXPECT_TRUE(window_state->IsTrustedPinned());

  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  EXPECT_EQ(work_area.ToString(), window->bounds().ToString());

  // Sending non-unpin/non-workspace related event should be ignored.
  {
    const WMEvent fullscreen_event(WM_EVENT_FULLSCREEN);
    window_state->OnWMEvent(&fullscreen_event);
  }
  EXPECT_TRUE(window_state->IsTrustedPinned());

  // Update display triggers workspace event.
  UpdateDisplay("300x200");
  EXPECT_EQ("0,0 300x200", window->GetBoundsInScreen().ToString());

  // Unpin should work.
  window_state->Restore();
  EXPECT_FALSE(window_state->IsTrustedPinned());
}

TEST_F(WindowStateTest, AllowSetBoundsDirect) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->IsMaximized());
  gfx::Rect work_area =
      display::Screen::GetScreen()->GetPrimaryDisplay().work_area();
  gfx::Rect original_bounds(50, 50, 200, 200);
  window->SetBounds(original_bounds);
  ASSERT_EQ(original_bounds, window->bounds());

  window_state->set_allow_set_bounds_direct(true);
  window_state->Maximize();

  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(work_area, window->bounds());

  gfx::Rect new_bounds(10, 10, 300, 300);
  window->SetBounds(new_bounds);
  EXPECT_EQ(new_bounds, window->bounds());

  window_state->Restore();
  EXPECT_FALSE(window_state->IsMaximized());
  EXPECT_EQ(original_bounds, window->bounds());

  window_state->set_allow_set_bounds_direct(false);
  window_state->Maximize();

  EXPECT_TRUE(window_state->IsMaximized());
  EXPECT_EQ(work_area, window->bounds());
  window->SetBounds(new_bounds);
  EXPECT_EQ(work_area, window->bounds());
}

TEST_F(WindowStateTest, FullscreenMinimizedSwitching) {
  std::unique_ptr<aura::Window> window(CreateTestWindowInShellWithId(0));
  WindowState* window_state = GetWindowState(window.get());

  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsFullscreen());

  // Toggling the fullscreen window should restore to normal.
  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsNormalStateType());

  window_state->Maximize();
  ASSERT_TRUE(window_state->IsMaximized());

  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsFullscreen());

  // Toggling the fullscreen window should restore to maximized.
  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsMaximized());

  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsFullscreen());

  // Minimize from fullscreen.
  window_state->Minimize();
  ASSERT_TRUE(window_state->IsMinimized());

  // Unminimize should restore to fullscreen.
  window_state->Unminimize();
  ASSERT_TRUE(window_state->IsFullscreen());

  // Toggling the fullscreen window should restore to maximized.
  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsMaximized());

  // Minimize from fullscreen.
  window_state->Minimize();
  ASSERT_TRUE(window_state->IsMinimized());

  // Fullscreen a minimized window.
  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsFullscreen());

  // Toggling the fullscreen window should not return to minimized. It should
  // return to the state before minimizing and fullscreen.
  ash::wm::ToggleFullScreen(window_state, nullptr);
  ASSERT_TRUE(window_state->IsMaximized());
}

TEST_F(WindowStateTest, CanConsumeSystemKeys) {
  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithBounds(gfx::Rect(100, 100, 100, 100)));
  WindowState* window_state = GetWindowState(window.get());

  EXPECT_FALSE(window_state->CanConsumeSystemKeys());

  window->SetProperty(kCanConsumeSystemKeysKey, true);
  EXPECT_TRUE(window_state->CanConsumeSystemKeys());
}

// TODO(skuhne): Add more unit test to verify the correctness for the restore
// operation.

}  // namespace wm
}  // namespace ash
