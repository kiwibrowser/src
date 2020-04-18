// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/accelerator_commands.h"

#include "ash/accelerators/accelerator_commands.h"
#include "ash/shell.h"
#include "ash/wm/window_state.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/native_app_window.h"
#include "services/ui/public/interfaces/window_manager_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

using testing::Combine;
using testing::Values;
using testing::WithParamInterface;

namespace {

// WidgetDelegateView which allows the widget to be maximized.
class MaximizableWidgetDelegate : public views::WidgetDelegateView {
 public:
  MaximizableWidgetDelegate() {}
  ~MaximizableWidgetDelegate() override {}

  bool CanMaximize() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaximizableWidgetDelegate);
};

// Returns true if |window_state|'s window is in immersive fullscreen. Infer
// whether the window is in immersive fullscreen based on whether the shelf is
// hidden when the window is fullscreen. (This is not quite right because the
// shelf is hidden if a window is in both immersive fullscreen and tab
// fullscreen.)
bool IsInImmersiveFullscreen(ash::wm::WindowState* window_state) {
  return window_state->IsFullscreen() &&
         !window_state->GetHideShelfWhenFullscreen();
}

}  // namespace

typedef InProcessBrowserTest AcceleratorCommandsBrowserTest;

// Confirm that toggling window miximized works properly
IN_PROC_BROWSER_TEST_F(AcceleratorCommandsBrowserTest, ToggleMaximized) {
  ASSERT_TRUE(ash::Shell::HasInstance()) << "No Instance";
  ash::wm::WindowState* window_state = ash::wm::GetActiveWindowState();
  ASSERT_TRUE(window_state);

  // When not in fullscreen, accelerators::ToggleMaximized toggles Maximized.
  EXPECT_FALSE(window_state->IsMaximized());
  ash::accelerators::ToggleMaximized();
  EXPECT_TRUE(window_state->IsMaximized());
  ash::accelerators::ToggleMaximized();
  EXPECT_FALSE(window_state->IsMaximized());

  // When in fullscreen accelerators::ToggleMaximized gets out of fullscreen.
  EXPECT_FALSE(window_state->IsFullscreen());
  Browser* browser = chrome::FindBrowserWithWindow(window_state->window());
  ASSERT_TRUE(browser);
  chrome::ToggleFullscreenMode(browser);
  EXPECT_TRUE(window_state->IsFullscreen());
  ash::accelerators::ToggleMaximized();
  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_FALSE(window_state->IsMaximized());
  ash::accelerators::ToggleMaximized();
  EXPECT_FALSE(window_state->IsFullscreen());
  EXPECT_TRUE(window_state->IsMaximized());
}

class AcceleratorCommandsFullscreenBrowserTest
    : public WithParamInterface<ui::WindowShowState>,
      public InProcessBrowserTest {
 public:
  AcceleratorCommandsFullscreenBrowserTest()
      : initial_show_state_(GetParam()) {}
  virtual ~AcceleratorCommandsFullscreenBrowserTest() {}

  // Sets |window_state|'s show state to |initial_show_state_|.
  void SetToInitialShowState(ash::wm::WindowState* window_state) {
    if (initial_show_state_ == ui::SHOW_STATE_MAXIMIZED)
      window_state->Maximize();
    else
      window_state->Restore();
  }

  // Returns true if |window_state|'s show state is |initial_show_state_|.
  bool IsInitialShowState(const ash::wm::WindowState* window_state) const {
    if (initial_show_state_ == ui::SHOW_STATE_MAXIMIZED)
      return window_state->IsMaximized();
    else
      return window_state->IsNormalStateType();
  }

 private:
  ui::WindowShowState initial_show_state_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorCommandsFullscreenBrowserTest);
};

// Test that toggling window fullscreen works properly.
IN_PROC_BROWSER_TEST_P(AcceleratorCommandsFullscreenBrowserTest,
                       ToggleFullscreen) {
  ASSERT_TRUE(ash::Shell::HasInstance()) << "No Instance";

  // 1) Browser windows.
  ASSERT_TRUE(browser()->is_type_tabbed());
  ash::wm::WindowState* window_state =
      ash::wm::GetWindowState(browser()->window()->GetNativeWindow());
  ASSERT_TRUE(window_state->IsActive());
  SetToInitialShowState(window_state);
  EXPECT_TRUE(IsInitialShowState(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(IsInImmersiveFullscreen(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(IsInitialShowState(window_state));

  // 2) ToggleFullscreen() should have no effect on windows which cannot be
  // maximized.
  window_state->window()->SetProperty(aura::client::kResizeBehaviorKey,
                                      ui::mojom::kResizeBehaviorNone);
  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(IsInitialShowState(window_state));

  // 3) Hosted apps.
  Browser::CreateParams browser_create_params(
      Browser::CreateParams::CreateForApp("Test", true /* trusted_source */,
                                          gfx::Rect(), browser()->profile(),
                                          true));

  Browser* app_host_browser = new Browser(browser_create_params);
  ASSERT_TRUE(app_host_browser->is_app());
  AddBlankTabAndShow(app_host_browser);
  window_state =
      ash::wm::GetWindowState(app_host_browser->window()->GetNativeWindow());
  ASSERT_TRUE(window_state->IsActive());
  SetToInitialShowState(window_state);
  EXPECT_TRUE(IsInitialShowState(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(IsInImmersiveFullscreen(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(IsInitialShowState(window_state));

  // 4) Popup browser windows.
  browser_create_params =
      Browser::CreateParams(Browser::TYPE_POPUP, browser()->profile(), true);
  Browser* popup_browser = new Browser(browser_create_params);
  ASSERT_TRUE(popup_browser->is_type_popup());
  ASSERT_FALSE(popup_browser->is_app());
  AddBlankTabAndShow(popup_browser);
  window_state =
      ash::wm::GetWindowState(popup_browser->window()->GetNativeWindow());
  ASSERT_TRUE(window_state->IsActive());
  SetToInitialShowState(window_state);
  EXPECT_TRUE(IsInitialShowState(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(IsInImmersiveFullscreen(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(IsInitialShowState(window_state));

  // 5) Miscellaneous windows (e.g. task manager).
  views::Widget::InitParams params;
  params.delegate = new MaximizableWidgetDelegate();
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  std::unique_ptr<views::Widget> widget(new views::Widget);
  widget->Init(params);
  widget->Show();

  window_state = ash::wm::GetWindowState(widget->GetNativeWindow());
  ASSERT_TRUE(window_state->IsActive());
  SetToInitialShowState(window_state);
  EXPECT_TRUE(IsInitialShowState(window_state));

  ash::accelerators::ToggleFullscreen();
  EXPECT_TRUE(window_state->IsFullscreen());
  EXPECT_TRUE(IsInImmersiveFullscreen(window_state));

  // TODO(pkotwicz|oshima): Make toggling fullscreen restore the window to its
  // show state prior to entering fullscreen.
  ash::accelerators::ToggleFullscreen();
  EXPECT_FALSE(window_state->IsFullscreen());
}

INSTANTIATE_TEST_CASE_P(InitiallyRestored,
                        AcceleratorCommandsFullscreenBrowserTest,
                        Values(ui::SHOW_STATE_NORMAL));
INSTANTIATE_TEST_CASE_P(InitiallyMaximized,
                        AcceleratorCommandsFullscreenBrowserTest,
                        Values(ui::SHOW_STATE_MAXIMIZED));

class AcceleratorCommandsPlatformAppFullscreenBrowserTest
    : public WithParamInterface<ui::WindowShowState>,
      public extensions::PlatformAppBrowserTest {
 public:
  AcceleratorCommandsPlatformAppFullscreenBrowserTest()
      : initial_show_state_(GetParam()) {}
  virtual ~AcceleratorCommandsPlatformAppFullscreenBrowserTest() {}

  // Sets |app_window|'s show state to |initial_show_state_|.
  void SetToInitialShowState(extensions::AppWindow* app_window) {
    if (initial_show_state_ == ui::SHOW_STATE_MAXIMIZED)
      app_window->Maximize();
    else
      app_window->Restore();
  }

  // Returns true if |app_window|'s show state is |initial_show_state_|.
  bool IsInitialShowState(extensions::AppWindow* app_window) const {
    if (initial_show_state_ == ui::SHOW_STATE_MAXIMIZED)
      return app_window->GetBaseWindow()->IsMaximized();
    else
      return ui::BaseWindow::IsRestored(*app_window->GetBaseWindow());
  }

 private:
  ui::WindowShowState initial_show_state_;

  DISALLOW_COPY_AND_ASSIGN(AcceleratorCommandsPlatformAppFullscreenBrowserTest);
};

// Test the behavior of platform apps when ToggleFullscreen() is called.
IN_PROC_BROWSER_TEST_P(AcceleratorCommandsPlatformAppFullscreenBrowserTest,
                       ToggleFullscreen) {
  ASSERT_TRUE(ash::Shell::HasInstance()) << "No Instance";
  const extensions::Extension* extension =
      LoadAndLaunchPlatformApp("minimal", "Launched");

  {
    // Test that ToggleFullscreen() toggles a platform's app's fullscreen
    // state and that it additionally puts the app into immersive fullscreen
    // if put_all_windows_in_immersive() returns true.
    extensions::AppWindow::CreateParams params;
    params.frame = extensions::AppWindow::FRAME_CHROME;
    extensions::AppWindow* app_window =
        CreateAppWindowFromParams(browser()->profile(), extension, params);
    extensions::NativeAppWindow* native_app_window =
        app_window->GetBaseWindow();
    SetToInitialShowState(app_window);
    ASSERT_TRUE(app_window->GetBaseWindow()->IsActive());
    EXPECT_TRUE(IsInitialShowState(app_window));

    ash::accelerators::ToggleFullscreen();
    EXPECT_TRUE(native_app_window->IsFullscreen());
    ash::wm::WindowState* window_state =
        ash::wm::GetWindowState(native_app_window->GetNativeWindow());
    EXPECT_TRUE(IsInImmersiveFullscreen(window_state));

    ash::accelerators::ToggleFullscreen();
    EXPECT_TRUE(IsInitialShowState(app_window));

    CloseAppWindow(app_window);
  }

  {
    // Repeat the test, but make sure that frameless platform apps are never put
    // into immersive fullscreen.
    extensions::AppWindow::CreateParams params;
    params.frame = extensions::AppWindow::FRAME_NONE;
    extensions::AppWindow* app_window =
        CreateAppWindowFromParams(browser()->profile(), extension, params);
    extensions::NativeAppWindow* native_app_window =
        app_window->GetBaseWindow();
    ASSERT_TRUE(app_window->GetBaseWindow()->IsActive());
    SetToInitialShowState(app_window);
    EXPECT_TRUE(IsInitialShowState(app_window));

    ash::accelerators::ToggleFullscreen();
    EXPECT_TRUE(native_app_window->IsFullscreen());
    ash::wm::WindowState* window_state =
        ash::wm::GetWindowState(native_app_window->GetNativeWindow());
    EXPECT_FALSE(IsInImmersiveFullscreen(window_state));

    ash::accelerators::ToggleFullscreen();
    EXPECT_TRUE(IsInitialShowState(app_window));

    CloseAppWindow(app_window);
  }
}

INSTANTIATE_TEST_CASE_P(InitiallyRestored,
                        AcceleratorCommandsPlatformAppFullscreenBrowserTest,
                        Values(ui::SHOW_STATE_NORMAL));
INSTANTIATE_TEST_CASE_P(InitiallyMaximized,
                        AcceleratorCommandsPlatformAppFullscreenBrowserTest,
                        Values(ui::SHOW_STATE_MAXIMIZED));
