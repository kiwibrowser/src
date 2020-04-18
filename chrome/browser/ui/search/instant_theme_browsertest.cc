// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/search/instant_test_base.h"
#include "chrome/browser/ui/search/instant_test_utils.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

class InstantThemeTest : public extensions::ExtensionBrowserTest,
                         public InstantTestBase {
 public:
  InstantThemeTest() {}

 protected:
  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(https_test_server().Start());
    GURL base_url = https_test_server().GetURL("/instant_extended.html");
    GURL ntp_url = https_test_server().GetURL("/instant_extended_ntp.html");
    InstantTestBase::Init(base_url, ntp_url, false);
  }

  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();

    content::URLDataSource::Add(profile(), new ThemeSource(profile()));
  }

  void InstallThemeAndVerify(const std::string& theme_dir,
                             const std::string& theme_name) {
    bool had_previous_theme =
        !!ThemeServiceFactory::GetThemeForProfile(profile());

    const base::FilePath theme_path = test_data_dir_.AppendASCII(theme_dir);
    // Themes install asynchronously so we must check the number of enabled
    // extensions after theme install completes.
    size_t num_before = extensions::ExtensionRegistry::Get(profile())
                            ->enabled_extensions()
                            .size();
    content::WindowedNotificationObserver theme_change_observer(
        chrome::NOTIFICATION_BROWSER_THEME_CHANGED,
        content::Source<ThemeService>(
            ThemeServiceFactory::GetForProfile(profile())));
    ASSERT_TRUE(InstallExtensionWithUIAutoConfirm(
        theme_path, 1, extensions::ExtensionBrowserTest::browser()));
    theme_change_observer.Wait();
    size_t num_after = extensions::ExtensionRegistry::Get(profile())
                           ->enabled_extensions()
                           .size();
    // If a theme was already installed, we're just swapping one for another, so
    // no change in extension count.
    int expected_change = had_previous_theme ? 0 : 1;
    EXPECT_EQ(num_before + expected_change, num_after);

    const extensions::Extension* new_theme =
        ThemeServiceFactory::GetThemeForProfile(profile());
    ASSERT_NE(nullptr, new_theme);
    ASSERT_EQ(new_theme->name(), theme_name);
  }

  // Loads a named image from |image_url| in the given |tab|. |loaded|
  // returns whether the image was able to load without error.
  // The method returns true if the JavaScript executed cleanly.
  bool LoadImage(content::WebContents* tab,
                 const GURL& image_url,
                 bool* loaded) {
    std::string js_chrome =
        "var img = document.createElement('img');"
        "img.onerror = function() { domAutomationController.send(false); };"
        "img.onload  = function() { domAutomationController.send(true); };"
        "img.src = '" +
        image_url.spec() + "';";
    return content::ExecuteScriptAndExtractBool(tab, js_chrome, loaded);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(InstantThemeTest);
};

IN_PROC_BROWSER_TEST_F(InstantThemeTest, ThemeBackgroundAccess) {
  ASSERT_NO_FATAL_FAILURE(InstallThemeAndVerify("theme", "camo theme"));
  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // The "Instant" New Tab should have access to chrome-search: scheme but not
  // chrome: scheme.
  const GURL chrome_url("chrome://theme/IDR_THEME_NTP_BACKGROUND");
  const GURL search_url("chrome-search://theme/IDR_THEME_NTP_BACKGROUND");
  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  bool loaded = false;
  ASSERT_TRUE(LoadImage(tab, chrome_url, &loaded));
  EXPECT_FALSE(loaded) << chrome_url;
  ASSERT_TRUE(LoadImage(tab, search_url, &loaded));
  EXPECT_TRUE(loaded) << search_url;
}

// Flaky on all bots. http://crbug.com/335297.
IN_PROC_BROWSER_TEST_F(InstantThemeTest,
                       DISABLED_NoThemeBackgroundChangeEventOnTabSwitch) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  // Install a theme.
  ASSERT_NO_FATAL_FAILURE(InstallThemeAndVerify("theme", "camo theme"));
  ASSERT_EQ(1, browser()->tab_strip_model()->count());

  // Open new tab.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  // Make sure the tab did not receive an onthemechange event for the
  // already-installed theme. (An event *is* sent, but that happens before the
  // page can register its handler.)
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());
  int on_theme_changed_calls = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "onThemeChangedCalls", &on_theme_changed_calls));
  EXPECT_EQ(0, on_theme_changed_calls);

  // Activate the previous tab.
  browser()->tab_strip_model()->ActivateTabAt(0, false);
  ASSERT_EQ(0, browser()->tab_strip_model()->active_index());

  // Switch back to new tab.
  browser()->tab_strip_model()->ActivateTabAt(1, false);
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());

  // Confirm that new tab got no onthemechange event while switching tabs.
  active_tab = browser()->tab_strip_model()->GetActiveWebContents();
  on_theme_changed_calls = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "onThemeChangedCalls", &on_theme_changed_calls));
  EXPECT_EQ(0, on_theme_changed_calls);
}

// Flaky on all bots. http://crbug.com/335297, http://crbug.com/265971.
IN_PROC_BROWSER_TEST_F(InstantThemeTest,
                       DISABLED_SendThemeBackgroundChangedEvent) {
  ASSERT_NO_FATAL_FAILURE(SetupInstant(browser()));

  // Install a theme.
  ASSERT_NO_FATAL_FAILURE(InstallThemeAndVerify("theme", "camo theme"));
  ASSERT_EQ(1, browser()->tab_strip_model()->count());

  // Open new tab.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_TAB |
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ASSERT_EQ(2, browser()->tab_strip_model()->count());

  // Make sure the tab did not receive an onthemechange event for the
  // already-installed theme. (An event *is* sent, but that happens before the
  // page can register its handler.)
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(1, browser()->tab_strip_model()->active_index());
  int on_theme_changed_calls = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "onThemeChangedCalls", &on_theme_changed_calls));
  EXPECT_EQ(0, on_theme_changed_calls);

  // Install a different theme.
  ASSERT_NO_FATAL_FAILURE(InstallThemeAndVerify("theme2", "snowflake theme"));

  // Confirm that the new tab got notified about the theme changed event.
  on_theme_changed_calls = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "onThemeChangedCalls", &on_theme_changed_calls));
  EXPECT_EQ(1, on_theme_changed_calls);
}
