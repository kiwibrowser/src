// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/browsertest_util.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "chrome/browser/extensions/bookmark_app_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/launch_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/extensions/app_launch_params.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/browser/web_applications/web_app.h"
#include "chrome/common/web_application_info.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/extensions/updater/local_extension_cache.h"
#include "chromeos/chromeos_paths.h"
#endif

namespace extensions {
namespace browsertest_util {

namespace {

ExtensionService* GetExtensionService(Profile* profile) {
  return ExtensionSystem::Get(profile)->extension_service();
}

}  // namespace

void CreateAndInitializeLocalCache() {
#if defined(OS_CHROMEOS)
  base::FilePath extension_cache_dir;
  CHECK(base::PathService::Get(chromeos::DIR_DEVICE_EXTENSION_LOCAL_CACHE,
                               &extension_cache_dir));
  base::FilePath cache_init_file = extension_cache_dir.Append(
      extensions::LocalExtensionCache::kCacheReadyFlagFileName);
  EXPECT_EQ(base::WriteFile(cache_init_file, "", 0), 0);
#endif
}

const Extension* InstallBookmarkApp(Profile* profile, WebApplicationInfo info) {
  size_t num_extensions =
      ExtensionRegistry::Get(profile)->enabled_extensions().size();

  content::WindowedNotificationObserver windowed_observer(
      NOTIFICATION_CRX_INSTALLER_DONE,
      content::NotificationService::AllSources());
  CreateOrUpdateBookmarkApp(GetExtensionService(profile), &info);
  windowed_observer.Wait();

  EXPECT_EQ(++num_extensions,
            ExtensionRegistry::Get(profile)->enabled_extensions().size());
  const Extension* app =
      content::Details<const Extension>(windowed_observer.details()).ptr();
  extensions::SetLaunchType(profile, app->id(),
                            info.open_as_window
                                ? extensions::LAUNCH_TYPE_WINDOW
                                : extensions::LAUNCH_TYPE_REGULAR);

  return app;
}

Browser* LaunchAppBrowser(Profile* profile, const Extension* extension_app) {
  EXPECT_TRUE(OpenApplication(
      AppLaunchParams(profile, extension_app, LAUNCH_CONTAINER_WINDOW,
                      WindowOpenDisposition::CURRENT_TAB, SOURCE_TEST)));

  Browser* browser = chrome::FindLastActive();
  bool is_correct_app_browser =
      browser && web_app::GetExtensionIdFromApplicationName(
                     browser->app_name()) == extension_app->id();
  EXPECT_TRUE(is_correct_app_browser);

  return is_correct_app_browser ? browser : nullptr;
}

}  // namespace browsertest_util
}  // namespace extensions
