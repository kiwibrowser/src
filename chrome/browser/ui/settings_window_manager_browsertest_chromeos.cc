// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/settings_window_manager_chromeos.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/settings_window_manager_observer_chromeos.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "url/gurl.h"

namespace {

class SettingsWindowTestObserver
    : public chrome::SettingsWindowManagerObserver {
 public:
  SettingsWindowTestObserver() : browser_(NULL), new_settings_count_(0) {}
  ~SettingsWindowTestObserver() override {}

  void OnNewSettingsWindow(Browser* settings_browser) override {
    browser_ = settings_browser;
    ++new_settings_count_;
  }

  Browser* browser() { return browser_; }
  size_t new_settings_count() const { return new_settings_count_; }

 private:
  Browser* browser_;
  size_t new_settings_count_;

  DISALLOW_COPY_AND_ASSIGN(SettingsWindowTestObserver);
};

}  // namespace

class SettingsWindowManagerTest : public InProcessBrowserTest {
 public:
  SettingsWindowManagerTest()
      : settings_manager_(chrome::SettingsWindowManager::GetInstance()),
        test_profile_(NULL) {
    settings_manager_->AddObserver(&observer_);
  }
  ~SettingsWindowManagerTest() override {
    settings_manager_->RemoveObserver(&observer_);
  }

  Profile* CreateTestProfile() {
    CHECK(!test_profile_);

    ProfileManager* profile_manager = g_browser_process->profile_manager();
    base::RunLoop run_loop;
    profile_manager->CreateProfileAsync(
        profile_manager->GenerateNextProfileDirectoryPath(),
        base::Bind(&SettingsWindowManagerTest::ProfileInitialized,
                   base::Unretained(this), run_loop.QuitClosure()),
        base::string16(), std::string(), std::string());
    run_loop.Run();

    return test_profile_;
  }

  void ProfileInitialized(const base::Closure& closure,
                          Profile* profile,
                          Profile::CreateStatus status) {
    if (status == Profile::CREATE_STATUS_INITIALIZED) {
      test_profile_ = profile;
      closure.Run();
    }
  }

  void ShowSettingsForProfile(Profile* profile) {
    settings_manager_->ShowChromePageForProfile(
        profile, GURL(chrome::kChromeUISettingsURL));
  }

  void CloseNonDefaultBrowsers() {
    std::list<Browser*> browsers_to_close;
    for (auto* b : *BrowserList::GetInstance()) {
      if (b != browser())
        browsers_to_close.push_back(b);
    }
    for (std::list<Browser*>::iterator iter = browsers_to_close.begin();
         iter != browsers_to_close.end(); ++iter) {
      CloseBrowserSynchronously(*iter);
    }
  }

 protected:
  chrome::SettingsWindowManager* settings_manager_;
  SettingsWindowTestObserver observer_;
  base::ScopedTempDir temp_profile_dir_;
  Profile* test_profile_;  // Owned by g_browser_process->profile_manager()

  DISALLOW_COPY_AND_ASSIGN(SettingsWindowManagerTest);
};

IN_PROC_BROWSER_TEST_F(SettingsWindowManagerTest, OpenSettingsWindow) {
  // Open a settings window.
  ShowSettingsForProfile(browser()->profile());
  Browser* settings_browser =
      settings_manager_->FindBrowserForProfile(browser()->profile());
  ASSERT_TRUE(settings_browser);
  // Ensure the observer fired correctly.
  EXPECT_EQ(1u, observer_.new_settings_count());
  EXPECT_EQ(settings_browser, observer_.browser());

  // Open the settings again: no new window.
  ShowSettingsForProfile(browser()->profile());
  EXPECT_EQ(settings_browser,
            settings_manager_->FindBrowserForProfile(browser()->profile()));
  EXPECT_EQ(1u, observer_.new_settings_count());

  // Close the settings window.
  CloseBrowserSynchronously(settings_browser);
  EXPECT_FALSE(settings_manager_->FindBrowserForProfile(browser()->profile()));

  // Open a new settings window.
  ShowSettingsForProfile(browser()->profile());
  Browser* settings_browser2 =
      settings_manager_->FindBrowserForProfile(browser()->profile());
  ASSERT_TRUE(settings_browser2);
  EXPECT_EQ(2u, observer_.new_settings_count());

  CloseBrowserSynchronously(settings_browser2);
}

#if !defined(OS_CHROMEOS)
IN_PROC_BROWSER_TEST_F(SettingsWindowManagerTest, SettingsWindowMultiProfile) {
  Profile* test_profile = CreateTestProfile();
  ASSERT_TRUE(test_profile);

  // Open a settings window.
  ShowSettingsForProfile(browser()->profile());
  Browser* settings_browser =
      settings_manager_->FindBrowserForProfile(browser()->profile());
  ASSERT_TRUE(settings_browser);
  // Ensure the observer fired correctly.
  EXPECT_EQ(1u, observer_.new_settings_count());
  EXPECT_EQ(settings_browser, observer_.browser());

  // Open a settings window for a new profile.
  ShowSettingsForProfile(test_profile);
  Browser* settings_browser2 =
      settings_manager_->FindBrowserForProfile(test_profile);
  ASSERT_TRUE(settings_browser2);
  // Ensure the observer fired correctly.
  EXPECT_EQ(2u, observer_.new_settings_count());
  EXPECT_EQ(settings_browser2, observer_.browser());

  CloseBrowserSynchronously(settings_browser);
  CloseBrowserSynchronously(settings_browser2);
}
#endif

IN_PROC_BROWSER_TEST_F(SettingsWindowManagerTest, OpenChromePages) {
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // History should open in the existing browser window.
  chrome::ShowHistory(browser());
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // Settings should open a new browser window.
  chrome::ShowSettings(browser());
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  // About should reuse the existing Settings window.
  chrome::ShowAboutChrome(browser());
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());

  // Extensions should open in an existing browser window.
  CloseNonDefaultBrowsers();
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());
  std::string extension_to_highlight;  // none
  chrome::ShowExtensions(browser(), extension_to_highlight);
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // Downloads should open in an existing browser window.
  chrome::ShowDownloads(browser());
  EXPECT_EQ(1u, chrome::GetTotalBrowserCount());

  // About should open a new browser window.
  chrome::ShowAboutChrome(browser());
  EXPECT_EQ(2u, chrome::GetTotalBrowserCount());
}
