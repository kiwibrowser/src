// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#include "chrome/browser/extensions/lazy_background_page_test_util.h"
#include "chrome/browser/notifications/desktop_notification_profile_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"

class NotificationIdleTest : public ExtensionApiTest {
 protected:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    extensions::ProcessManager::SetEventPageIdleTimeForTesting(1);
    extensions::ProcessManager::SetEventPageSuspendingTimeForTesting(1);
  }

  const extensions::Extension* LoadExtensionAndWait(
      const std::string& test_name) {
    LazyBackgroundObserver page_complete;
    base::FilePath extdir = test_data_dir_.AppendASCII(test_name);
    const extensions::Extension* extension = LoadExtension(extdir);
    if (extension)
      page_complete.Wait();
    return extension;
  }
};

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, NotificationsNoPermission) {
  ASSERT_TRUE(RunExtensionTest("notifications/has_not_permission")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, NotificationsHasPermission) {
  DesktopNotificationProfileUtil::GrantPermission(browser()->profile(),
      GURL("chrome-extension://peoadpeiejnhkmpaakpnompolbglelel"));

  ASSERT_TRUE(RunExtensionTest("notifications/has_permission_prefs"))
      << message_;
}

  // MessaceCenter-specific test.
#if defined(RUN_MESSAGE_CENTER_TESTS)
#define MAYBE_NotificationsAllowUnload NotificationsAllowUnload
#else
#define MAYBE_NotificationsAllowUnload DISABLED_NotificationsAllowUnload
#endif

IN_PROC_BROWSER_TEST_F(NotificationIdleTest, MAYBE_NotificationsAllowUnload) {
  const extensions::Extension* extension =
      LoadExtensionAndWait("notifications/api/unload");
  ASSERT_TRUE(extension) << message_;

  // Lazy Background Page has been shut down.
  extensions::ProcessManager* pm = extensions::ProcessManager::Get(profile());
  EXPECT_FALSE(pm->GetBackgroundHostForExtension(last_loaded_extension_id()));
}
