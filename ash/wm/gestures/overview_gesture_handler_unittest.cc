// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/gestures/overview_gesture_handler.h"

#include "ash/root_window_controller.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "ash/wm/window_util.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"

namespace ash {

class OverviewGestureHandlerTest : public AshTestBase {
 public:
  OverviewGestureHandlerTest() = default;
  ~OverviewGestureHandlerTest() override = default;

  aura::Window* CreateWindow(const gfx::Rect& bounds) {
    return CreateTestWindowInShellWithDelegate(&delegate_, -1, bounds);
  }

  void ToggleOverview() {
    Shell::Get()->window_selector_controller()->ToggleOverview();
  }

  bool IsSelecting() {
    return Shell::Get()->window_selector_controller()->IsSelecting();
  }

  float vertical_threshold_pixels() const {
    return OverviewGestureHandler::vertical_threshold_pixels_;
  }

  float horizontal_threshold_pixels() const {
    return OverviewGestureHandler::horizontal_threshold_pixels_;
  }

 private:
  aura::test::TestWindowDelegate delegate_;

  DISALLOW_COPY_AND_ASSIGN(OverviewGestureHandlerTest);
};

// Tests a three fingers upwards scroll gesture to enter and a scroll down to
// exit overview.
TEST_F(OverviewGestureHandlerTest, VerticalScrolls) {
  gfx::Rect bounds(0, 0, 400, 400);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  ui::test::EventGenerator generator(root_window, root_window);
  const float long_scroll = 2 * vertical_threshold_pixels();
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           0, -long_scroll, 100, 3);
  EXPECT_TRUE(IsSelecting());

  // Swiping up again does nothing.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           0, -long_scroll, 100, 3);
  EXPECT_TRUE(IsSelecting());

  // Swiping down exits.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           0, long_scroll, 100, 3);
  EXPECT_FALSE(IsSelecting());

  // Swiping down again does nothing.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           0, long_scroll, 100, 3);
  EXPECT_FALSE(IsSelecting());
}

// Tests three finger horizontal scroll gesture to move selection left or right.
TEST_F(OverviewGestureHandlerTest, HorizontalScrollInOverview) {
  gfx::Rect bounds(0, 0, 400, 400);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window3(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window4(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window5(CreateWindow(bounds));
  ui::test::EventGenerator generator(root_window, root_window);
  const float vertical_scroll = 2 * vertical_threshold_pixels();
  const float horizontal_scroll = horizontal_threshold_pixels();
  // Enter overview mode as if using an accelerator.
  // Entering overview mode with an upwards three-finger scroll gesture would
  // have the same result (allow selection using horizontal scroll).
  ToggleOverview();
  EXPECT_TRUE(IsSelecting());

  // Long scroll right moves selection to the fourth window.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           horizontal_scroll * 4, 0, 100, 3);
  EXPECT_TRUE(IsSelecting());

  // Short scroll left (3 fingers) moves selection to the third window.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           -horizontal_scroll, 0, 100, 3);
  EXPECT_TRUE(IsSelecting());

  // Short scroll left (3 fingers) moves selection to the second window.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           -horizontal_scroll, 0, 100, 3);
  EXPECT_TRUE(IsSelecting());

  // Swiping down exits and selects the currently-highlighted window.
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           0, vertical_scroll, 100, 3);
  EXPECT_FALSE(IsSelecting());

  // Second MRU window is selected (i.e. |window4|).
  EXPECT_EQ(window4.get(), wm::GetActiveWindow());
}

// Tests that a mostly horizontal three-finger scroll does not trigger overview.
TEST_F(OverviewGestureHandlerTest, HorizontalScrolls) {
  gfx::Rect bounds(0, 0, 400, 400);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  ui::test::EventGenerator generator(root_window, root_window);
  const float long_scroll = 2 * vertical_threshold_pixels();
  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           long_scroll + 100, -long_scroll, 100, 3);
  EXPECT_FALSE(IsSelecting());

  generator.ScrollSequence(gfx::Point(), base::TimeDelta::FromMilliseconds(5),
                           -long_scroll - 100, -long_scroll, 100, 3);
  EXPECT_FALSE(IsSelecting());
}

// Tests a scroll up with three fingers without releasing followed by a scroll
// down by a lesser amount which should still be enough to exit.
TEST_F(OverviewGestureHandlerTest, ScrollUpDownWithoutReleasing) {
  gfx::Rect bounds(0, 0, 400, 400);
  aura::Window* root_window = Shell::GetPrimaryRootWindow();
  std::unique_ptr<aura::Window> window1(CreateWindow(bounds));
  std::unique_ptr<aura::Window> window2(CreateWindow(bounds));
  ui::test::EventGenerator generator(root_window, root_window);
  base::TimeTicks timestamp = base::TimeTicks::Now();
  gfx::Point start;
  int num_fingers = 3;
  base::TimeDelta step_delay(base::TimeDelta::FromMilliseconds(5));
  ui::ScrollEvent fling_cancel(ui::ET_SCROLL_FLING_CANCEL, start, timestamp, 0,
                               0, 0, 0, 0, num_fingers);
  generator.Dispatch(&fling_cancel);

  // Scroll up by 1000px.
  for (int i = 0; i < 100; ++i) {
    timestamp += step_delay;
    ui::ScrollEvent move(ui::ET_SCROLL, start, timestamp, 0, 0, -10, 0, -10,
                         num_fingers);
    generator.Dispatch(&move);
  }

  EXPECT_TRUE(IsSelecting());

  // Without releasing scroll back down by 600px.
  for (int i = 0; i < 60; ++i) {
    timestamp += step_delay;
    ui::ScrollEvent move(ui::ET_SCROLL, start, timestamp, 0, 0, 10, 0, 10,
                         num_fingers);
    generator.Dispatch(&move);
  }

  EXPECT_FALSE(IsSelecting());
  ui::ScrollEvent fling_start(ui::ET_SCROLL_FLING_START, start, timestamp, 0, 0,
                              10, 0, 10, num_fingers);
  generator.Dispatch(&fling_start);
}

}  // namespace ash
