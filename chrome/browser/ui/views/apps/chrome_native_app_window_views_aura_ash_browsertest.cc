// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/immersive/immersive_fullscreen_controller.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/ui/ash/tablet_mode_client.h"
#include "chrome/browser/ui/views/apps/chrome_native_app_window_views_aura_ash.h"
#include "extensions/browser/app_window/app_window.h"
#include "ui/base/ui_base_types.h"
#include "ui/wm/core/window_util.h"

#include "chromeos/login/login_state.h"

class ChromeNativeAppWindowViewsAuraAshBrowserTest
    : public extensions::PlatformAppBrowserTest {
 public:
  ChromeNativeAppWindowViewsAuraAshBrowserTest() = default;
  ~ChromeNativeAppWindowViewsAuraAshBrowserTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNativeAppWindowViewsAuraAshBrowserTest);
};

// Verify that immersive mode is enabled or disabled as expected.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       ImmersiveWorkFlow) {
  extensions::AppWindow* app_window = CreateTestAppWindow("{}");
  auto* window = static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
      GetNativeAppWindowForAppWindow(app_window));
  ASSERT_TRUE(window != nullptr);
  ASSERT_TRUE(window->immersive_fullscreen_controller_.get() != nullptr);
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  constexpr int kFrameHeight = 32;

  views::ClientView* client_view =
      window->widget()->non_client_view()->client_view();
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that when fullscreen is toggled on, immersive mode is enabled and
  // that when fullscreen is toggled off, immersive mode is disabled.
  app_window->OSFullscreen();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  EXPECT_EQ(0, client_view->bounds().y());

  app_window->Restore();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that since the auto hide title bars in tablet mode feature turned
  // on, immersive mode is enabled once tablet mode is entered, and disabled
  // once tablet mode is exited.
  ash::TabletModeController* tablet_mode_controller =
      ash::Shell::Get()->tablet_mode_controller();
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  tablet_mode_controller->FlushForTesting();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  EXPECT_EQ(0, client_view->bounds().y());

  tablet_mode_controller->EnableTabletModeWindowManager(false);
  tablet_mode_controller->FlushForTesting();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  EXPECT_EQ(kFrameHeight, client_view->bounds().y());

  // Verify that the window was fullscreened before entering tablet mode, it
  // will remain fullscreened after exiting tablet mode.
  app_window->OSFullscreen();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  tablet_mode_controller->FlushForTesting();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  tablet_mode_controller->EnableTabletModeWindowManager(false);
  tablet_mode_controller->FlushForTesting();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  app_window->Restore();

  // Verify that minimized windows do not have immersive mode enabled.
  app_window->Minimize();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  tablet_mode_controller->FlushForTesting();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  window->Show();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  app_window->Minimize();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  tablet_mode_controller->EnableTabletModeWindowManager(false);
  tablet_mode_controller->FlushForTesting();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());

  // Verify that activation change should not change the immersive
  // state.
  window->Show();
  app_window->OSFullscreen();
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  ::wm::DeactivateWindow(window->GetNativeWindow());
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
  ::wm::ActivateWindow(window->GetNativeWindow());
  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());

  CloseAppWindow(app_window);
}

// Verifies that apps in immersive fullscreen will have a restore state of
// maximized.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       ImmersiveModeFullscreenRestoreType) {
  extensions::AppWindow* app_window = CreateTestAppWindow("{}");
  auto* window = static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
      GetNativeAppWindowForAppWindow(app_window));
  ASSERT_TRUE(window != nullptr);
  ASSERT_TRUE(window->immersive_fullscreen_controller_.get() != nullptr);

  app_window->OSFullscreen();
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window->GetRestoredState());
  ash::Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
      true);
  EXPECT_TRUE(window->IsFullscreen());
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window->GetRestoredState());
  ash::Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
      false);
  EXPECT_EQ(ui::SHOW_STATE_MAXIMIZED, window->GetRestoredState());

  CloseAppWindow(app_window);
}

// Verify that immersive mode stays disabled when entering tablet mode in
// forced fullscreen mode (e.g. when running in a kiosk session).
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       NoImmersiveModeWhenForcedFullscreen) {
  extensions::AppWindow* app_window = CreateTestAppWindow("{}");
  auto* window = static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
      GetNativeAppWindowForAppWindow(app_window));
  ASSERT_TRUE(window != nullptr);
  ASSERT_TRUE(window->immersive_fullscreen_controller_.get() != nullptr);

  app_window->ForcedFullscreen();

  ash::TabletModeController* tablet_mode_controller =
      ash::Shell::Get()->tablet_mode_controller();
  tablet_mode_controller->EnableTabletModeWindowManager(true);
  tablet_mode_controller->FlushForTesting();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
  tablet_mode_controller->EnableTabletModeWindowManager(false);
  tablet_mode_controller->FlushForTesting();
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());
}

// Make sure a normal window is not in immersive mode, and uses
// immersive in fullscreen.
IN_PROC_BROWSER_TEST_F(ChromeNativeAppWindowViewsAuraAshBrowserTest,
                       PublicSessionImmersiveMode) {
  chromeos::LoginState::Get()->SetLoggedInState(
      chromeos::LoginState::LOGGED_IN_ACTIVE,
      chromeos::LoginState::LOGGED_IN_USER_PUBLIC_ACCOUNT);

  extensions::AppWindow* app_window = CreateTestAppWindow("{}");
  auto* window = static_cast<ChromeNativeAppWindowViewsAuraAsh*>(
      GetNativeAppWindowForAppWindow(app_window));
  ASSERT_TRUE(window != nullptr);
  ASSERT_TRUE(window->immersive_fullscreen_controller_.get() != nullptr);
  EXPECT_FALSE(window->immersive_fullscreen_controller_->IsEnabled());

  app_window->SetFullscreen(extensions::AppWindow::FULLSCREEN_TYPE_HTML_API,
                            true);

  EXPECT_TRUE(window->immersive_fullscreen_controller_->IsEnabled());
}
