// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/ui/app_list/app_list_controller_impl.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/test_utils.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/test/event_generator.h"
#include "ui/wm/core/window_util.h"

using AppListTest = InProcessBrowserTest;
using AppListControllerDelegateImplTest = extensions::PlatformAppBrowserTest;

// TODO(crbug.com/759779, crbug.com/819386): Add back
// |ClickingContextMenuDoesNotDismiss|.

// Test AppListControllerDelegateImpl::IsAppOpen for extension apps.
IN_PROC_BROWSER_TEST_F(AppListControllerDelegateImplTest, IsExtensionAppOpen) {
  AppListControllerDelegateImpl delegate;
  EXPECT_FALSE(delegate.IsAppOpen("fake_extension_app_id"));

  base::FilePath extension_path = test_data_dir_.AppendASCII("app");
  const extensions::Extension* extension_app = LoadExtension(extension_path);
  ASSERT_NE(nullptr, extension_app);
  EXPECT_FALSE(delegate.IsAppOpen(extension_app->id()));
  {
    content::WindowedNotificationObserver app_loaded_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());
    OpenApplication(AppLaunchParams(
        profile(), extension_app, extensions::LAUNCH_CONTAINER_WINDOW,
        WindowOpenDisposition::NEW_WINDOW, extensions::SOURCE_TEST));
    app_loaded_observer.Wait();
  }
  EXPECT_TRUE(delegate.IsAppOpen(extension_app->id()));
}

// Test AppListControllerDelegateImpl::IsAppOpen for platform apps.
IN_PROC_BROWSER_TEST_F(AppListControllerDelegateImplTest, IsPlatformAppOpen) {
  AppListControllerDelegateImpl delegate;
  EXPECT_FALSE(delegate.IsAppOpen("fake_platform_app_id"));

  const extensions::Extension* app = InstallPlatformApp("minimal");
  EXPECT_FALSE(delegate.IsAppOpen(app->id()));
  {
    content::WindowedNotificationObserver app_loaded_observer(
        content::NOTIFICATION_LOAD_COMPLETED_MAIN_FRAME,
        content::NotificationService::AllSources());
    LaunchPlatformApp(app);
    app_loaded_observer.Wait();
  }
  EXPECT_TRUE(delegate.IsAppOpen(app->id()));
}
