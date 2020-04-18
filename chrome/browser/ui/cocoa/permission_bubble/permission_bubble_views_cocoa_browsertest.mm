// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "chrome/browser/permissions/permission_request_manager_test_api.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands_mac.h"
#include "chrome/browser/ui/browser_window.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/bubble_anchor_helper.h"
#import "chrome/browser/ui/cocoa/fullscreen/fullscreen_toolbar_controller.h"
#include "chrome/browser/ui/exclusive_access/fullscreen_controller.h"
#include "chrome/browser/ui/permission_bubble/permission_bubble_browser_test_util.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/permission_bubble/permission_prompt_impl.h"
#import "testing/gtest_mac.h"
#include "ui/base/test/scoped_fake_nswindow_fullscreen.h"

// This file now tests the views permission bubble, on a Cocoa browser window.

namespace {

// Shows a test bubble using the permission request manager rather than any
// toolkit-specific bubble implementation.
void ShowBubble(Browser* browser) {
  auto test_api =
      std::make_unique<test::PermissionRequestManagerTestApi>(browser);
  EXPECT_TRUE(test_api->manager());

  test_api->AddSimpleRequest(CONTENT_SETTINGS_TYPE_GEOLOCATION);

  // The PermissionRequestManager displays prompts asynchronously.
  EXPECT_FALSE(test_api->GetPromptWindow());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(test_api->GetPromptWindow());
}

}  // namespace

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest, HasLocationBarByDefault) {
  ShowBubble(browser());
  HasVisibleLocationBarForBrowser(browser());
}

// Disabled. See https://crbug.com/845389 - this regressed somewhere between
// r545258 and r559030 (suspect: r549698), but it may be obsolete soon.
IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest,
                       DISABLED_BrowserFullscreenHasLocationBar) {
  ui::test::ScopedFakeNSWindowFullscreen faker;

  ShowBubble(browser());

  // Toggling the toolbar in fullscreen may trigger an animation, but it also
  // depends on the current mouse cursor position which is hard to manipulate in
  // a browser_test. So toggle the toolbar style between PRESENT and NONE using
  // the test API. Don't use HIDDEN, since that would cause flakes depending
  // where the mouse cursor is on the test machine.
  BrowserWindowController* browser_controller = [BrowserWindowController
      browserWindowControllerForWindow:browser()->window()->GetNativeWindow()];

  // Note that outside of fullscreen, there is no fullscreen toolbar controller.
  EXPECT_FALSE([browser_controller fullscreenToolbarController]);

  ExclusiveAccessManager* access_manager =
      browser()->exclusive_access_manager();
  FullscreenController* fullscreen_controller =
      access_manager->fullscreen_controller();

  EXPECT_FALSE(access_manager->context()->IsFullscreen());
  fullscreen_controller->ToggleBrowserFullscreenMode();
  faker.FinishTransition();
  EXPECT_TRUE(access_manager->context()->IsFullscreen());

  FullscreenToolbarController* toolbar_controller =
      [browser_controller fullscreenToolbarController];
  EXPECT_TRUE(toolbar_controller);

  // In fullscreen, try removing the toolbar.
  [toolbar_controller setToolbarStyle:FullscreenToolbarStyle::TOOLBAR_NONE];
  EXPECT_FALSE(HasVisibleLocationBarForBrowser(browser()));
  [toolbar_controller setToolbarStyle:FullscreenToolbarStyle::TOOLBAR_PRESENT];
  EXPECT_TRUE(HasVisibleLocationBarForBrowser(browser()));

  // Leave fullscreen. Toolbar should come back.
  fullscreen_controller->ToggleBrowserFullscreenMode();
  faker.FinishTransition();
  EXPECT_FALSE(access_manager->context()->IsFullscreen());
  EXPECT_FALSE([browser_controller fullscreenToolbarController]);
}

// Disabled. See https://crbug.com/845389 - this regressed somewhere between
// r545258 and r559030 (suspect: r549698), but it may be obsolete soon.
IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest,
                       DISABLED_TabFullscreenHasLocationBar) {
  ui::test::ScopedFakeNSWindowFullscreen faker;

  // TODO(tapted): This should use ShowBubble(). However, on 10.9 it triggers a
  // DCHECK failure in cr_setPatternPhase:forView:. See http://crbug.com/802107.
  auto prompt =
      std::make_unique<PermissionPromptImpl>(browser(), test_delegate());
  EXPECT_TRUE(HasVisibleLocationBarForBrowser(browser()));

  FullscreenController* controller =
      browser()->exclusive_access_manager()->fullscreen_controller();
  controller->EnterFullscreenModeForTab(
      browser()->tab_strip_model()->GetActiveWebContents(), GURL());
  faker.FinishTransition();

  EXPECT_FALSE(HasVisibleLocationBarForBrowser(browser()));
  controller->ExitFullscreenModeForTab(
      browser()->tab_strip_model()->GetActiveWebContents());
  faker.FinishTransition();

  EXPECT_TRUE(HasVisibleLocationBarForBrowser(browser()));
}

IN_PROC_BROWSER_TEST_F(PermissionBubbleBrowserTest, AppHasNoLocationBar) {
  Browser* app_browser = OpenExtensionAppWindow();
  // ShowBubble(app_browser) doesn't actually show a bubble for extension app
  // windows, so create one directly.
  auto prompt =
      std::make_unique<PermissionPromptImpl>(app_browser, test_delegate());
  EXPECT_FALSE(HasVisibleLocationBarForBrowser(app_browser));
}

// http://crbug.com/470724
// Kiosk mode on Mac has a location bar but it shouldn't.
IN_PROC_BROWSER_TEST_F(PermissionBubbleKioskBrowserTest,
                       DISABLED_KioskHasNoLocationBar) {
  ShowBubble(browser());
  EXPECT_FALSE(HasVisibleLocationBarForBrowser(browser()));
}
