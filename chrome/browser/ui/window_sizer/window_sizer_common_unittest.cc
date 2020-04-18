// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/window_sizer/window_sizer_common_unittest.h"

#include <stddef.h>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

#if defined(USE_AURA)
#include "ui/aura/window.h"
#endif

namespace {

using util = WindowSizerTestUtil;

// TODO(rjkroege): Use the common TestScreen.
class TestScreen : public display::Screen {
 public:
  TestScreen() : previous_screen_(display::Screen::GetScreen()) {
    display::Screen::SetScreenInstance(this);
  }
  ~TestScreen() override {
    display::Screen::SetScreenInstance(previous_screen_);
  }

  // Sets the index of the display returned from GetDisplayNearestWindow().
  // Only used on aura.
  void set_index_of_display_nearest_window(int index) {
    index_of_display_nearest_window_ = index;
  }

  // Overridden from display::Screen:
  gfx::Point GetCursorScreenPoint() override {
    NOTREACHED();
    return gfx::Point();
  }

  bool IsWindowUnderCursor(gfx::NativeWindow window) override {
    NOTIMPLEMENTED();
    return false;
  }

  gfx::NativeWindow GetWindowAtScreenPoint(const gfx::Point& point) override {
    NOTREACHED();
    return NULL;
  }

  int GetNumDisplays() const override { return displays_.size(); }

  const std::vector<display::Display>& GetAllDisplays() const override {
    return displays_;
  }

  display::Display GetDisplayNearestWindow(
      gfx::NativeWindow window) const override {
#if defined(USE_AURA)
    return displays_[index_of_display_nearest_window_];
#else
    NOTREACHED();
    return display::Display();
#endif
  }

  display::Display GetDisplayNearestPoint(
      const gfx::Point& point) const override {
    NOTREACHED();
    return display::Display();
  }

  display::Display GetDisplayMatching(
      const gfx::Rect& match_rect) const override {
    int max_area = 0;
    size_t max_area_index = 0;

    for (size_t i = 0; i < displays_.size(); ++i) {
      gfx::Rect overlap = displays_[i].bounds();
      overlap.Intersect(match_rect);
      int area = overlap.width() * overlap.height();
      if (area > max_area) {
        max_area = area;
        max_area_index = i;
      }
    }
    return displays_[max_area_index];
  }

  display::Display GetPrimaryDisplay() const override { return displays_[0]; }

  void AddObserver(display::DisplayObserver* observer) override {
    NOTREACHED();
  }

  void RemoveObserver(display::DisplayObserver* observer) override {
    NOTREACHED();
  }

  void AddDisplay(const gfx::Rect& bounds,
                  const gfx::Rect& work_area) {
    display::Display display(displays_.size(), bounds);
    display.set_work_area(work_area);
    displays_.push_back(display);
  }

 private:
  display::Screen* previous_screen_;
  size_t index_of_display_nearest_window_ = 0u;
  std::vector<display::Display> displays_;

  DISALLOW_COPY_AND_ASSIGN(TestScreen);
};

}  // namespace

TestStateProvider::TestStateProvider()
    : has_persistent_data_(false),
      persistent_show_state_(ui::SHOW_STATE_DEFAULT),
      has_last_active_data_(false),
      last_active_show_state_(ui::SHOW_STATE_DEFAULT) {}

void TestStateProvider::SetPersistentState(const gfx::Rect& bounds,
                                           const gfx::Rect& work_area,
                                           ui::WindowShowState show_state,
                                           bool has_persistent_data) {
  persistent_bounds_ = bounds;
  persistent_work_area_ = work_area;
  persistent_show_state_ = show_state;
  has_persistent_data_ = has_persistent_data;
}

void TestStateProvider::SetLastActiveState(const gfx::Rect& bounds,
                                           ui::WindowShowState show_state,
                                           bool has_last_active_data) {
  last_active_bounds_ = bounds;
  last_active_show_state_ = show_state;
  has_last_active_data_ = has_last_active_data;
}

bool TestStateProvider::GetPersistentState(
    gfx::Rect* bounds,
    gfx::Rect* saved_work_area,
    ui::WindowShowState* show_state) const {
  DCHECK(show_state);
  *bounds = persistent_bounds_;
  *saved_work_area = persistent_work_area_;
  if (*show_state == ui::SHOW_STATE_DEFAULT)
    *show_state = persistent_show_state_;
  return has_persistent_data_;
}

bool TestStateProvider::GetLastActiveWindowState(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  DCHECK(show_state);
  *bounds = last_active_bounds_;
  if (*show_state == ui::SHOW_STATE_DEFAULT)
    *show_state = last_active_show_state_;
  return has_last_active_data_;
}

int kWindowTilePixels = WindowSizer::kWindowTilePixels;

// static
void WindowSizerTestUtil::GetWindowBoundsAndShowState(
    const gfx::Rect& monitor1_bounds,
    const gfx::Rect& monitor1_work_area,
    const gfx::Rect& monitor2_bounds,
    const gfx::Rect& bounds,
    const gfx::Rect& work_area,
    ui::WindowShowState show_state_persisted,
    ui::WindowShowState show_state_last,
    Source source,
    const Browser* browser,
    const gfx::Rect& passed_in,
    size_t display_index,
    gfx::Rect* out_bounds,
    ui::WindowShowState* out_show_state) {
  DCHECK(out_show_state);
  TestScreen test_screen;
  test_screen.AddDisplay(monitor1_bounds, monitor1_work_area);
  if (!monitor2_bounds.IsEmpty())
    test_screen.AddDisplay(monitor2_bounds, monitor2_bounds);
  test_screen.set_index_of_display_nearest_window(display_index);
  std::unique_ptr<TestStateProvider> sp(new TestStateProvider);
  if (source == PERSISTED || source == BOTH)
    sp->SetPersistentState(bounds, work_area, show_state_persisted, true);
  if (source == LAST_ACTIVE || source == BOTH)
    sp->SetLastActiveState(bounds, show_state_last, true);
  std::unique_ptr<WindowSizer::TargetDisplayProvider> tdp(
      new WindowSizer::DefaultTargetDisplayProvider);

  WindowSizer sizer(std::move(sp), std::move(tdp), &test_screen, browser);
  sizer.DetermineWindowBoundsAndShowState(passed_in,
                                          out_bounds,
                                          out_show_state);
}

// static
ui::WindowShowState WindowSizerTestUtil::GetWindowShowState(
    ui::WindowShowState show_state_persisted,
    ui::WindowShowState show_state_last,
    Source source,
    const Browser* browser,
    const gfx::Rect& bounds,
    const gfx::Rect& display_config) {
  TestScreen test_screen;
  test_screen.AddDisplay(display_config, display_config);
  std::unique_ptr<TestStateProvider> sp(new TestStateProvider);
  if (source == PERSISTED || source == BOTH)
    sp->SetPersistentState(bounds, display_config, show_state_persisted, true);
  if (source == LAST_ACTIVE || source == BOTH)
    sp->SetLastActiveState(bounds, show_state_last, true);
  std::unique_ptr<WindowSizer::TargetDisplayProvider> tdp(
      new WindowSizer::DefaultTargetDisplayProvider);

  WindowSizer sizer(std::move(sp), std::move(tdp), &test_screen, browser);

  ui::WindowShowState out_show_state = ui::SHOW_STATE_DEFAULT;
  gfx::Rect out_bounds;
  sizer.DetermineWindowBoundsAndShowState(
      gfx::Rect(),
      &out_bounds,
      &out_show_state);
  return out_show_state;
}

// static
void WindowSizerTestUtil::GetWindowBounds(const gfx::Rect& monitor1_bounds,
                                          const gfx::Rect& monitor1_work_area,
                                          const gfx::Rect& monitor2_bounds,
                                          const gfx::Rect& bounds,
                                          const gfx::Rect& work_area,
                                          Source source,
                                          const Browser* browser,
                                          const gfx::Rect& passed_in,
                                          gfx::Rect* out_bounds) {
  ui::WindowShowState out_show_state = ui::SHOW_STATE_DEFAULT;
  GetWindowBoundsAndShowState(
      monitor1_bounds, monitor1_work_area, monitor2_bounds, bounds, work_area,
      ui::SHOW_STATE_DEFAULT, ui::SHOW_STATE_DEFAULT, source, browser,
      passed_in, 0u, out_bounds, &out_show_state);
}

#if !defined(OS_MACOSX)

#if !defined(OS_CHROMEOS)
// Passing null for the browser parameter of GetWindowBounds makes the test skip
// all Ash-specific logic, so there's no point running this on Chrome OS.
TEST(WindowSizerTestCommon,
     PersistedWindowOffscreenWithNonAggressiveRepositioning) {
  { // off the left but the minimum visibility condition is barely satisfied
    // without relocaiton.
    gfx::Rect initial_bounds(-470, 50, 500, 400);

    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(), initial_bounds,
                          gfx::Rect(), PERSISTED, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(initial_bounds.ToString(), window_bounds.ToString());
  }

  { // off the left and the minimum visibility condition is satisfied by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(-471, 50, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(-470 /* not -471 */, 50, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // off the top
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, -370, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("50,0 500x400", window_bounds.ToString());
  }

  { // off the right but the minimum visibility condition is barely satisified
    // without relocation.
    gfx::Rect initial_bounds(994, 50, 500, 400);

    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(), initial_bounds,
                          gfx::Rect(), PERSISTED, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(initial_bounds.ToString(), window_bounds.ToString());
  }

  { // off the right and the minimum visibility condition is satisified by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(995, 50, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 995 */, 50, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // off the bottom but the minimum visibility condition is barely satisified
    // without relocation.
    gfx::Rect initial_bounds(50, 738, 500, 400);

    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(), initial_bounds,
                          gfx::Rect(), PERSISTED, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(initial_bounds.ToString(), window_bounds.ToString());
  }

  { // off the bottom and the minimum visibility condition is satisified by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, 739, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(50, 738 /* not 739 */, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // off the topleft
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(-471, -371, 500, 400), gfx::Rect(),
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(-470 /* not -471 */, 0, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // off the topright and the minimum visibility condition is satisified by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(995, -371, 500, 400), gfx::Rect(),
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 995 */, 0, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // off the bottomleft and the minimum visibility condition is satisified by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(-471, 739, 500, 400), gfx::Rect(),
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(-470 /* not -471 */,
                        738 /* not 739 */,
                        500,
                        400).ToString(),
              window_bounds.ToString());
  }

  { // off the bottomright and the minimum visibility condition is satisified by
    // relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(995, 739, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 995 */,
                        738 /* not 739 */,
                        500,
                        400).ToString(),
              window_bounds.ToString());
  }

  { // entirely off left
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(-700, 50, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(-470 /* not -700 */, 50, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // entirely off left (monitor was detached since last run)
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(-700, 50, 500, 400), left_s1024x768,
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("0,50 500x400", window_bounds.ToString());
  }

  { // entirely off top
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, -500, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("50,0 500x400", window_bounds.ToString());
  }

  { // entirely off top (monitor was detached since last run)
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, -500, 500, 400), top_s1024x768,
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("50,0 500x400", window_bounds.ToString());
  }

  { // entirely off right
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(1200, 50, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 1200 */, 50, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // entirely off right (monitor was detached since last run)
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(1200, 50, 500, 400), right_s1024x768,
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("524,50 500x400", window_bounds.ToString());
  }

  { // entirely off bottom
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, 800, 500, 400), gfx::Rect(), PERSISTED,
                          NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(50, 738 /* not 800 */, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // entirely off bottom (monitor was detached since last run)
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(50, 800, 500, 400), bottom_s1024x768,
                          PERSISTED, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ("50,368 500x400", window_bounds.ToString());
  }
}
#endif  // !defined(OS_CHROMEOS)

// Test that the window is sized appropriately for the first run experience
// where the default window bounds calculation is invoked.
TEST(WindowSizerTestCommon, AdjustFitSize) {
  { // Check that the window gets resized to the screen.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, p1024x768, gfx::Rect(), gfx::Rect(), gfx::Rect(), DEFAULT,
        NULL, gfx::Rect(-10, -10, 1024 + 20, 768 + 20), &window_bounds);
    EXPECT_EQ("0,0 1024x768", window_bounds.ToString());
  }

  { // Check that a window which hangs out of the screen get moved back in.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL,
                          gfx::Rect(1020, 700, 100, 100), &window_bounds);
    EXPECT_EQ("924,668 100x100", window_bounds.ToString());
  }
}

#endif // defined(OS_MACOSX)
