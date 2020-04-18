// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_
#define ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_

#include <string>

#include "ash/public/cpp/ash_public_export.h"

namespace base {
struct Feature;
}

namespace app_list {
namespace features {

// Please keep these features sorted.

// Enables the answer card in the app list.
ASH_PUBLIC_EXPORT extern const base::Feature kEnableAnswerCard;

// Enables app shortcuts search.
ASH_PUBLIC_EXPORT extern const base::Feature kEnableAppShortcutSearch;

// Enables background blur for the app list, lock screen, and tab switcher, also
// enables the AppsGridView mask layer. In this mode, slower devices may have
// choppier app list animations. crbug.com/765292.
ASH_PUBLIC_EXPORT extern const base::Feature kEnableBackgroundBlur;

// Enables the Play Store app search.
ASH_PUBLIC_EXPORT extern const base::Feature kEnablePlayStoreAppSearch;

// Enables the home launcher in tablet mode. In this mode, the launcher will be
// always shown right on top of the wallpaper. Home button will minimize all
// windows instead of toggling the launcher.
ASH_PUBLIC_EXPORT extern const base::Feature kEnableHomeLauncher;

// Enables the Settings shortcut search.
ASH_PUBLIC_EXPORT extern const base::Feature kEnableSettingsShortcutSearch;

bool ASH_PUBLIC_EXPORT IsAnswerCardEnabled();
bool ASH_PUBLIC_EXPORT IsAppShortcutSearchEnabled();
bool ASH_PUBLIC_EXPORT IsBackgroundBlurEnabled();
bool ASH_PUBLIC_EXPORT IsPlayStoreAppSearchEnabled();
bool ASH_PUBLIC_EXPORT IsHomeLauncherEnabled();
bool ASH_PUBLIC_EXPORT IsSettingsShortcutSearchEnabled();
std::string ASH_PUBLIC_EXPORT AnswerServerUrl();
std::string ASH_PUBLIC_EXPORT AnswerServerQuerySuffix();

}  // namespace features
}  // namespace app_list

#endif  // ASH_PUBLIC_CPP_APP_LIST_APP_LIST_FEATURES_H_
