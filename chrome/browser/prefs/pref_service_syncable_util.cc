// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/pref_service_syncable_util.h"

#include <vector>

#include "base/logging.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/sync_preferences/pref_service_syncable.h"

#if defined(OS_ANDROID)
#include "components/proxy_config/proxy_config_pref_names.h"
#endif

sync_preferences::PrefServiceSyncable* PrefServiceSyncableFromProfile(
    Profile* profile) {
  return static_cast<sync_preferences::PrefServiceSyncable*>(
      profile->GetPrefs());
}

sync_preferences::PrefServiceSyncable* PrefServiceSyncableIncognitoFromProfile(
    Profile* profile) {
  return static_cast<sync_preferences::PrefServiceSyncable*>(
      profile->GetOffTheRecordPrefs());
}

std::unique_ptr<sync_preferences::PrefServiceSyncable>
CreateIncognitoPrefServiceSyncable(
    sync_preferences::PrefServiceSyncable* pref_service,
    PrefStore* incognito_extension_pref_store,
    std::unique_ptr<PrefValueStore::Delegate> delegate) {
  // List of keys that cannot be changed in the user prefs file by the incognito
  // profile.  All preferences that store information about the browsing history
  // or behavior of the user should have this property.
  std::vector<const char*> overlay_pref_names;
  overlay_pref_names.push_back(prefs::kBrowserWindowPlacement);
  overlay_pref_names.push_back(prefs::kMediaRouterTabMirroringSources);
  overlay_pref_names.push_back(prefs::kSaveFileDefaultDirectory);
#if defined(OS_ANDROID)
  overlay_pref_names.push_back(proxy_config::prefs::kProxy);
#endif
  return pref_service->CreateIncognitoPrefService(
      incognito_extension_pref_store, overlay_pref_names, std::move(delegate));
}
