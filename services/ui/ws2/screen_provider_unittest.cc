// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/screen_provider.h"

#include <stdint.h>

#include "base/strings/string_number_conversions.h"
#include "services/ui/common/task_runner_test_base.h"
#include "services/ui/public/interfaces/screen_provider.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/display/display.h"
#include "ui/display/screen_base.h"

using display::Display;
using display::DisplayList;
using display::DisplayObserver;

namespace ui {
namespace ws2 {
namespace {

std::string DisplayIdsToString(
    const std::vector<mojom::WsDisplayPtr>& wm_displays) {
  std::string display_ids;
  for (const auto& wm_display : wm_displays) {
    if (!display_ids.empty())
      display_ids += " ";
    display_ids += base::NumberToString(wm_display->display.id());
  }
  return display_ids;
}

// A testing screen that generates the OnDidProcessDisplayChanges() notification
// similar to production code.
class TestScreen : public display::ScreenBase {
 public:
  TestScreen() { display::Screen::SetScreenInstance(this); }

  ~TestScreen() override { display::Screen::SetScreenInstance(nullptr); }

  void AddDisplay(const Display& display, DisplayList::Type type) {
    display_list().AddDisplay(display, type);
    for (DisplayObserver& observer : *display_list().observers())
      observer.OnDidProcessDisplayChanges();
  }

  void UpdateDisplay(const Display& display, DisplayList::Type type) {
    display_list().UpdateDisplay(display, type);
    for (DisplayObserver& observer : *display_list().observers())
      observer.OnDidProcessDisplayChanges();
  }

  void RemoveDisplay(int64_t display_id) {
    display_list().RemoveDisplay(display_id);
    for (DisplayObserver& observer : *display_list().observers())
      observer.OnDidProcessDisplayChanges();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestScreen);
};

class TestScreenProviderObserver : public mojom::ScreenProviderObserver {
 public:
  TestScreenProviderObserver() = default;
  ~TestScreenProviderObserver() override = default;

  mojom::ScreenProviderObserverPtr GetPtr() {
    mojom::ScreenProviderObserverPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // mojom::ScreenProviderObserver:
  void OnDisplaysChanged(std::vector<mojom::WsDisplayPtr> displays,
                         int64_t primary_display_id,
                         int64_t internal_display_id) override {
    displays_ = std::move(displays);
    display_ids_ = DisplayIdsToString(displays_);
    primary_display_id_ = primary_display_id;
    internal_display_id_ = internal_display_id;
  }

  mojo::Binding<mojom::ScreenProviderObserver> binding_{this};
  std::vector<mojom::WsDisplayPtr> displays_;
  std::string display_ids_;
  int64_t primary_display_id_ = 0;
  int64_t internal_display_id_ = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestScreenProviderObserver);
};

// Mojo needs a task runner.
using ScreenProviderTest = TaskRunnerTestBase;

TEST_F(ScreenProviderTest, AddRemoveDisplay) {
  TestScreen screen;
  screen.AddDisplay(Display(111, gfx::Rect(0, 0, 640, 480)),
                    DisplayList::Type::PRIMARY);
  Display::SetInternalDisplayId(111);

  ScreenProvider screen_provider;
  TestScreenProviderObserver observer;

  // Adding an observer triggers an update.
  screen_provider.AddObserver(observer.GetPtr());
  // Wait for mojo message to observer.
  RunUntilIdle();
  EXPECT_EQ("111", observer.display_ids_);
  EXPECT_EQ(111, observer.primary_display_id_);
  EXPECT_EQ(111, observer.internal_display_id_);
  observer.display_ids_.clear();

  // Adding a display triggers an update.
  screen.AddDisplay(Display(222, gfx::Rect(640, 0, 640, 480)),
                    DisplayList::Type::NOT_PRIMARY);
  // Wait for mojo message to observer.
  RunUntilIdle();
  EXPECT_EQ("111 222", observer.display_ids_);
  EXPECT_EQ(111, observer.primary_display_id_);
  observer.display_ids_.clear();

  // Updating which display is primary triggers an update.
  screen.UpdateDisplay(Display(222, gfx::Rect(640, 0, 800, 600)),
                       DisplayList::Type::PRIMARY);
  // Wait for mojo message to observer.
  RunUntilIdle();
  EXPECT_EQ("111 222", observer.display_ids_);
  EXPECT_EQ(222, observer.primary_display_id_);
  observer.display_ids_.clear();

  // Removing a display triggers an update.
  screen.RemoveDisplay(111);
  // Wait for mojo message to observer.
  RunUntilIdle();
  EXPECT_EQ("222", observer.display_ids_);
  EXPECT_EQ(222, observer.primary_display_id_);
}

TEST_F(ScreenProviderTest, SetFrameDecorationValues) {
  // Set up a single display.
  TestScreen screen;
  screen.AddDisplay(Display(111, gfx::Rect(0, 0, 640, 480)),
                    DisplayList::Type::PRIMARY);

  // Set up custom frame decoration values.
  ScreenProvider screen_provider;
  screen_provider.SetFrameDecorationValues(gfx::Insets(1, 2, 3, 4), 55u);

  // Add an observer to the screen provider.
  TestScreenProviderObserver observer;
  screen_provider.AddObserver(observer.GetPtr());
  // Wait for mojo message to observer.
  RunUntilIdle();

  // The screen information contains the frame decoration values.
  ASSERT_EQ(1u, observer.displays_.size());
  const mojom::FrameDecorationValuesPtr& values =
      observer.displays_[0]->frame_decoration_values;
  EXPECT_EQ(gfx::Insets(1, 2, 3, 4), values->normal_client_area_insets);
  EXPECT_EQ(gfx::Insets(1, 2, 3, 4), values->maximized_client_area_insets);
  EXPECT_EQ(55u, values->max_title_bar_button_width);
}

}  // namespace
}  // namespace ws2
}  // namespace ui
