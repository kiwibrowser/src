// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "ash/public/cpp/app_list/app_list_switches.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_client_impl.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/browser/ui/app_list/search/search_controller.h"
#include "chrome/browser/ui/app_list/test/chrome_app_list_test_support.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/chromeos_switches.h"
#include "components/user_manager/user_names.h"

// Browser Test for AppListController that runs on all platforms supporting
// app_list.
using AppListControllerBrowserTest = InProcessBrowserTest;

// Test the CreateNewWindow function of the controller delegate.
IN_PROC_BROWSER_TEST_F(AppListControllerBrowserTest, CreateNewWindow) {
  AppListClientImpl* client = AppListClientImpl::GetInstance();
  AppListControllerDelegate* controller(client->GetControllerDelegate());
  ASSERT_TRUE(controller);

  EXPECT_EQ(1U, chrome::GetBrowserCount(browser()->profile()));
  EXPECT_EQ(0U, chrome::GetBrowserCount(
                    browser()->profile()->GetOffTheRecordProfile()));

  controller->CreateNewWindow(browser()->profile(), false);
  EXPECT_EQ(2U, chrome::GetBrowserCount(browser()->profile()));

  controller->CreateNewWindow(browser()->profile(), true);
  EXPECT_EQ(1U, chrome::GetBrowserCount(
                    browser()->profile()->GetOffTheRecordProfile()));
}

// Browser Test for AppListController that observes search result changes.
using AppListControllerSearchResultsBrowserTest =
    extensions::ExtensionBrowserTest;

// Test showing search results, and uninstalling one of them while displayed.
IN_PROC_BROWSER_TEST_F(AppListControllerSearchResultsBrowserTest,
                       UninstallSearchResult) {
  base::FilePath test_extension_path;
  ASSERT_TRUE(
      base::PathService::Get(chrome::DIR_TEST_DATA, &test_extension_path));
  test_extension_path = test_extension_path.AppendASCII("extensions")
      .AppendASCII("platform_apps")
      .AppendASCII("minimal");

  AppListClientImpl* client = AppListClientImpl::GetInstance();
  ASSERT_TRUE(client);
  // Associate |client| with the current profile.
  client->UpdateProfile();

  AppListModelUpdater* model_updater = test::GetModelUpdater(client);
  ASSERT_TRUE(model_updater);
  app_list::SearchController* search_controller =
      client->GetSearchControllerForTest();
  ASSERT_TRUE(search_controller);

  // Install the extension.
  const extensions::Extension* extension = InstallExtension(
      test_extension_path, 1 /* expected_change: new install */);
  ASSERT_TRUE(extension);

  const std::string title = extension->name();

  // Show the app list first, otherwise we won't have a search box to update.
  client->ShowAppList();
  client->FlushMojoForTesting();

  // Currently the search box is empty, so we have no result.
  EXPECT_FALSE(search_controller->GetResultByTitleForTest(title));

  // Now a search finds the extension.
  model_updater->UpdateSearchBox(base::ASCIIToUTF16(title),
                                 true /* initiated_by_user */);

  // Ensure everything is done, from Chrome to Ash and backwards.
  client->FlushMojoForTesting();
  EXPECT_TRUE(search_controller->GetResultByTitleForTest(title));

  // Uninstall the extension.
  UninstallExtension(extension->id());

  // Ensure everything is done, from Chrome to Ash and backwards.
  client->FlushMojoForTesting();

  // We cannot find the extension any more.
  EXPECT_FALSE(search_controller->GetResultByTitleForTest(title));

  client->DismissAppList();
}

class AppListControllerGuestModeBrowserTest : public InProcessBrowserTest {
 public:
  AppListControllerGuestModeBrowserTest() {}

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppListControllerGuestModeBrowserTest);
};

void AppListControllerGuestModeBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  command_line->AppendSwitch(chromeos::switches::kGuestSession);
  command_line->AppendSwitchASCII(chromeos::switches::kLoginUser,
                                  user_manager::kGuestUserName);
  command_line->AppendSwitchASCII(chromeos::switches::kLoginProfile,
                                  TestingProfile::kTestUserProfileDir);
  command_line->AppendSwitch(switches::kIncognito);
}

// Test creating the initial app list in guest mode.
IN_PROC_BROWSER_TEST_F(AppListControllerGuestModeBrowserTest, Incognito) {
  AppListClientImpl* client = AppListClientImpl::GetInstance();
  EXPECT_TRUE(client->GetCurrentAppListProfile());

  client->ShowAppList();
  EXPECT_EQ(browser()->profile(), client->GetCurrentAppListProfile());
}
