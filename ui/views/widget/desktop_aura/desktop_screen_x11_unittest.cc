// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/desktop_screen_x11.h"

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/hit_test.h"
#include "ui/display/display_observer.h"
#include "ui/events/test/event_generator.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/x/x11_atom_cache.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host_x11.h"

namespace {

// Class which allows for the designation of non-client component targets of
// hit tests.
class TestDesktopNativeWidgetAura : public views::DesktopNativeWidgetAura {
 public:
  explicit TestDesktopNativeWidgetAura(
      views::internal::NativeWidgetDelegate* delegate)
      : views::DesktopNativeWidgetAura(delegate) {}
  ~TestDesktopNativeWidgetAura() override {}

  void set_window_component(int window_component) {
    window_component_ = window_component;
  }

  // DesktopNativeWidgetAura:
  int GetNonClientComponent(const gfx::Point& point) const override {
    return window_component_;
  }

 private:
  int window_component_;

  DISALLOW_COPY_AND_ASSIGN(TestDesktopNativeWidgetAura);
};

}  // namespace

namespace views {

const int64_t kFirstDisplay = 5321829;
const int64_t kSecondDisplay = 928310;

class DesktopScreenX11Test : public views::ViewsTestBase,
                             public display::DisplayObserver {
 public:
  DesktopScreenX11Test() {}
  ~DesktopScreenX11Test() override {}

  // Overridden from testing::Test:
  void SetUp() override {
    ViewsTestBase::SetUp();
    // Initialize the world to the single monitor case.
    std::vector<display::Display> displays;
    displays.push_back(
        display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
    screen_.reset(new DesktopScreenX11(displays));
    screen_->AddObserver(this);
  }

  void TearDown() override {
    screen_.reset();
    ViewsTestBase::TearDown();
  }

 protected:
  std::vector<display::Display> changed_display_;
  std::vector<display::Display> added_display_;
  std::vector<display::Display> removed_display_;

  DesktopScreenX11* screen() { return screen_.get(); }

  void NotifyDisplaysChanged(const std::vector<display::Display>& displays) {
    std::vector<display::Display> old_displays = screen_->displays_;
    screen_->SetDisplaysInternal(displays);
    screen_->change_notifier_.NotifyDisplaysChanged(old_displays,
                                                    screen_->displays_);
  }

  void ResetDisplayChanges() {
    changed_display_.clear();
    added_display_.clear();
    removed_display_.clear();
  }

  Widget* BuildTopLevelDesktopWidget(const gfx::Rect& bounds,
      bool use_test_native_widget) {
    Widget* toplevel = new Widget;
    Widget::InitParams toplevel_params =
        CreateParams(Widget::InitParams::TYPE_WINDOW);
    if (use_test_native_widget) {
      toplevel_params.native_widget =
          new TestDesktopNativeWidgetAura(toplevel);
    } else {
      toplevel_params.native_widget =
          new views::DesktopNativeWidgetAura(toplevel);
    }
    toplevel_params.bounds = bounds;
    toplevel_params.remove_standard_frame = true;
    toplevel->Init(toplevel_params);
    return toplevel;
  }

 private:
  // Overridden from display::DisplayObserver:
  void OnDisplayAdded(const display::Display& new_display) override {
    added_display_.push_back(new_display);
  }

  void OnDisplayRemoved(const display::Display& old_display) override {
    removed_display_.push_back(old_display);
  }

  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override {
    changed_display_.push_back(display);
  }

  std::unique_ptr<DesktopScreenX11> screen_;

  DISALLOW_COPY_AND_ASSIGN(DesktopScreenX11Test);
};

TEST_F(DesktopScreenX11Test, BoundsChangeSingleMonitor) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(1u, changed_display_.size());
  EXPECT_EQ(0u, added_display_.size());
  EXPECT_EQ(0u, removed_display_.size());
}

TEST_F(DesktopScreenX11Test, AddMonitorToTheRight) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(0u, changed_display_.size());
  EXPECT_EQ(1u, added_display_.size());
  EXPECT_EQ(0u, removed_display_.size());
}

TEST_F(DesktopScreenX11Test, AddMonitorToTheLeft) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(0, 0, 1024, 768)));
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(1024, 0, 640, 480)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(1u, changed_display_.size());
  EXPECT_EQ(1u, added_display_.size());
  EXPECT_EQ(0u, removed_display_.size());
}

TEST_F(DesktopScreenX11Test, RemoveMonitorOnRight) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  ResetDisplayChanges();

  displays.clear();
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(0u, changed_display_.size());
  EXPECT_EQ(0u, added_display_.size());
  EXPECT_EQ(1u, removed_display_.size());
}

TEST_F(DesktopScreenX11Test, RemoveMonitorOnLeft) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  ResetDisplayChanges();

  displays.clear();
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(0, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(1u, changed_display_.size());
  EXPECT_EQ(0u, added_display_.size());
  EXPECT_EQ(1u, removed_display_.size());
}

TEST_F(DesktopScreenX11Test, GetDisplayNearestPoint) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(kFirstDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(630, 10)).id());
  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(650, 10)).id());
  EXPECT_EQ(kFirstDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(10, 10)).id());
  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(10000, 10000)).id());
  EXPECT_EQ(kFirstDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(639, -10)).id());
  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(641, -20)).id());
  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(600, 760)).id());
  EXPECT_EQ(kFirstDisplay,
            screen()->GetDisplayNearestPoint(gfx::Point(-1000, 760)).id());
}

TEST_F(DesktopScreenX11Test, GetDisplayMatchingBasic) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayMatching(gfx::Rect(700, 20, 100, 100)).id());
}

TEST_F(DesktopScreenX11Test, GetDisplayMatchingOverlap) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  EXPECT_EQ(kSecondDisplay,
            screen()->GetDisplayMatching(gfx::Rect(630, 20, 100, 100)).id());
}

TEST_F(DesktopScreenX11Test, GetPrimaryDisplay) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(640, 0, 1024, 768)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(0, 0, 640, 480)));
  NotifyDisplaysChanged(displays);

  // The first display in the list is always the primary, even if other
  // displays are to the left in screen layout.
  EXPECT_EQ(kFirstDisplay, screen()->GetPrimaryDisplay().id());
}

TEST_F(DesktopScreenX11Test, GetDisplayNearestWindow) {
  // Set up a two monitor situation.
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);

  Widget* window_one = BuildTopLevelDesktopWidget(gfx::Rect(10, 10, 10, 10),
      false);
  Widget* window_two = BuildTopLevelDesktopWidget(gfx::Rect(650, 50, 10, 10),
      false);

  EXPECT_EQ(
      kFirstDisplay,
      screen()->GetDisplayNearestWindow(window_one->GetNativeWindow()).id());
  EXPECT_EQ(
      kSecondDisplay,
      screen()->GetDisplayNearestWindow(window_two->GetNativeWindow()).id());

  window_one->CloseNow();
  window_two->CloseNow();
}

// Tests that the window is maximized in response to a double click event.
TEST_F(DesktopScreenX11Test, DoubleClickHeaderMaximizes) {
  if (!ui::WmSupportsHint(gfx::GetAtom("_NET_WM_STATE_MAXIMIZED_VERT")))
    return;

  Widget* widget = BuildTopLevelDesktopWidget(gfx::Rect(0, 0, 100, 100), true);
  widget->Show();
  TestDesktopNativeWidgetAura* native_widget =
      static_cast<TestDesktopNativeWidgetAura*>(widget->native_widget());
  native_widget->set_window_component(HTCAPTION);

  aura::Window* window = widget->GetNativeWindow();
  window->SetProperty(aura::client::kResizeBehaviorKey,
                      ui::mojom::kResizeBehaviorCanMaximize);

  // Cast to superclass as DesktopWindowTreeHostX11 hide IsMaximized
  DesktopWindowTreeHost* rwh =
      DesktopWindowTreeHostX11::GetHostForXID(window->GetHost()->
          GetAcceleratedWidget());

  ui::test::EventGenerator generator(window);
  generator.DoubleClickLeftButton();
  RunPendingMessages();
  EXPECT_TRUE(rwh->IsMaximized());

  widget->CloseNow();
}

// Tests that the window does not maximize in response to a double click event,
// if the first click was to a different target component than that of the
// second click.
TEST_F(DesktopScreenX11Test, DoubleClickTwoDifferentTargetsDoesntMaximizes) {
  Widget* widget = BuildTopLevelDesktopWidget(gfx::Rect(0, 0, 100, 100), true);
  widget->Show();
  TestDesktopNativeWidgetAura* native_widget =
      static_cast<TestDesktopNativeWidgetAura*>(widget->native_widget());

  aura::Window* window = widget->GetNativeWindow();
  window->SetProperty(aura::client::kResizeBehaviorKey,
                      ui::mojom::kResizeBehaviorCanMaximize);

  // Cast to superclass as DesktopWindowTreeHostX11 hide IsMaximized
  DesktopWindowTreeHost* rwh =
      DesktopWindowTreeHostX11::GetHostForXID(window->GetHost()->
          GetAcceleratedWidget());

  ui::test::EventGenerator generator(window);
  native_widget->set_window_component(HTCLIENT);
  generator.ClickLeftButton();
  native_widget->set_window_component(HTCAPTION);
  generator.set_flags(ui::EF_IS_DOUBLE_CLICK);
  generator.ClickLeftButton();
  generator.set_flags(ui::EF_NONE);
  RunPendingMessages();
  EXPECT_FALSE(rwh->IsMaximized());

  widget->CloseNow();
}

// Tests that the window does not maximize in response to a double click event,
// if the double click was interrupted by a right click.
TEST_F(DesktopScreenX11Test, RightClickDuringDoubleClickDoesntMaximize) {
  Widget* widget = BuildTopLevelDesktopWidget(gfx::Rect(0, 0, 100, 100), true);
  widget->Show();
  TestDesktopNativeWidgetAura* native_widget =
      static_cast<TestDesktopNativeWidgetAura*>(widget->native_widget());

  aura::Window* window = widget->GetNativeWindow();
  window->SetProperty(aura::client::kResizeBehaviorKey,
                      ui::mojom::kResizeBehaviorCanMaximize);

  // Cast to superclass as DesktopWindowTreeHostX11 hide IsMaximized
  DesktopWindowTreeHost* rwh = static_cast<DesktopWindowTreeHost*>(
      DesktopWindowTreeHostX11::GetHostForXID(window->GetHost()->
          GetAcceleratedWidget()));

  ui::test::EventGenerator generator(window);
  native_widget->set_window_component(HTCLIENT);
  generator.ClickLeftButton();
  native_widget->set_window_component(HTCAPTION);
  generator.PressRightButton();
  generator.ReleaseRightButton();
  EXPECT_FALSE(rwh->IsMaximized());
  generator.set_flags(ui::EF_IS_DOUBLE_CLICK);
  generator.ClickLeftButton();
  generator.set_flags(ui::EF_NONE);
  RunPendingMessages();
  EXPECT_FALSE(rwh->IsMaximized());

  widget->CloseNow();
}

// Test that rotating the displays notifies the DisplayObservers.
TEST_F(DesktopScreenX11Test, RotationChange) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);
  ResetDisplayChanges();

  displays[0].set_rotation(display::Display::ROTATE_90);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(1u, changed_display_.size());

  displays[1].set_rotation(display::Display::ROTATE_90);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[0].set_rotation(display::Display::ROTATE_270);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(3u, changed_display_.size());

  displays[0].set_rotation(display::Display::ROTATE_270);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(3u, changed_display_.size());

  displays[0].set_rotation(display::Display::ROTATE_0);
  displays[1].set_rotation(display::Display::ROTATE_0);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(5u, changed_display_.size());
}

// Test that changing the displays workarea notifies the DisplayObservers.
TEST_F(DesktopScreenX11Test, WorkareaChange) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);
  ResetDisplayChanges();

  displays[0].set_work_area(gfx::Rect(0, 0, 300, 300));
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(1u, changed_display_.size());

  displays[1].set_work_area(gfx::Rect(0, 0, 300, 300));
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[0].set_work_area(gfx::Rect(0, 0, 300, 300));
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[1].set_work_area(gfx::Rect(0, 0, 300, 300));
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[0].set_work_area(gfx::Rect(0, 0, 640, 480));
  displays[1].set_work_area(gfx::Rect(640, 0, 1024, 768));
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(4u, changed_display_.size());
}

// Test that changing the device scale factor notifies the DisplayObservers.
TEST_F(DesktopScreenX11Test, DeviceScaleFactorChange) {
  std::vector<display::Display> displays;
  displays.push_back(
      display::Display(kFirstDisplay, gfx::Rect(0, 0, 640, 480)));
  displays.push_back(
      display::Display(kSecondDisplay, gfx::Rect(640, 0, 1024, 768)));
  NotifyDisplaysChanged(displays);
  ResetDisplayChanges();

  displays[0].set_device_scale_factor(2.5f);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(1u, changed_display_.size());
  EXPECT_EQ(2.5f, gfx::GetFontRenderParamsDeviceScaleFactor());

  displays[1].set_device_scale_factor(2.5f);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[0].set_device_scale_factor(2.5f);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[1].set_device_scale_factor(2.5f);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(2u, changed_display_.size());

  displays[0].set_device_scale_factor(1.f);
  displays[1].set_device_scale_factor(1.f);
  NotifyDisplaysChanged(displays);
  EXPECT_EQ(4u, changed_display_.size());
  EXPECT_EQ(1.f, gfx::GetFontRenderParamsDeviceScaleFactor());
}

}  // namespace views
