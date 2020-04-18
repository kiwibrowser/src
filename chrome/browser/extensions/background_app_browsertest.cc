// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "chrome/browser/background/background_mode_manager.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/profiles/profile_manager.h"

class TestBackgroundModeManager : public BackgroundModeManager {
 public:
  TestBackgroundModeManager(const base::CommandLine& command_line,
                            ProfileAttributesStorage* profile_storage)
      : BackgroundModeManager(command_line, profile_storage),
        showed_background_app_installed_notification_for_test_(false) {}

  ~TestBackgroundModeManager() override {}

  void DisplayClientInstalledNotification(const base::string16& name) override {
    showed_background_app_installed_notification_for_test_ = true;
  }

  bool showed_background_app_installed_notification_for_test() {
    return showed_background_app_installed_notification_for_test_;
  }

  void set_showed_background_app_installed_notification_for_test(
      bool showed) {
    showed_background_app_installed_notification_for_test_ = showed;
  }

 private:
  // Tracks if we have shown a "Background App Installed" notification to the
  // user.  Used for unit tests only.
  bool showed_background_app_installed_notification_for_test_;

  FRIEND_TEST_ALL_PREFIXES(BackgroundAppBrowserTest,
                           ReloadBackgroundApp);

  DISALLOW_COPY_AND_ASSIGN(TestBackgroundModeManager);
};

using BackgroundAppBrowserTest = extensions::ExtensionBrowserTest;

// Tests that if we reload a background app, we don't get a popup bubble
// telling us that a new background app has been installed.
IN_PROC_BROWSER_TEST_F(BackgroundAppBrowserTest, ReloadBackgroundApp) {
  // Pass this in to the browser test.
  std::unique_ptr<BackgroundModeManager> test_background_mode_manager(
      new TestBackgroundModeManager(*base::CommandLine::ForCurrentProcess(),
                                    &(g_browser_process->profile_manager()
                                          ->GetProfileAttributesStorage())));
  g_browser_process->set_background_mode_manager_for_test(
      std::move(test_background_mode_manager));
  TestBackgroundModeManager* manager =
      reinterpret_cast<TestBackgroundModeManager*>(
          g_browser_process->background_mode_manager());

  // Load our background extension
  ASSERT_FALSE(
      manager->showed_background_app_installed_notification_for_test());
  const extensions::Extension* extension = LoadExtension(
      test_data_dir_.AppendASCII("background_app"));
  ASSERT_FALSE(extension == NULL);

  // Set the test flag to not shown.
  manager->set_showed_background_app_installed_notification_for_test(false);

  // Reload our background extension
  ReloadExtension(extension->id());

  // Ensure that we did not see a "Background extension loaded" dialog.
  EXPECT_FALSE(
      manager->showed_background_app_installed_notification_for_test());
}
