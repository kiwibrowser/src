// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_util.h"

#include "ash/test/ash_test_base.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"

namespace ash {
namespace wm {

namespace {

std::string GetAdjustedBounds(const gfx::Rect& visible,
                              gfx::Rect to_be_adjusted) {
  AdjustBoundsToEnsureMinimumWindowVisibility(visible, &to_be_adjusted);
  return to_be_adjusted.ToString();
}

}  // namespace

using WindowUtilTest = AshTestBase;

TEST_F(WindowUtilTest, CenterWindow) {
  UpdateDisplay("500x400, 600x400");
  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithBounds(gfx::Rect(12, 20, 100, 100)));

  WindowState* window_state = GetWindowState(window.get());
  EXPECT_FALSE(window_state->bounds_changed_by_user());

  CenterWindow(window.get());
  // Centring window is considered as a user's action.
  EXPECT_TRUE(window_state->bounds_changed_by_user());
  EXPECT_EQ("200,126 100x100", window->bounds().ToString());
  EXPECT_EQ("200,126 100x100", window->GetBoundsInScreen().ToString());
  window->SetBoundsInScreen(gfx::Rect(600, 0, 100, 100), GetSecondaryDisplay());
  CenterWindow(window.get());
  EXPECT_EQ("250,126 100x100", window->bounds().ToString());
  EXPECT_EQ("750,126 100x100", window->GetBoundsInScreen().ToString());
}

TEST_F(WindowUtilTest, AdjustBoundsToEnsureMinimumVisibility) {
  const gfx::Rect visible_bounds(0, 0, 100, 100);

  EXPECT_EQ("0,0 90x90",
            GetAdjustedBounds(visible_bounds, gfx::Rect(0, 0, 90, 90)));
  EXPECT_EQ("0,0 100x100",
            GetAdjustedBounds(visible_bounds, gfx::Rect(0, 0, 150, 150)));
  EXPECT_EQ("-50,0 100x100",
            GetAdjustedBounds(visible_bounds, gfx::Rect(-50, -50, 150, 150)));
  EXPECT_EQ("-75,10 100x100",
            GetAdjustedBounds(visible_bounds, gfx::Rect(-100, 10, 150, 150)));
  EXPECT_EQ("75,75 100x100",
            GetAdjustedBounds(visible_bounds, gfx::Rect(100, 100, 150, 150)));

  // For windows that have smaller dimensions than kMinimumOnScreenArea,
  // we should adjust bounds accordingly, leaving no white space.
  EXPECT_EQ("50,80 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(50, 80, 20, 20)));
  EXPECT_EQ("80,50 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(80, 50, 20, 20)));
  EXPECT_EQ("0,50 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(0, 50, 20, 20)));
  EXPECT_EQ("50,0 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(50, 0, 20, 20)));
  EXPECT_EQ("50,80 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(50, 100, 20, 20)));
  EXPECT_EQ("80,50 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(100, 50, 20, 20)));
  EXPECT_EQ("0,50 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(-10, 50, 20, 20)));
  EXPECT_EQ("50,0 20x20",
            GetAdjustedBounds(visible_bounds, gfx::Rect(50, -10, 20, 20)));

  const gfx::Rect visible_bounds_right(200, 50, 100, 100);

  EXPECT_EQ("210,60 90x90", GetAdjustedBounds(visible_bounds_right,
                                              gfx::Rect(210, 60, 90, 90)));
  EXPECT_EQ("210,60 100x100", GetAdjustedBounds(visible_bounds_right,
                                                gfx::Rect(210, 60, 150, 150)));
  EXPECT_EQ("125,50 100x100",
            GetAdjustedBounds(visible_bounds_right, gfx::Rect(0, 0, 150, 150)));
  EXPECT_EQ("275,50 100x100", GetAdjustedBounds(visible_bounds_right,
                                                gfx::Rect(300, 20, 150, 150)));
  EXPECT_EQ(
      "125,125 100x100",
      GetAdjustedBounds(visible_bounds_right, gfx::Rect(-100, 150, 150, 150)));

  const gfx::Rect visible_bounds_left(-200, -50, 100, 100);
  EXPECT_EQ("-190,-40 90x90", GetAdjustedBounds(visible_bounds_left,
                                                gfx::Rect(-190, -40, 90, 90)));
  EXPECT_EQ(
      "-190,-40 100x100",
      GetAdjustedBounds(visible_bounds_left, gfx::Rect(-190, -40, 150, 150)));
  EXPECT_EQ(
      "-250,-40 100x100",
      GetAdjustedBounds(visible_bounds_left, gfx::Rect(-250, -40, 150, 150)));
  EXPECT_EQ(
      "-275,-50 100x100",
      GetAdjustedBounds(visible_bounds_left, gfx::Rect(-400, -60, 150, 150)));
  EXPECT_EQ("-125,0 100x100",
            GetAdjustedBounds(visible_bounds_left, gfx::Rect(0, 0, 150, 150)));
}

TEST_F(WindowUtilTest, MoveWindowToDisplay) {
  UpdateDisplay("500x400, 600x400");
  std::unique_ptr<aura::Window> window(
      CreateTestWindowInShellWithBounds(gfx::Rect(12, 20, 100, 100)));
  display::Screen* screen = display::Screen::GetScreen();
  const int64_t original_display_id =
      screen->GetDisplayNearestWindow(window.get()).id();
  EXPECT_EQ(screen->GetPrimaryDisplay().id(), original_display_id);
  const int original_container_id = window->parent()->id();
  const aura::Window* original_root = window->GetRootWindow();

  EXPECT_FALSE(MoveWindowToDisplay(window.get(), display::kInvalidDisplayId));
  EXPECT_EQ(original_display_id,
            screen->GetDisplayNearestWindow(window.get()).id());
  EXPECT_FALSE(MoveWindowToDisplay(window.get(), original_display_id));
  EXPECT_EQ(original_display_id,
            screen->GetDisplayNearestWindow(window.get()).id());

  ASSERT_EQ(2, screen->GetNumDisplays());
  const int64_t secondary_display_id = screen->GetAllDisplays()[1].id();
  EXPECT_NE(original_display_id, secondary_display_id);
  EXPECT_TRUE(MoveWindowToDisplay(window.get(), secondary_display_id));
  EXPECT_EQ(secondary_display_id,
            screen->GetDisplayNearestWindow(window.get()).id());
  EXPECT_EQ(original_container_id, window->parent()->id());
  EXPECT_NE(original_root, window->GetRootWindow());

  EXPECT_TRUE(MoveWindowToDisplay(window.get(), original_display_id));
  EXPECT_EQ(original_display_id,
            screen->GetDisplayNearestWindow(window.get()).id());
  EXPECT_EQ(original_container_id, window->parent()->id());
  EXPECT_EQ(original_root, window->GetRootWindow());
}

}  // namespace wm
}  // namespace ash
