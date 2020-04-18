// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/extended_mouse_warp_controller.h"

#include "ash/display/mouse_cursor_event_filter.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ui/display/display.h"
#include "ui/display/display_layout.h"
#include "ui/display/display_layout_builder.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/display/test/display_manager_test_api.h"
#include "ui/events/test/event_generator.h"

namespace ash {

class ExtendedMouseWarpControllerTest : public AshTestBase {
 public:
  ExtendedMouseWarpControllerTest() = default;
  ~ExtendedMouseWarpControllerTest() override = default;

 protected:
  MouseCursorEventFilter* event_filter() {
    return Shell::Get()->mouse_cursor_filter();
  }

  ExtendedMouseWarpController* mouse_warp_controller() {
    return static_cast<ExtendedMouseWarpController*>(
        event_filter()->mouse_warp_controller_for_test());
  }

  size_t GetWarpRegionsCount() {
    return mouse_warp_controller()->warp_regions_.size();
  }

  const ExtendedMouseWarpController::WarpRegion* GetWarpRegion(size_t index) {
    return mouse_warp_controller()->warp_regions_[index].get();
  }

  const gfx::Rect& GetIndicatorBounds(int64_t id) {
    return GetWarpRegion(0)->GetIndicatorBoundsForTest(id);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ExtendedMouseWarpControllerTest);
};

// Verifies if MouseCursorEventFilter's bounds calculation works correctly.
TEST_F(ExtendedMouseWarpControllerTest, IndicatorBoundsTestOnRight) {
  UpdateDisplay("360x360,700x700");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  int64_t display_0_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[0])
                             .id();
  int64_t display_1_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[1])
                             .id();

  std::unique_ptr<display::DisplayLayout> layout(
      display::test::CreateDisplayLayout(display_manager(),
                                         display::DisplayPlacement::RIGHT, 0));

  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);

  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 32, 1, 328), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 0, 1, 360), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  EXPECT_EQ(gfx::Rect(359, 0, 1, 360), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 32, 1, 328), GetIndicatorBounds(display_1_id));

  // Move 2nd display downwards a bit.
  layout->placement_list[0].offset = 5;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  // This is same as before because the 2nd display's y is above
  // the indicator's x.
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 32, 1, 328), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 5, 1, 355), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  EXPECT_EQ(gfx::Rect(359, 5, 1, 355), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 37, 1, 323), GetIndicatorBounds(display_1_id));

  // Move it down further so that the shared edge is shorter than
  // minimum hole size (160).
  layout->placement_list[0].offset = 200;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 200, 1, 160), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 200, 1, 160), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 200, 1, 160), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 200, 1, 160), GetIndicatorBounds(display_1_id));

  // Now move 2nd display upwards.
  layout->placement_list[0].offset = -5;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 32, 1, 328), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 0, 1, 360), GetIndicatorBounds(display_1_id));
  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  // 32 px are reserved on 2nd display from top, so y must be
  // (32 - 5) = 27.
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(359, 0, 1, 360), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 27, 1, 333), GetIndicatorBounds(display_1_id));

  event_filter()->HideSharedEdgeIndicator();
}

TEST_F(ExtendedMouseWarpControllerTest, IndicatorBoundsTestOnLeft) {
  UpdateDisplay("360x360,700x700");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();

  int64_t display_0_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[0])
                             .id();
  int64_t display_1_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[1])
                             .id();

  std::unique_ptr<display::DisplayLayout> layout(
      display::test::CreateDisplayLayout(display_manager(),
                                         display::DisplayPlacement::LEFT, 0));
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());

  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 32, 1, 328), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(-1, 0, 1, 360), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 0, 1, 360), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(-1, 32, 1, 328), GetIndicatorBounds(display_1_id));

  layout->placement_list[0].offset = 250;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 250, 1, 110), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(-1, 250, 1, 110), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 250, 1, 110), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(-1, 250, 1, 110), GetIndicatorBounds(display_1_id));

  event_filter()->HideSharedEdgeIndicator();
}

TEST_F(ExtendedMouseWarpControllerTest, IndicatorBoundsTestOnTopBottom) {
  UpdateDisplay("360x360,700x700");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  int64_t display_0_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[0])
                             .id();
  int64_t display_1_id = display::Screen::GetScreen()
                             ->GetDisplayNearestWindow(root_windows[1])
                             .id();

  std::unique_ptr<display::DisplayLayout> layout(
      display::test::CreateDisplayLayout(display_manager(),
                                         display::DisplayPlacement::TOP, 0));
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 0, 360, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(0, -1, 360, 1), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 0, 360, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(0, -1, 360, 1), GetIndicatorBounds(display_1_id));

  layout->placement_list[0].offset = 250;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(250, 0, 110, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(250, -1, 110, 1), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(250, 0, 110, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(250, -1, 110, 1), GetIndicatorBounds(display_1_id));

  layout->placement_list[0].position = display::DisplayPlacement::BOTTOM;
  layout->placement_list[0].offset = 0;
  display_manager()->SetLayoutForCurrentDisplays(layout->Copy());
  event_filter()->ShowSharedEdgeIndicator(root_windows[0] /* primary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 359, 360, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(0, 360, 360, 1), GetIndicatorBounds(display_1_id));

  event_filter()->ShowSharedEdgeIndicator(root_windows[1] /* secondary */);
  ASSERT_EQ(1U, GetWarpRegionsCount());
  EXPECT_EQ(gfx::Rect(0, 359, 360, 1), GetIndicatorBounds(display_0_id));
  EXPECT_EQ(gfx::Rect(0, 360, 360, 1), GetIndicatorBounds(display_1_id));

  event_filter()->HideSharedEdgeIndicator();
}

// Verify indicators show up as expected with 3+ displays.
TEST_F(ExtendedMouseWarpControllerTest, IndicatorBoundsTestThreeDisplays) {
  UpdateDisplay("360x360,700x700,1000x1000");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  display::Screen* screen = display::Screen::GetScreen();
  int64_t display_0_id = screen->GetDisplayNearestWindow(root_windows[0]).id();
  int64_t display_1_id = screen->GetDisplayNearestWindow(root_windows[1]).id();
  int64_t display_2_id = screen->GetDisplayNearestWindow(root_windows[2]).id();

  // Drag from left most display
  event_filter()->ShowSharedEdgeIndicator(root_windows[0]);
  ASSERT_EQ(2U, GetWarpRegionsCount());
  const ExtendedMouseWarpController::WarpRegion* region_0 = GetWarpRegion(0);
  const ExtendedMouseWarpController::WarpRegion* region_1 = GetWarpRegion(1);
  EXPECT_EQ(gfx::Rect(359, 32, 1, 328),
            region_1->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 0, 1, 360),
            region_1->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1059, 0, 1, 700),
            region_0->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1060, 0, 1, 700),
            region_0->GetIndicatorBoundsForTest(display_2_id));

  // Drag from middle display
  event_filter()->ShowSharedEdgeIndicator(root_windows[1]);
  ASSERT_EQ(2U, mouse_warp_controller()->warp_regions_.size());
  region_0 = GetWarpRegion(0);
  region_1 = GetWarpRegion(1);
  EXPECT_EQ(gfx::Rect(359, 0, 1, 360),
            region_1->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 32, 1, 328),
            region_1->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1059, 32, 1, 668),
            region_0->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1060, 0, 1, 700),
            region_0->GetIndicatorBoundsForTest(display_2_id));

  // Right most display
  event_filter()->ShowSharedEdgeIndicator(root_windows[2]);
  ASSERT_EQ(2U, mouse_warp_controller()->warp_regions_.size());
  region_0 = GetWarpRegion(0);
  region_1 = GetWarpRegion(1);
  EXPECT_EQ(gfx::Rect(359, 0, 1, 360),
            region_1->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(360, 0, 1, 360),
            region_1->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1059, 0, 1, 700),
            region_0->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(1060, 32, 1, 668),
            region_0->GetIndicatorBoundsForTest(display_2_id));
  event_filter()->HideSharedEdgeIndicator();
  // TODO(oshima): Add test cases primary swap.
}

TEST_F(ExtendedMouseWarpControllerTest,
       IndicatorBoundsTestThreeDisplaysWithLayout) {
  UpdateDisplay("700x500,500x500,1000x1000");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  display::Screen* screen = display::Screen::GetScreen();
  int64_t display_0_id = screen->GetDisplayNearestWindow(root_windows[0]).id();
  int64_t display_1_id = screen->GetDisplayNearestWindow(root_windows[1]).id();
  int64_t display_2_id = screen->GetDisplayNearestWindow(root_windows[2]).id();

  // Layout so that all displays touches togter like this:
  //  +-----+---+
  //  |  0  | 1 |
  //  +-+---+--++
  //    |  2   |
  //    +------+
  display::DisplayLayoutBuilder builder(display_0_id);
  builder.AddDisplayPlacement(display_1_id, display_0_id,
                              display::DisplayPlacement::RIGHT, 0);
  builder.AddDisplayPlacement(display_2_id, display_0_id,
                              display::DisplayPlacement::BOTTOM, 100);

  display_manager()->SetLayoutForCurrentDisplays(builder.Build());
  ASSERT_EQ(3U, GetWarpRegionsCount());

  // Drag from 0.
  event_filter()->ShowSharedEdgeIndicator(root_windows[0]);
  ASSERT_EQ(3U, GetWarpRegionsCount());
  const ExtendedMouseWarpController::WarpRegion* region_0 = GetWarpRegion(0);
  const ExtendedMouseWarpController::WarpRegion* region_1 = GetWarpRegion(1);
  const ExtendedMouseWarpController::WarpRegion* region_2 = GetWarpRegion(2);
  // between 2 and 0
  EXPECT_EQ(gfx::Rect(100, 499, 600, 1),
            region_0->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(100, 500, 600, 1),
            region_0->GetIndicatorBoundsForTest(display_2_id));
  // between 2 and 1
  EXPECT_EQ(gfx::Rect(700, 499, 400, 1),
            region_1->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(700, 500, 400, 1),
            region_1->GetIndicatorBoundsForTest(display_2_id));
  // between 1 and 0
  EXPECT_EQ(gfx::Rect(699, 32, 1, 468),
            region_2->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(700, 0, 1, 500),
            region_2->GetIndicatorBoundsForTest(display_1_id));
  event_filter()->HideSharedEdgeIndicator();
}

TEST_F(ExtendedMouseWarpControllerTest,
       IndicatorBoundsTestThreeDisplaysWithLayout2) {
  UpdateDisplay("700x500,500x500,1000x1000");
  aura::Window::Windows root_windows = Shell::GetAllRootWindows();
  display::Screen* screen = display::Screen::GetScreen();
  int64_t display_0_id = screen->GetDisplayNearestWindow(root_windows[0]).id();
  int64_t display_1_id = screen->GetDisplayNearestWindow(root_windows[1]).id();
  int64_t display_2_id = screen->GetDisplayNearestWindow(root_windows[2]).id();

  // Layout so that 0 and 1 displays are disconnected.
  //  +-----+ +---+
  //  |  0  | |1 |
  //  +-+---+-+++
  //    |  2   |
  //    +------+
  display::DisplayLayoutBuilder builder(display_0_id);
  builder.AddDisplayPlacement(display_2_id, display_0_id,
                              display::DisplayPlacement::BOTTOM, 100);
  builder.AddDisplayPlacement(display_1_id, display_2_id,
                              display::DisplayPlacement::TOP, 800);

  display_manager()->SetLayoutForCurrentDisplays(builder.Build());
  ASSERT_EQ(2U, GetWarpRegionsCount());

  // Drag from 0.
  event_filter()->ShowSharedEdgeIndicator(root_windows[0]);
  ASSERT_EQ(2U, GetWarpRegionsCount());
  const ExtendedMouseWarpController::WarpRegion* region_0 = GetWarpRegion(0);
  const ExtendedMouseWarpController::WarpRegion* region_1 = GetWarpRegion(1);
  // between 2 and 0
  EXPECT_EQ(gfx::Rect(100, 499, 600, 1),
            region_0->GetIndicatorBoundsForTest(display_0_id));
  EXPECT_EQ(gfx::Rect(100, 500, 600, 1),
            region_0->GetIndicatorBoundsForTest(display_2_id));
  // between 2 and 1
  EXPECT_EQ(gfx::Rect(900, 499, 200, 1),
            region_1->GetIndicatorBoundsForTest(display_1_id));
  EXPECT_EQ(gfx::Rect(900, 500, 200, 1),
            region_1->GetIndicatorBoundsForTest(display_2_id));
  event_filter()->HideSharedEdgeIndicator();
}

}  // namespace ash
