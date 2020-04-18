// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_TYPES_H_
#define CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_TYPES_H_

#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "ui/gfx/image/image.h"

namespace chromeos {

enum class AppType {
  // Used for error scenarios and other cases where the app type isn't going to
  // be used (e.g. not launching an app).
  INVALID,

  // An Android app.
  ARC,

  // A Progressive Web App.
  PWA,
};

// Describes the possible ways for the intent picker to be closed.
enum class IntentPickerCloseReason {
  // There was an error in showing the intent picker.
  ERROR,

  // The user dismissed the picker without making a choice.
  DIALOG_DEACTIVATED,

  // A preferred app was found for launch.
  PREFERRED_APP_FOUND,

  // The user chose to stay in Chrome.
  STAY_IN_CHROME,

  // The user chose to open an app.
  OPEN_APP,
};

// Describes what's the preferred platform for this navigation, if any.
enum class PreferredPlatform {
  // Either there was an error or there is no preferred app at all.
  NONE,

  // The preferred app is Chrome.
  NATIVE_CHROME,

  // The preferred app is an ARC app.
  ARC,

  // TODO(crbug.com/826982) Not needed until app registry is in use.
  // The preferred app is a PWA app.
  PWA,
};

enum class AppsNavigationAction {
  // The current navigation should be cancelled.
  CANCEL,

  // The current navigation should resume.
  RESUME,
};

// Represents the data required to display an app in a picker to the user.
struct IntentPickerAppInfo {
  IntentPickerAppInfo(AppType type,
                      const gfx::Image& icon,
                      const std::string& launch_name,
                      const std::string& display_name);

  IntentPickerAppInfo(IntentPickerAppInfo&& other);

  IntentPickerAppInfo& operator=(IntentPickerAppInfo&& other);

  // The type of app that this object represents.
  AppType type;

  // The icon to be displayed for this app in the picker.
  gfx::Image icon;

  // The string used to launch this app. Represents an Android package name
  // when type is ARC.
  std::string launch_name;

  // The string shown to the user to identify this app in the intent picker.
  std::string display_name;

  DISALLOW_COPY_AND_ASSIGN(IntentPickerAppInfo);
};

// Callback to allow app-platform-specific code to asynchronously signal what
// action should be taken for the current navigation, and provide a list of apps
// which can handle the navigation.
using AppsNavigationCallback =
    base::OnceCallback<void(AppsNavigationAction action,
                            std::vector<IntentPickerAppInfo> apps)>;

// Callback to allow app-platform-specific code to asynchronously provide a list
// of apps which can handle the navigation.
using GetAppsCallback =
    base::OnceCallback<void(std::vector<IntentPickerAppInfo> apps)>;

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APPS_INTENT_HELPER_APPS_NAVIGATION_TYPES_H_
