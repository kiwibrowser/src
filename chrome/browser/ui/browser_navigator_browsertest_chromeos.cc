// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_navigator_browsertest.h"

#include "base/command_line.h"
#include "chrome/browser/chromeos/login/chrome_restart_request.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/test_multi_user_window_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/settings_window_manager_chromeos.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chromeos/chromeos_switches.h"
#include "components/account_id/account_id.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"

namespace {

GURL GetGoogleURL() {
  return GURL("http://www.google.com/");
}

using BrowserNavigatorTestChromeOS = BrowserNavigatorTest;

// This test verifies that the settings page is opened in a new browser window.
IN_PROC_BROWSER_TEST_F(BrowserNavigatorTestChromeOS, NavigateToSettings) {
  GURL old_url = browser()->tab_strip_model()->GetActiveWebContents()->GetURL();
  {
    content::WindowedNotificationObserver observer(
        content::NOTIFICATION_LOAD_STOP,
        content::NotificationService::AllSources());
    chrome::ShowSettings(browser());
    observer.Wait();
  }
  // browser() tab contents should be unaffected.
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(old_url,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());

  // Settings page should be opened in a new window.
  Browser* settings_browser =
      chrome::SettingsWindowManager::GetInstance()->FindBrowserForProfile(
          browser()->profile());
  EXPECT_NE(browser(), settings_browser);
  EXPECT_EQ(
      GURL("chrome://settings"),
      settings_browser->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Subclass that tests navigation while in the Guest session.
class BrowserGuestSessionNavigatorTest: public BrowserNavigatorTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    base::CommandLine command_line_copy = *command_line;
    command_line_copy.AppendSwitchASCII(
        chromeos::switches::kLoginProfile, "user");
    command_line_copy.AppendSwitch(chromeos::switches::kGuestSession);
    chromeos::GetOffTheRecordCommandLine(GetGoogleURL(),
                                         true,
                                         command_line_copy,
                                         command_line);
  }
};

// This test verifies that the settings page is opened in the incognito window
// in Guest Session (as well as all other windows in Guest session).
IN_PROC_BROWSER_TEST_F(BrowserGuestSessionNavigatorTest,
                       Disposition_Settings_UseIncognitoWindow) {
  Browser* incognito_browser = CreateIncognitoBrowser();

  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());
  EXPECT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(1, incognito_browser->tab_strip_model()->count());

  // Navigate to the settings page.
  NavigateParams params(MakeNavigateParams(incognito_browser));
  params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  params.url = GURL("chrome://chrome/settings");
  params.window_action = NavigateParams::SHOW_WINDOW;
  params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  Navigate(&params);

  // Settings page should be opened in incognito window.
  EXPECT_NE(browser(), params.browser);
  EXPECT_EQ(incognito_browser, params.browser);
  EXPECT_EQ(2, incognito_browser->tab_strip_model()->count());
  EXPECT_EQ(GURL("chrome://chrome/settings"),
            incognito_browser->tab_strip_model()->GetActiveWebContents()->
                GetURL());
}

// Test that in multi user environments a newly created browser gets created
// on the same desktop as the browser is shown on.
IN_PROC_BROWSER_TEST_F(BrowserGuestSessionNavigatorTest,
                       Browser_Gets_Created_On_Visiting_Desktop) {
  // Test 1: Test that a browser created from a visiting browser will be on the
  // same visiting desktop.
  {
    const AccountId desktop_account_id(
        AccountId::FromUserEmail("desktop_user_id@fake.com"));
    TestMultiUserWindowManager* manager =
        new TestMultiUserWindowManager(browser(), desktop_account_id);

    EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

    // Navigate to the settings page.
    NavigateParams params(MakeNavigateParams(browser()));
    params.disposition = WindowOpenDisposition::NEW_POPUP;
    params.url = GURL("chrome://chrome/settings");
    params.window_action = NavigateParams::SHOW_WINDOW;
    params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
    params.browser = browser();
    Navigate(&params);

    EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

    aura::Window* created_window = manager->created_window();
    ASSERT_TRUE(created_window);
    EXPECT_TRUE(
        manager->IsWindowOnDesktopOfUser(created_window, desktop_account_id));
  }
  // Test 2: Test that a window which is not visiting does not cause an owner
  // assignment of a newly created browser.
  {
    const AccountId browser_owner =
        multi_user_util::GetAccountIdFromProfile(browser()->profile());
    TestMultiUserWindowManager* manager =
        new TestMultiUserWindowManager(browser(), browser_owner);

    // Navigate to the settings page.
    NavigateParams params(MakeNavigateParams(browser()));
    params.disposition = WindowOpenDisposition::NEW_POPUP;
    params.url = GURL("chrome://chrome/settings");
    params.window_action = NavigateParams::SHOW_WINDOW;
    params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
    params.browser = browser();
    Navigate(&params);

    EXPECT_EQ(3u, chrome::GetTotalBrowserCount());

    // The ShowWindowForUser should not have been called since the window is
    // already on the correct desktop.
    ASSERT_FALSE(manager->created_window());
  }
}

}  // namespace
