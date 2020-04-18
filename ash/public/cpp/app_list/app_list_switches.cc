// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/app_list/app_list_switches.h"

#include "base/command_line.h"

namespace app_list {
namespace switches {

// Specifies the chrome-extension:// URL for the contents of an additional page
// added to the app launcher.
const char kCustomLauncherPage[] = "custom-launcher-page";

// If set, the app list will not be dismissed when it loses focus. This is
// useful when testing the app list or a custom launcher page. It can still be
// dismissed via the other methods (like the Esc key).
const char kDisableAppListDismissOnBlur[] = "disable-app-list-dismiss-on-blur";

// If set, the app list will be enabled as if enabled from CWS.
const char kEnableAppList[] = "enable-app-list";

// Enable/disable drive search in chrome launcher.
const char kEnableDriveSearchInChromeLauncher[] =
    "enable-drive-search-in-app-launcher";
const char kDisableDriveSearchInChromeLauncher[] =
    "disable-drive-search-in-app-launcher";

// If set, the app list will forget it has been installed on startup. Note this
// doesn't prevent the app list from running, it just makes Chrome think the app
// list hasn't been enabled (as in kEnableAppList) yet.
const char kResetAppListInstallState[] = "reset-app-list-install-state";

bool ShouldNotDismissOnBlur() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      kDisableAppListDismissOnBlur);
}

bool IsDriveSearchInChromeLauncherEnabled() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kEnableDriveSearchInChromeLauncher))
    return true;

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kDisableDriveSearchInChromeLauncher))
    return false;

  return true;
}

}  // namespace switches
}  // namespace app_list
