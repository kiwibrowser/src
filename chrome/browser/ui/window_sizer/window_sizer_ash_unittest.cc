// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/window_properties.h"
#include "ash/scoped_root_window_for_new_windows.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/ui/ash/ash_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/window_sizer/window_sizer_common_unittest.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/test_browser_window_aura.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/render_view_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/aura/test/test_windows.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/wm/public/activation_client.h"

class WindowSizerAshTest : public ash::AshTestBase {
 public:
  WindowSizerAshTest() = default;
  ~WindowSizerAshTest() override = default;

  // The window sizing code only works when the window hasn't yet been created.
  std::unique_ptr<Browser> CreateWindowlessBrowser(
      Browser::CreateParams params) {
    params.skip_window_init_for_testing = true;
    return std::make_unique<Browser>(params);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowSizerAshTest);
};

namespace {

// Copied from window_sizer_ash.cc constant of the same name.
constexpr int kForceMaximizeWidthLimit = 1366;

// Shorten identifiers to improve line wrapping.
using util = WindowSizerTestUtil;
const int kDesktopBorderSize = WindowSizer::kDesktopBorderSize;
const int kMaximumWindowWidth = WindowSizer::kMaximumWindowWidth;

std::unique_ptr<Browser> CreateTestBrowser(aura::Window* window,
                                           const gfx::Rect& bounds,
                                           Browser::CreateParams* params) {
  if (!bounds.IsEmpty())
    window->SetBounds(bounds);
  std::unique_ptr<Browser> browser =
      chrome::CreateBrowserWithAuraTestWindowForParams(base::WrapUnique(window),
                                                       params);
  if (!browser->is_type_popup()) {
    browser->window()->GetNativeWindow()->SetProperty(
        ash::kWindowPositionManagedTypeKey, true);
  }
  return browser;
}

}  // namespace

// Test that the window is sized appropriately for the first run experience
// where the default window bounds calculation is invoked.
TEST_F(WindowSizerAshTest, DefaultSizeCase) {
  { // 4:3 monitor case, 1024x768, no taskbar
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize,
                  1024 - kDesktopBorderSize * 2, 768 - kDesktopBorderSize),
        window_bounds);
  }

  { // 4:3 monitor case, 1024x768, taskbar on bottom
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, taskbar_bottom_work_area, gfx::Rect(),
                          gfx::Rect(), gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect(kDesktopBorderSize, kDesktopBorderSize,
                        1024 - kDesktopBorderSize * 2,
                        taskbar_bottom_work_area.height() - kDesktopBorderSize),
              window_bounds);
  }

  { // 4:3 monitor case, 1024x768, taskbar on right
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, taskbar_right_work_area, gfx::Rect(),
                          gfx::Rect(), gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize,
                  taskbar_right_work_area.width() - kDesktopBorderSize * 2,
                  768 - kDesktopBorderSize),
        window_bounds);
  }

  { // 4:3 monitor case, 1024x768, taskbar on left
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, taskbar_left_work_area, gfx::Rect(),
                          gfx::Rect(), gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect(taskbar_left_work_area.x() + kDesktopBorderSize,
                        kDesktopBorderSize,
                        taskbar_left_work_area.width() - kDesktopBorderSize * 2,
                        taskbar_left_work_area.height() - kDesktopBorderSize),
              window_bounds);
  }

  { // 4:3 monitor case, 1024x768, taskbar on top
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, taskbar_top_work_area, gfx::Rect(),
                          gfx::Rect(), gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect(kDesktopBorderSize,
                        taskbar_top_work_area.y() + kDesktopBorderSize,
                        1024 - kDesktopBorderSize * 2,
                        taskbar_top_work_area.height() - kDesktopBorderSize),
              window_bounds);
  }

  { // 4:3 monitor case, 1280x1024
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1280x1024, p1280x1024, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect((1280 - kMaximumWindowWidth) / 2, kDesktopBorderSize,
                        kMaximumWindowWidth, 1024 - kDesktopBorderSize),
              window_bounds);
  }

  { // 4:3 monitor case, 1600x1200
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect((1600 - kMaximumWindowWidth) / 2, kDesktopBorderSize,
                        kMaximumWindowWidth, 1200 - kDesktopBorderSize),
              window_bounds);
  }

  { // 16:10 monitor case, 1680x1050
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1680x1050, p1680x1050, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect((1680 - kMaximumWindowWidth) / 2, kDesktopBorderSize,
                        kMaximumWindowWidth, 1050 - kDesktopBorderSize),
              window_bounds);
  }

  { // 16:10 monitor case, 1920x1200
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1920x1200, p1920x1200, gfx::Rect(), gfx::Rect(),
                          gfx::Rect(), DEFAULT, NULL, gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ(gfx::Rect((1920 - kMaximumWindowWidth) / 2, kDesktopBorderSize,
                        kMaximumWindowWidth, 1200 - kDesktopBorderSize),
              window_bounds);
  }
}

// Test that the next opened window is positioned appropriately given the
// bounds of an existing window of the same type.
TEST_F(WindowSizerAshTest, LastWindowBoundsCase) {
  { // normal, in the middle of the screen somewhere.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, p1024x768, gfx::Rect(),
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize, 500, 400),
        gfx::Rect(), LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(kWindowTilePixels + kDesktopBorderSize,
                        kWindowTilePixels + kDesktopBorderSize, 500, 400)
                  .ToString(),
              window_bounds.ToString());
  }

  { // taskbar on top.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, taskbar_top_work_area, gfx::Rect(),
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize, 500, 400),
        gfx::Rect(), LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(kWindowTilePixels + kDesktopBorderSize,
                        std::max(kWindowTilePixels + kDesktopBorderSize,
                                 34 /* toolbar height */),
                        500, 400)
                  .ToString(),
              window_bounds.ToString());
  }

  { // Too small to satisify the minimum visibility condition.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, p1024x768, gfx::Rect(),
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize, 29, 29), gfx::Rect(),
        LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(kWindowTilePixels + kDesktopBorderSize,
                        kWindowTilePixels + kDesktopBorderSize, 30 /* not 29 */,
                        30 /* not 29 */)
                  .ToString(),
              window_bounds.ToString());
  }


  { // Normal.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, p1024x768, gfx::Rect(),
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize, 500, 400),
        gfx::Rect(), LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(kWindowTilePixels + kDesktopBorderSize,
                        kWindowTilePixels + kDesktopBorderSize, 500, 400)
                  .ToString(),
              window_bounds.ToString());
  }
}

TEST_F(WindowSizerAshTest, LastWindowOffscreenWithNonAggressiveRepositioning) {
  { // taskbar on left.
    gfx::Rect window_bounds;
    util::GetWindowBounds(
        p1024x768, taskbar_left_work_area, gfx::Rect(),
        gfx::Rect(kDesktopBorderSize, kDesktopBorderSize, 500, 400),
        gfx::Rect(), LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(kWindowTilePixels + kDesktopBorderSize,
                        kWindowTilePixels + kDesktopBorderSize, 500, 400)
                  .ToString(),
              window_bounds.ToString());
  }

  { // offset would put the new window offscreen at the bottom but the minimum
    // visibility condition is barely satisfied without relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(10, 728, 500, 400), gfx::Rect(),
                          LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(10 + kWindowTilePixels, 738, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // offset would put the new window offscreen at the bottom and the minimum
    // visibility condition is satisified by relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(10, 729, 500, 400), gfx::Rect(),
                          LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(10 + kWindowTilePixels,
                        738 /* not 739 */,
                        500,
                        400).ToString(),
              window_bounds.ToString());
  }

  { // offset would put the new window offscreen at the right but the minimum
    // visibility condition is barely satisfied without relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(984, 10, 500, 400), gfx::Rect(),
                          LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994, 10 + kWindowTilePixels, 500, 400).ToString(),
              window_bounds.ToString());
  }

  { // offset would put the new window offscreen at the right and the minimum
    // visibility condition is satisified by relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(985, 10, 500, 400), gfx::Rect(),
                          LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 995 */,
                        10 + kWindowTilePixels,
                        500,
                        400).ToString(),
              window_bounds.ToString());
  }

  { // offset would put the new window offscreen at the bottom right and the
    // minimum visibility condition is satisified by relocation.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1024x768, p1024x768, gfx::Rect(),
                          gfx::Rect(985, 729, 500, 400), gfx::Rect(),
                          LAST_ACTIVE, NULL, gfx::Rect(), &window_bounds);
    EXPECT_EQ(gfx::Rect(994 /* not 995 */,
                        738 /* not 739 */,
                        500,
                        400).ToString(),
              window_bounds.ToString());
  }
}

// Test the placement of newly created windows.
TEST_F(WindowSizerAshTest, PlaceNewWindows) {
  // Create a browser to pass into the util::GetWindowBounds function.
  std::unique_ptr<TestingProfile> profile(new TestingProfile());
  Browser::CreateParams native_params(profile.get(), true);
  auto browser = CreateWindowlessBrowser(native_params);

  // Creating a popup handler here to make sure it does not interfere with the
  // existing windows.
  Browser::CreateParams params2(profile.get(), true);
  std::unique_ptr<Browser> browser2 = (CreateTestBrowser(
      CreateTestWindowInShellWithId(0), gfx::Rect(16, 32, 640, 320), &params2));
  BrowserWindow* browser_window = browser2->window();

  // Creating a popup to make sure it does not interfere with the positioning.
  Browser::CreateParams params_popup(Browser::TYPE_POPUP, profile.get(), true);
  std::unique_ptr<Browser> browser_popup(
      CreateTestBrowser(CreateTestWindowInShellWithId(1),
                        gfx::Rect(16, 32, 128, 256), &params_popup));

  // Creating a panel to make sure it does not interfere with the positioning.
  Browser::CreateParams params_panel(Browser::TYPE_POPUP, profile.get(), true);
  std::unique_ptr<Browser> browser_panel(
      CreateTestBrowser(CreateTestWindowInShellWithId(2),
                        gfx::Rect(32, 48, 256, 512), &params_panel));

  browser_window->Show();

  // Make sure that popups do not get changed.
  {
    Browser::CreateParams params_popup(Browser::TYPE_POPUP, profile.get(),
                                       true);
    auto new_popup = CreateWindowlessBrowser(params_popup);
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, gfx::Rect(),
                          gfx::Rect(50, 100, 300, 150), bottom_s1600x1200,
                          PERSISTED, new_popup.get(), gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ("50,100 300x150", window_bounds.ToString());
  }

  browser_window->Hide();
  { // If a window is there but not shown the persisted default should be used.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, gfx::Rect(),
                          gfx::Rect(50, 100, 300, 150), bottom_s1600x1200,
                          PERSISTED, browser.get(), gfx::Rect(),
                          &window_bounds);
    EXPECT_EQ("50,100 300x150", window_bounds.ToString());
  }

  { // If a window is there but not shown the default should be returned.
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, gfx::Rect(), gfx::Rect(),
                          bottom_s1600x1200, DEFAULT, browser.get(),
                          gfx::Rect(), &window_bounds);
    // Note: We need to also take the defaults maximum width into account here
    // since that might get used if the resolution is too big.
    EXPECT_EQ(
        gfx::Rect(
            std::max(kDesktopBorderSize, (1600 - kMaximumWindowWidth) / 2),
            kDesktopBorderSize,
            std::min(kMaximumWindowWidth, 1600 - 2 * kDesktopBorderSize),
            1200 - kDesktopBorderSize)
            .ToString(),
        window_bounds.ToString());
  }
}

// Test the placement of newly created windows on an empty desktop.
// This test supplements "PlaceNewWindows" by testing the creation of a newly
// created browser window on an empty desktop.
TEST_F(WindowSizerAshTest, PlaceNewBrowserWindowOnEmptyDesktop) {
  // Create a browser to pass into the util::GetWindowBoundsAndShowState
  // function.
  std::unique_ptr<TestingProfile> profile(new TestingProfile());
  Browser::CreateParams native_params(profile.get(), true);
  auto browser = CreateWindowlessBrowser(native_params);

  // A common screen size for Chrome OS devices where this behavior is
  // desirable.
  const gfx::Rect p1366x768(0, 0, kForceMaximizeWidthLimit, 768);

  // If there is no previous state the window should get maximized if the
  // screen is less than or equal to our limit (1366 pixels width).
  gfx::Rect window_bounds;
  ui::WindowShowState out_show_state1 = ui::SHOW_STATE_DEFAULT;
  util::GetWindowBoundsAndShowState(
      p1366x768,               // The screen resolution.
      p1366x768,               // The monitor work area.
      gfx::Rect(),             // The second monitor.
      gfx::Rect(),             // The (persisted) bounds.
      p1366x768,               // The overall work area.
      ui::SHOW_STATE_NORMAL,   // The persisted show state.
      ui::SHOW_STATE_DEFAULT,  // The last show state.
      DEFAULT,                 // No persisted values.
      browser.get(),           // Use this browser.
      gfx::Rect(),             // Don't request valid bounds.
      0u,                      // Display index.
      &window_bounds, &out_show_state1);
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, out_show_state1);

  // If there is a stored coordinate however, that should be taken instead.
  ui::WindowShowState out_show_state2 = ui::SHOW_STATE_DEFAULT;
  util::GetWindowBoundsAndShowState(
      p1366x768,                     // The screen resolution.
      p1366x768,                     // The monitor work area.
      gfx::Rect(),                   // The second monitor.
      gfx::Rect(50, 100, 300, 150),  // The (persisted) bounds.
      p1366x768,                     // The overall work area.
      ui::SHOW_STATE_NORMAL,         // The persisted show state.
      ui::SHOW_STATE_DEFAULT,        // The last show state.
      PERSISTED,                     // Set the persisted values.
      browser.get(),                 // Use this browser.
      gfx::Rect(),                   // Don't request valid bounds.
      0u,                            // Display index.
      &window_bounds, &out_show_state2);
  EXPECT_EQ(ui::SHOW_STATE_NORMAL, out_show_state2);
  EXPECT_EQ("50,100 300x150", window_bounds.ToString());

  // A larger monitor should not trigger auto-maximize.
  ui::WindowShowState out_show_state3 = ui::SHOW_STATE_DEFAULT;
  util::GetWindowBoundsAndShowState(
      p1600x1200,              // The screen resolution.
      p1600x1200,              // The monitor work area.
      gfx::Rect(),             // The second monitor.
      gfx::Rect(),             // The (persisted) bounds.
      p1600x1200,              // The overall work area.
      ui::SHOW_STATE_NORMAL,   // The persisted show state.
      ui::SHOW_STATE_DEFAULT,  // The last show state.
      DEFAULT,                 // No persisted values.
      browser.get(),           // Use this browser.
      gfx::Rect(),             // Don't request valid bounds.
      0u,                      // Display index.
      &window_bounds, &out_show_state3);
  EXPECT_EQ(ui::SHOW_STATE_DEFAULT, out_show_state3);
}

// Test the placement of newly created windows on multiple dislays.
TEST_F(WindowSizerAshTest, PlaceNewWindowsOnMultipleDisplays) {
  UpdateDisplay("1600x1200,1600x1200");
  gfx::Rect primary_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  gfx::Rect secondary_bounds =
      display_manager()->GetSecondaryDisplay().bounds();

  std::unique_ptr<TestingProfile> profile(new TestingProfile());

  // Create browser windows that are used as reference.
  Browser::CreateParams params(profile.get(), true);
  std::unique_ptr<Browser> browser(CreateTestBrowser(
      CreateTestWindowInShellWithId(0), gfx::Rect(10, 10, 200, 200), &params));
  BrowserWindow* browser_window = browser->window();
  gfx::NativeWindow native_window = browser_window->GetNativeWindow();
  browser_window->Show();
  EXPECT_EQ(native_window->GetRootWindow(),
            ash::Shell::GetRootWindowForNewWindows());

  Browser::CreateParams another_params(profile.get(), true);
  std::unique_ptr<Browser> another_browser(
      CreateTestBrowser(CreateTestWindowInShellWithId(1),
                        gfx::Rect(400, 10, 300, 300), &another_params));
  BrowserWindow* another_browser_window = another_browser->window();
  gfx::NativeWindow another_native_window =
      another_browser_window->GetNativeWindow();
  another_browser_window->Show();

  // Creating a new window to verify the new placement.
  Browser::CreateParams new_params(profile.get(), true);
  auto new_browser = CreateWindowlessBrowser(new_params);

  // Make sure the primary root is active.
  ASSERT_EQ(ash::Shell::GetPrimaryRootWindow(),
            ash::Shell::GetRootWindowForNewWindows());

  // First new window should be in the primary.
  {
    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, secondary_bounds, gfx::Rect(),
                          secondary_bounds, PERSISTED, new_browser.get(),
                          gfx::Rect(), &window_bounds);
    // TODO(oshima): Use exact bounds when the window_sizer_ash is
    // moved to ash and changed to include the result from
    // RearrangeVisibleWindowOnShow.
    EXPECT_TRUE(primary_bounds.Contains(window_bounds));
  }

  // Move the window to the right side of the secondary display and create a new
  // window. It should be opened then on the secondary display.
  {
    display::Display second_display =
        display::Screen::GetScreen()->GetDisplayNearestPoint(
            gfx::Point(1600 + 100, 10));
    browser_window->GetNativeWindow()->SetBoundsInScreen(
        gfx::Rect(secondary_bounds.CenterPoint().x() - 100, 10, 200, 200),
        second_display);
    wm::GetActivationClient(native_window->GetRootWindow())
        ->ActivateWindow(native_window);
    EXPECT_NE(ash::Shell::GetPrimaryRootWindow(),
              ash::Shell::GetRootWindowForNewWindows());
    gfx::Rect window_bounds;
    ui::WindowShowState out_show_state = ui::SHOW_STATE_DEFAULT;
    util::GetWindowBoundsAndShowState(
        p1600x1200, p1600x1200, secondary_bounds, gfx::Rect(), secondary_bounds,
        ui::SHOW_STATE_DEFAULT, ui::SHOW_STATE_DEFAULT, PERSISTED,
        new_browser.get(), gfx::Rect(), 1u, &window_bounds, &out_show_state);
    // TODO(oshima): Use exact bounds when the window_sizer_ash is
    // moved to ash and changed to include the result from
    // RearrangeVisibleWindowOnShow.
    EXPECT_TRUE(secondary_bounds.Contains(window_bounds));
  }

  // Activate another window in the primary display and create a new window.
  // It should be created in the primary display.
  {
    wm::GetActivationClient(another_native_window->GetRootWindow())
        ->ActivateWindow(another_native_window);
    EXPECT_EQ(ash::Shell::GetPrimaryRootWindow(),
              ash::Shell::GetRootWindowForNewWindows());

    gfx::Rect window_bounds;
    util::GetWindowBounds(p1600x1200, p1600x1200, secondary_bounds, gfx::Rect(),
                          secondary_bounds, PERSISTED, new_browser.get(),
                          gfx::Rect(), &window_bounds);
    // TODO(oshima): Use exact bounds when the window_sizer_ash is
    // moved to ash and changed to include the result from
    // RearrangeVisibleWindowOnShow.
    EXPECT_TRUE(primary_bounds.Contains(window_bounds));
  }
}

// Test that the show state is properly returned for non default cases.
TEST_F(WindowSizerAshTest, TestShowState) {
  std::unique_ptr<TestingProfile> profile(new TestingProfile());

  // Creating a browser & window to play with.
  Browser::CreateParams params(Browser::TYPE_TABBED, profile.get(), true);
  auto browser = CreateWindowlessBrowser(params);

  // Create also a popup browser since that behaves different.
  Browser::CreateParams params_popup(Browser::TYPE_POPUP, profile.get(), true);
  auto browser_popup = CreateWindowlessBrowser(params_popup);

  // Tabbed windows should retrieve the saved window state - since there is a
  // top window.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            util::GetWindowShowState(ui::SHOW_STATE_MAXIMIZED,
                                     ui::SHOW_STATE_NORMAL, PERSISTED,
                                     browser.get(), p1600x1200, p1600x1200));
  // A window that is smaller than the whole work area is set to default state.
  EXPECT_EQ(ui::SHOW_STATE_DEFAULT,
            util::GetWindowShowState(ui::SHOW_STATE_DEFAULT,
                                     ui::SHOW_STATE_NORMAL, PERSISTED,
                                     browser.get(), p1280x1024, p1600x1200));
  // A window that is sized to occupy the whole work area is maximized.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            util::GetWindowShowState(ui::SHOW_STATE_DEFAULT,
                                     ui::SHOW_STATE_NORMAL, PERSISTED,
                                     browser.get(), p1600x1200, p1600x1200));
  // Non tabbed windows should always follow the window saved visibility state.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            util::GetWindowShowState(
                ui::SHOW_STATE_MAXIMIZED, ui::SHOW_STATE_NORMAL, BOTH,
                browser_popup.get(), p1600x1200, p1600x1200));
  // The non tabbed window will take the status of the last active of its kind.
  EXPECT_EQ(ui::SHOW_STATE_NORMAL,
            util::GetWindowShowState(
                ui::SHOW_STATE_DEFAULT, ui::SHOW_STATE_NORMAL, BOTH,
                browser_popup.get(), p1600x1200, p1600x1200));

  // A tabbed window should now take the top level window state.
  EXPECT_EQ(
      ui::SHOW_STATE_NORMAL,
      util::GetWindowShowState(ui::SHOW_STATE_MAXIMIZED, ui::SHOW_STATE_NORMAL,
                               BOTH, browser.get(), p1600x1200, p1600x1200));
  // Non tabbed windows should always follow the window saved visibility state.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
            util::GetWindowShowState(
                ui::SHOW_STATE_MAXIMIZED, ui::SHOW_STATE_MINIMIZED, BOTH,
                browser_popup.get(), p1600x1200, p1600x1200));

  // In smaller screen resolutions we default to maximized if there is no other
  // window visible.
  int min_size = kForceMaximizeWidthLimit / 2;
  if (min_size > 0) {
    const gfx::Rect tiny_screen(0, 0, min_size, min_size);
    EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED,
              util::GetWindowShowState(
                  ui::SHOW_STATE_MAXIMIZED, ui::SHOW_STATE_DEFAULT, BOTH,
                  browser.get(), tiny_screen, tiny_screen));
  }
}

// Test that the default show state override behavior is properly handled.
TEST_F(WindowSizerAshTest, TestShowStateDefaults) {
  // Creating a browser & window to play with.
  std::unique_ptr<TestingProfile> profile(new TestingProfile());

  Browser::CreateParams params(Browser::TYPE_TABBED, profile.get(), true);
  auto browser = CreateWindowlessBrowser(params);

  // Create also a popup browser since that behaves slightly different for
  // defaults.
  Browser::CreateParams params_popup(Browser::TYPE_POPUP, profile.get(), true);
  auto browser_popup = CreateWindowlessBrowser(params_popup);

  // Check that a browser creation state always get used if not given as
  // SHOW_STATE_DEFAULT.
  ui::WindowShowState window_show_state = util::GetWindowShowState(
      ui::SHOW_STATE_MAXIMIZED, ui::SHOW_STATE_MAXIMIZED, DEFAULT,
      browser.get(), p1600x1200, p1600x1200);
  EXPECT_EQ(window_show_state, ui::SHOW_STATE_DEFAULT);

  browser->set_initial_show_state(ui::SHOW_STATE_MINIMIZED);
  EXPECT_EQ(util::GetWindowShowState(ui::SHOW_STATE_MAXIMIZED,
                                     ui::SHOW_STATE_MAXIMIZED, BOTH,
                                     browser.get(), p1600x1200, p1600x1200),
            ui::SHOW_STATE_MINIMIZED);
  browser->set_initial_show_state(ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(util::GetWindowShowState(ui::SHOW_STATE_MAXIMIZED,
                                     ui::SHOW_STATE_MAXIMIZED, BOTH,
                                     browser.get(), p1600x1200, p1600x1200),
            ui::SHOW_STATE_NORMAL);
  browser->set_initial_show_state(ui::SHOW_STATE_MAXIMIZED);
  EXPECT_EQ(
      util::GetWindowShowState(ui::SHOW_STATE_NORMAL, ui::SHOW_STATE_NORMAL,
                               BOTH, browser.get(), p1600x1200, p1600x1200),
      ui::SHOW_STATE_MAXIMIZED);

  // Check that setting the maximized command line option is forcing the
  // maximized state.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kStartMaximized);

  browser->set_initial_show_state(ui::SHOW_STATE_NORMAL);
  EXPECT_EQ(
      util::GetWindowShowState(ui::SHOW_STATE_NORMAL, ui::SHOW_STATE_NORMAL,
                               BOTH, browser.get(), p1600x1200, p1600x1200),
      ui::SHOW_STATE_MAXIMIZED);

  // The popup should favor the initial show state over the command line.
  EXPECT_EQ(util::GetWindowShowState(
                ui::SHOW_STATE_NORMAL, ui::SHOW_STATE_NORMAL, BOTH,
                browser_popup.get(), p1600x1200, p1600x1200),
            ui::SHOW_STATE_NORMAL);
}

TEST_F(WindowSizerAshTest, DefaultStateBecomesMaximized) {
  // Create a browser to pass into the util::GetWindowBounds function.
  std::unique_ptr<TestingProfile> profile(new TestingProfile());
  Browser::CreateParams native_params(profile.get(), true);
  auto browser = CreateWindowlessBrowser(native_params);

  gfx::Rect display_bounds =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds();
  gfx::Rect specified_bounds = display_bounds;

  // Make a window bigger than the display work area.
  specified_bounds.Inset(-20, -20);
  ui::WindowShowState show_state = ui::SHOW_STATE_DEFAULT;
  gfx::Rect bounds;
  WindowSizer::GetBrowserWindowBoundsAndShowState(
      std::string(), specified_bounds, browser.get(), &bounds, &show_state);
  // The window should start maximized with its restore bounds shrunken.
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, show_state);
  EXPECT_NE(display_bounds.ToString(), bounds.ToString());
  EXPECT_TRUE(display_bounds.Contains(bounds));

  // Make a window smaller than the display work area.
  specified_bounds.Inset(100, 100);
  show_state = ui::SHOW_STATE_DEFAULT;
  WindowSizer::GetBrowserWindowBoundsAndShowState(
      std::string(), specified_bounds, browser.get(), &bounds, &show_state);
  // The window should start in default state.
  EXPECT_EQ(ui::SHOW_STATE_DEFAULT, show_state);
  EXPECT_EQ(specified_bounds.ToString(), bounds.ToString());
}

// Test that the target root window is used as the destination of
// the non browser window. This differ from PersistedBoundsCase
// in that this uses real ash shell implementations + StateProvider
// TargetDisplayProvider, rather than mocks.
TEST_F(WindowSizerAshTest, DefaultBoundsInTargetDisplay) {
  if (!ash_util::ShouldOpenAshOnStartup())
    return;
  UpdateDisplay("500x500,600x600");

  // By default windows are placed on the primary display.
  aura::Window* first_root = ash::Shell::GetAllRootWindows()[0];
  EXPECT_EQ(first_root, ash::Shell::GetRootWindowForNewWindows());
  gfx::Rect bounds;
  ui::WindowShowState show_state;
  WindowSizer::GetBrowserWindowBoundsAndShowState(std::string(), gfx::Rect(),
                                                  NULL, &bounds, &show_state);
  EXPECT_TRUE(first_root->GetBoundsInScreen().Contains(bounds));

  {
    // When the second display is active new windows are placed there.
    aura::Window* second_root = ash::Shell::GetAllRootWindows()[1];
    ash::ScopedRootWindowForNewWindows tmp(second_root);
    gfx::Rect bounds;
    ui::WindowShowState show_state;
    WindowSizer::GetBrowserWindowBoundsAndShowState(
        std::string(),
        gfx::Rect(),
        NULL,
        &bounds,
        &show_state);
    EXPECT_TRUE(second_root->GetBoundsInScreen().Contains(bounds));
  }
}

TEST_F(WindowSizerAshTest, TrustedPopupBehavior) {
  std::unique_ptr<TestingProfile> profile(new TestingProfile());
  Browser::CreateParams trusted_popup_create_params(Browser::TYPE_POPUP,
                                                    profile.get(), true);
  trusted_popup_create_params.trusted_source = true;

  auto trusted_popup = CreateWindowlessBrowser(trusted_popup_create_params);
  // Trusted popup windows should follow the saved show state and ignore the
  // last show state.
  EXPECT_EQ(ui::SHOW_STATE_DEFAULT,
            util::GetWindowShowState(
                ui::SHOW_STATE_DEFAULT, ui::SHOW_STATE_NORMAL, BOTH,
                trusted_popup.get(), p1280x1024, p1600x1200));
  // A popup that is sized to occupy the whole work area has default state.
  EXPECT_EQ(ui::SHOW_STATE_DEFAULT,
            util::GetWindowShowState(
                ui::SHOW_STATE_DEFAULT, ui::SHOW_STATE_NORMAL, BOTH,
                trusted_popup.get(), p1600x1200, p1600x1200));
}
