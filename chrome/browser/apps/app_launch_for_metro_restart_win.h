// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_APPS_APP_LAUNCH_FOR_METRO_RESTART_WIN_H_
#define CHROME_BROWSER_APPS_APP_LAUNCH_FOR_METRO_RESTART_WIN_H_

#include <string>


class PrefRegistrySimple;
class Profile;

namespace app_metro_launch {

// Handles launching apps on browser startup due to an attempt to launch an app
// in Windows 8 Metro mode.
void HandleAppLaunchForMetroRestart(Profile* profile);

// Set a local pref to launch an app before relaunching chrome in desktop mode.
void SetAppLaunchForMetroRestart(Profile* profile,
                                 const std::string& extension_id);

// Register preferences to do with launching apps in Metro.
void RegisterPrefs(PrefRegistrySimple* registry);

}  // namespace app_metro_launch

#endif  // CHROME_BROWSER_APPS_APP_LAUNCH_FOR_METRO_RESTART_WIN_H_
