// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/launcher/chrome_launcher_controller_util.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/ash/chrome_launcher_prefs.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_util.h"
#include "chrome/browser/ui/ash/multi_user/multi_user_window_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"

const extensions::Extension* GetExtensionForAppID(const std::string& app_id,
                                                  Profile* profile) {
  return extensions::ExtensionRegistry::Get(profile)->GetExtensionById(
      app_id, extensions::ExtensionRegistry::EVERYTHING);
}

AppListControllerDelegate::Pinnable GetPinnableForAppID(
    const std::string& app_id,
    Profile* profile) {
  const base::ListValue* pref =
      profile->GetPrefs()->GetList(prefs::kPolicyPinnedLauncherApps);
  if (!pref)
    return AppListControllerDelegate::PIN_EDITABLE;
  // Pinned ARC apps policy defines the package name of the apps, that must
  // be pinned. All the launch activities of any package in policy are pinned.
  // In turn the input parameter to this function is app_id, which
  // is 32 chars hash. In case of ARC app this is a hash of
  // (package name + activity). This means that we must identify the package
  // from the hash, and check if this package is pinned by policy.
  const ArcAppListPrefs* const arc_prefs = ArcAppListPrefs::Get(profile);
  std::string arc_app_packege_name;
  if (arc_prefs) {
    std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
        arc_prefs->GetApp(app_id);
    if (app_info)
      arc_app_packege_name = app_info->package_name;
  }
  for (size_t index = 0; index < pref->GetSize(); ++index) {
    const base::DictionaryValue* app = nullptr;
    std::string app_id_or_package;
    if (pref->GetDictionary(index, &app) &&
        app->GetString(kPinnedAppsPrefAppIDPath, &app_id_or_package) &&
        (app_id == app_id_or_package ||
         arc_app_packege_name == app_id_or_package)) {
      return AppListControllerDelegate::PIN_FIXED;
    }
  }
  return AppListControllerDelegate::PIN_EDITABLE;
}
