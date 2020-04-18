// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"

#include "base/callback.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util_enums.h"
#include "chrome/common/extensions/api/settings_private.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/extensions/api/settings_private/chromeos_resolve_time_zone_by_geolocation_method_short.h"
#include "chrome/browser/extensions/api/settings_private/chromeos_resolve_time_zone_by_geolocation_on_off.h"
#endif

namespace extensions {
namespace settings_private {

GeneratedPrefs::GeneratedPrefs(Profile* profile) {
#if defined(OS_CHROMEOS)
  prefs_[kResolveTimezoneByGeolocationOnOff] =
      CreateGeneratedResolveTimezoneByGeolocationOnOff(profile);
  prefs_[kResolveTimezoneByGeolocationMethodShort] =
      CreateGeneratedResolveTimezoneByGeolocationMethodShort(profile);
#endif
}

GeneratedPrefs::~GeneratedPrefs() = default;

bool GeneratedPrefs::HasPref(const std::string& pref_name) const {
  return FindPrefImpl(pref_name) != nullptr;
}

std::unique_ptr<api::settings_private::PrefObject> GeneratedPrefs::GetPref(
    const std::string& pref_name) const {
  GeneratedPref* impl = FindPrefImpl(pref_name);
  if (!impl)
    return nullptr;

  return impl->GetPrefObject();
}

SetPrefResult GeneratedPrefs::SetPref(const std::string& pref_name,
                                      const base::Value* value) {
  GeneratedPref* impl = FindPrefImpl(pref_name);
  if (!impl)
    return SetPrefResult::PREF_NOT_FOUND;

  return impl->SetPref(value);
}

void GeneratedPrefs::AddObserver(const std::string& pref_name,
                                 GeneratedPref::Observer* observer) {
  GeneratedPref* impl = FindPrefImpl(pref_name);
  CHECK(impl);

  impl->AddObserver(observer);
}

void GeneratedPrefs::RemoveObserver(const std::string& pref_name,
                                    GeneratedPref::Observer* observer) {
  GeneratedPref* impl = FindPrefImpl(pref_name);
  if (!impl)
    return;

  impl->RemoveObserver(observer);
}

GeneratedPref* GeneratedPrefs::FindPrefImpl(
    const std::string& pref_name) const {
  const PrefsMap::const_iterator it = prefs_.find(pref_name);
  if (it == prefs_.end())
    return nullptr;

  return it->second.get();
}

}  // namespace settings_private
}  // namespace extensions
