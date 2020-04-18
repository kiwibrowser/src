// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_frame_ash.h"

#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_observer.h"

namespace {

// Waits for the given widget's bounds to change to the given rect.
class WidgetBoundsWatcher : public views::WidgetObserver {
 public:
  WidgetBoundsWatcher(views::Widget* widget, const gfx::Rect& target_bounds)
      : widget_(widget), target_bounds_(target_bounds) {}

  ~WidgetBoundsWatcher() override {}

  void Wait() {
    // Unconditionally wait for the widget bounds to update, even if the bounds
    // already match |target_bounds_|. Otherwise multiple values for
    // |target_bounds_| will pass the test (either the current or re-positioned
    // bounds would be accepted).
    widget_->AddObserver(this);
    run_loop_.Run();
  }

 private:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override {
    if (new_bounds == target_bounds_) {
      run_loop_.Quit();
      widget_->RemoveObserver(this);
    }
  }

  views::Widget* widget_;
  const gfx::Rect target_bounds_;
  base::RunLoop run_loop_;
  DISALLOW_COPY_AND_ASSIGN(WidgetBoundsWatcher);
};

class BrowserTestParam : public InProcessBrowserTest,
                         public testing::WithParamInterface<bool> {
 public:
  BrowserTestParam() {}
  ~BrowserTestParam() {}

  bool CreateV1App() { return GetParam(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(BrowserTestParam);
};

}  // namespace

// Test that in Chrome OS, for app browser (v1app) windows, the auto management
// logic is disabled while for tabbed browser windows, it is enabled.
IN_PROC_BROWSER_TEST_P(BrowserTestParam,
                       TabbedOrAppBrowserWindowAutoManagementTest) {
  // Default |browser()| is not used by this test.
  browser()->window()->Close();

  // Open a new browser window (app or tabbed depending on a parameter).
  bool is_test_app = CreateV1App();
  Browser::CreateParams params =
      is_test_app ? Browser::CreateParams::CreateForApp(
                        "test_browser_app", true /* trusted_source */,
                        gfx::Rect(), browser()->profile(), true)
                  : Browser::CreateParams(browser()->profile(), true);
  gfx::Rect original_bounds(gfx::Rect(150, 250, 400, 100));
  params.initial_show_state = ui::SHOW_STATE_NORMAL;
  params.initial_bounds = original_bounds;
  Browser* browser = new Browser(params);
  browser->window()->Show();

  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    WidgetBoundsWatcher watch(
        BrowserView::GetBrowserViewForBrowser(browser)->GetWidget(),
        original_bounds);
    watch.Wait();
  }

  // The bounds passed via |initial_bounds| should be respected regardless of
  // the window type.
  EXPECT_EQ(original_bounds, browser->window()->GetBounds());

  // Close the browser and re-create the browser window with the same app name.
  // Don't provide initial bounds. The bounds should have been saved, but for
  // tabbed windows, the position should be auto-managed.
  browser->window()->Close();
  params.initial_bounds = gfx::Rect();
  browser = new Browser(params);
  browser->window()->Show();

  // For tabbed browser window, it will be centered to work area by auto window
  // management logic; for app browser window, it will remain the given bounds.
  gfx::Rect expectation = original_bounds;
  if (!is_test_app) {
    expectation =
        display::Screen::GetScreen()
            ->GetDisplayNearestPoint(browser->window()->GetBounds().origin())
            .work_area();
    expectation.ClampToCenteredSize(original_bounds.size());
    expectation.set_y(original_bounds.y());
  }

  if (chromeos::GetAshConfig() == ash::Config::MASH) {
    WidgetBoundsWatcher watch(
        BrowserView::GetBrowserViewForBrowser(browser)->GetWidget(),
        expectation);
    watch.Wait();
  }

  EXPECT_EQ(expectation, browser->window()->GetBounds())
      << (is_test_app ? "for app window" : "for tabbed browser window");
}

INSTANTIATE_TEST_CASE_P(BrowserTestTabbedOrApp,
                        BrowserTestParam,
                        testing::Bool());
