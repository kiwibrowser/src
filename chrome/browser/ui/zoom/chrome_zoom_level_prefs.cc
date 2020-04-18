// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/zoom/chrome_zoom_level_prefs.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/zoom/zoom_event_manager.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/common/page_zoom.h"

#if defined(OS_LINUX)
#include <dlfcn.h>
#endif

namespace {

std::string GetHash(const base::FilePath& relative_path) {
  size_t int_key = BASE_HASH_NAMESPACE::hash<base::FilePath>()(relative_path);
  return base::NumberToString(int_key);
}

std::string GetPartitionKey(const base::FilePath& relative_path) {
  // Create a partition_key string with no '.'s in it.
  const base::FilePath::StringType& path = relative_path.value();
  // Prepend "x" to prevent an unlikely collision with an old
  // partition key (which contained only [0-9]).
  return "x" +
         base::HexEncode(
             path.c_str(),
             path.size() * sizeof(base::FilePath::StringType::value_type));
}

#if defined(OS_LINUX)
typedef size_t (*LibstdcppHashBytesType)(const void* ptr,
                                         size_t len,
                                         size_t seed);

LibstdcppHashBytesType GetLibstdcppHashBytesFunction() {
  static bool have_libstdcpp_hash_bytes_function = false;
  static LibstdcppHashBytesType libstdcpp_hash_bytes_function = nullptr;
  if (have_libstdcpp_hash_bytes_function)
    return libstdcpp_hash_bytes_function;

  void* libstdcpp = dlopen("libstdc++.so.6", RTLD_LAZY);
  if (libstdcpp) {
    libstdcpp_hash_bytes_function = reinterpret_cast<LibstdcppHashBytesType>(
        dlsym(libstdcpp, "_ZSt11_Hash_bytesPKvmm"));
  }
  have_libstdcpp_hash_bytes_function = true;
  return libstdcpp_hash_bytes_function;
}

// This function should only be called if
// GetLibstdcppHashBytesFunction() returns non-nullptr.
size_t LibstdcppHashString(const std::string& str) {
  // This constant was copied out of the libstdc++4.8 headers from the
  // Jessie sysroot, which was used before the switch to libc++.
  static constexpr size_t kHashStringSeed = 0xc70f6907UL;
  LibstdcppHashBytesType libstdcpp_hash_bytes_function =
      GetLibstdcppHashBytesFunction();
  DCHECK(libstdcpp_hash_bytes_function);
  return libstdcpp_hash_bytes_function(str.c_str(), str.length(),
                                       kHashStringSeed);
}
#endif

const char kZoomLevelPath[] = "zoom_level";
const char kLastModifiedPath[] = "last_modified";

// Extract a timestamp from |dictionary[kLastModifiedPath]|.
// Will return base::Time() if no timestamp exists.
base::Time GetTimeStamp(const base::DictionaryValue* dictionary) {
  std::string timestamp_str;
  dictionary->GetStringWithoutPathExpansion(kLastModifiedPath, &timestamp_str);
  int64_t timestamp = 0;
  base::StringToInt64(timestamp_str, &timestamp);
  base::Time last_modified = base::Time::FromInternalValue(timestamp);
  return last_modified;
}

}  // namespace

ChromeZoomLevelPrefs::ChromeZoomLevelPrefs(
    PrefService* pref_service,
    const base::FilePath& profile_path,
    const base::FilePath& partition_path,
    base::WeakPtr<zoom::ZoomEventManager> zoom_event_manager)
    : pref_service_(pref_service),
      zoom_event_manager_(zoom_event_manager),
      host_zoom_map_(nullptr) {
  DCHECK(pref_service_);

  DCHECK(!partition_path.empty());
  DCHECK((partition_path == profile_path) ||
         profile_path.IsParent(partition_path));
  base::FilePath partition_relative_path;
  profile_path.AppendRelativePath(partition_path, &partition_relative_path);
  MigrateOldZoomPreferences(partition_relative_path);
  partition_key_ = GetPartitionKey(partition_relative_path);
}

ChromeZoomLevelPrefs::~ChromeZoomLevelPrefs() {}

std::string ChromeZoomLevelPrefs::GetPartitionKeyForTesting(
    const base::FilePath& relative_path) {
  return GetPartitionKey(relative_path);
}

void ChromeZoomLevelPrefs::SetDefaultZoomLevelPref(double level) {
  if (content::ZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel()))
    return;

  DictionaryPrefUpdate update(pref_service_, prefs::kPartitionDefaultZoomLevel);
  update->SetDouble(partition_key_, level);
  // For unregistered paths, OnDefaultZoomLevelChanged won't be called, so
  // set this manually.
  host_zoom_map_->SetDefaultZoomLevel(level);
  default_zoom_changed_callbacks_.Notify();
  if (zoom_event_manager_)
    zoom_event_manager_->OnDefaultZoomLevelChanged();
}

double ChromeZoomLevelPrefs::GetDefaultZoomLevelPref() const {
  double default_zoom_level = 0.0;

  const base::DictionaryValue* default_zoom_level_dictionary =
      pref_service_->GetDictionary(prefs::kPartitionDefaultZoomLevel);
  // If no default has been previously set, the default returned is the
  // value used to initialize default_zoom_level in this function.
  default_zoom_level_dictionary->GetDouble(partition_key_, &default_zoom_level);
  return default_zoom_level;
}

std::unique_ptr<ChromeZoomLevelPrefs::DefaultZoomLevelSubscription>
ChromeZoomLevelPrefs::RegisterDefaultZoomLevelCallback(
    const base::Closure& callback) {
  return default_zoom_changed_callbacks_.Add(callback);
}

void ChromeZoomLevelPrefs::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  // If there's a manager to aggregate ZoomLevelChanged events, pass this event
  // along. Since we already hold a subscription to our associated HostZoomMap,
  // we don't need to create a separate subscription for this.
  if (zoom_event_manager_)
    zoom_event_manager_->OnZoomLevelChanged(change);

  if (change.mode != content::HostZoomMap::ZOOM_CHANGED_FOR_HOST)
    return;
  double level = change.zoom_level;
  DictionaryPrefUpdate update(pref_service_,
                              prefs::kPartitionPerHostZoomLevels);
  base::DictionaryValue* host_zoom_dictionaries = update.Get();
  DCHECK(host_zoom_dictionaries);

  bool modification_is_removal =
      content::ZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel());

  base::DictionaryValue* host_zoom_dictionary_weak = nullptr;
  if (!host_zoom_dictionaries->GetDictionary(partition_key_,
                                             &host_zoom_dictionary_weak)) {
    auto host_zoom_dictionary = std::make_unique<base::DictionaryValue>();
    host_zoom_dictionary_weak = host_zoom_dictionary.get();
    host_zoom_dictionaries->Set(partition_key_,
                                std::move(host_zoom_dictionary));
  }

  if (modification_is_removal) {
    host_zoom_dictionary_weak->RemoveWithoutPathExpansion(change.host, nullptr);
  } else {
    base::DictionaryValue dict;
    dict.SetDouble(kZoomLevelPath, level);
    dict.SetString(kLastModifiedPath,
                   base::Int64ToString(change.last_modified.ToInternalValue()));
    host_zoom_dictionary_weak->SetKey(change.host, std::move(dict));
  }
}

void ChromeZoomLevelPrefs::MigrateOldZoomPreferences(
    const base::FilePath& partition_relative_path) {
  MigrateOldZoomPreferencesForKeys(GetHash(partition_relative_path),
                                   GetPartitionKey(partition_relative_path));
#if defined(OS_LINUX)
  // On Linux, there was a bug for a brief period of time
  // (https://crbug.com/727149) where the libc++ hash was used as the
  // partition key.  This bug was in dev and beta for a few weeks, so
  // there may be users who have preferences for both the libstdc++
  // and the libc++ hashes.  Since the libc++ settings are newer, make
  // sure we migrate the libstdc++ settings (below) after the libc++
  // ones (above), so that the precedence is: new settings, libc++
  // settings, libstdc++ settings.
  if (GetLibstdcppHashBytesFunction()) {
    MigrateOldZoomPreferencesForKeys(base::NumberToString(LibstdcppHashString(
                                         partition_relative_path.value())),
                                     GetPartitionKey(partition_relative_path));
  }
#endif
}

void ChromeZoomLevelPrefs::MigrateOldZoomPreferencesForKeys(
    const std::string& old_key,
    const std::string& new_key) {
  const base::DictionaryValue* default_zoom_level_dictionary =
      pref_service_->GetDictionary(prefs::kPartitionDefaultZoomLevel);
  if (default_zoom_level_dictionary) {
    double old_default_zoom_level = 0;
    if (default_zoom_level_dictionary->GetDouble(old_key,
                                                 &old_default_zoom_level)) {
      // If there was an old zoom, but no new zoom, copy the old zoom
      // over.
      if (!default_zoom_level_dictionary->GetDouble(new_key, nullptr)) {
        DictionaryPrefUpdate update(pref_service_,
                                    prefs::kPartitionDefaultZoomLevel);
        update->SetDouble(new_key, old_default_zoom_level);
      }
      // Always clean up the old zoom setting.
      DictionaryPrefUpdate update(pref_service_,
                                  prefs::kPartitionDefaultZoomLevel);
      update->RemoveWithoutPathExpansion(old_key, nullptr);
    }
  }

  DictionaryPrefUpdate update(pref_service_,
                              prefs::kPartitionPerHostZoomLevels);
  base::DictionaryValue* host_zoom_dictionaries = update.Get();
  DCHECK(host_zoom_dictionaries);
  pref_service_->GetDictionary(prefs::kPartitionPerHostZoomLevels);
  const base::DictionaryValue* old_host_zoom_dictionary = nullptr;
  if (host_zoom_dictionaries->GetDictionary(old_key,
                                            &old_host_zoom_dictionary)) {
    base::DictionaryValue* new_host_zoom_dictionary = nullptr;
    if (!host_zoom_dictionaries->GetDictionary(new_key,
                                               &new_host_zoom_dictionary)) {
      auto host_zoom_dictionary = std::make_unique<base::DictionaryValue>();
      new_host_zoom_dictionary = host_zoom_dictionary.get();
      host_zoom_dictionaries->Set(new_key, std::move(host_zoom_dictionary));
    }
    DCHECK(new_host_zoom_dictionary);

    // For each host, if there was an old zoom, but no new zoom, copy
    // the old zoom setting over.
    for (base::DictionaryValue::Iterator it(*old_host_zoom_dictionary);
         !it.IsAtEnd(); it.Advance()) {
      const std::string& host = it.key();
      double zoom_level = 0.0f;
      if (it.value().GetAsDouble(&zoom_level) &&
          !new_host_zoom_dictionary->GetDouble(host, nullptr)) {
        new_host_zoom_dictionary->SetKey(host, base::Value(zoom_level));
      }
    }
    // Always clean up the old dictionary.
    DictionaryPrefUpdate update(pref_service_,
                                prefs::kPartitionPerHostZoomLevels);
    update->RemoveWithoutPathExpansion(old_key, nullptr);
  }
}

// TODO(wjmaclean): Remove the dictionary_path once the migration code is
// removed. crbug.com/420643
void ChromeZoomLevelPrefs::ExtractPerHostZoomLevels(
    const base::DictionaryValue* host_zoom_dictionary,
    bool sanitize_partition_host_zoom_levels) {
  std::vector<std::string> keys_to_remove;
  std::unique_ptr<base::DictionaryValue> host_zoom_dictionary_copy =
      host_zoom_dictionary->DeepCopyWithoutEmptyChildren();
  for (base::DictionaryValue::Iterator i(*host_zoom_dictionary_copy);
       !i.IsAtEnd();
       i.Advance()) {
    const std::string& host(i.key());
    double zoom_level = 0;
    base::Time last_modified;

    bool has_valid_zoom_level;
    if (i.value().is_dict()) {
      const base::DictionaryValue* dict;
      i.value().GetAsDictionary(&dict);
      has_valid_zoom_level = dict->GetDouble(kZoomLevelPath, &zoom_level);
      last_modified = GetTimeStamp(dict);
    } else {
      // Old zoom level that is stored directly as a double.
      has_valid_zoom_level = i.value().GetAsDouble(&zoom_level);
    }

    // Filter out A) the empty host, B) zoom levels equal to the default; and
    // remember them, so that we can later erase them from Prefs.
    // Values of type A and B could have been stored due to crbug.com/364399.
    // Values of type B could further have been stored before the default zoom
    // level was set to its current value. In either case, SetZoomLevelForHost
    // will ignore type B values, thus, to have consistency with HostZoomMap's
    // internal state, these values must also be removed from Prefs.
    if (host.empty() || !has_valid_zoom_level ||
        content::ZoomValuesEqual(zoom_level,
                                 host_zoom_map_->GetDefaultZoomLevel())) {
      keys_to_remove.push_back(host);
      continue;
    }

    host_zoom_map_->InitializeZoomLevelForHost(host, zoom_level, last_modified);
  }

  // We don't bother sanitizing non-partition dictionaries as they will be
  // discarded in the migration process. Note: since the structure of partition
  // per-host zoom level dictionaries is different from the legacy profile
  // per-host zoom level dictionaries, the following code will fail if run
  // on the legacy dictionaries.
  if (!sanitize_partition_host_zoom_levels)
    return;

  // Sanitize prefs to remove entries that match the default zoom level and/or
  // have an empty host.
  {
    DictionaryPrefUpdate update(pref_service_,
                                prefs::kPartitionPerHostZoomLevels);
    base::DictionaryValue* host_zoom_dictionaries = update.Get();
    base::DictionaryValue* host_zoom_dictionary = nullptr;
    host_zoom_dictionaries->GetDictionary(partition_key_,
                                          &host_zoom_dictionary);
    for (const std::string& s : keys_to_remove)
      host_zoom_dictionary->RemoveWithoutPathExpansion(s, nullptr);
  }
}

void ChromeZoomLevelPrefs::InitHostZoomMap(
    content::HostZoomMap* host_zoom_map) {
  // This init function must be called only once.
  DCHECK(!host_zoom_map_);
  DCHECK(host_zoom_map);
  host_zoom_map_ = host_zoom_map;

  // Initialize the default zoom level.
  host_zoom_map_->SetDefaultZoomLevel(GetDefaultZoomLevelPref());

  // Initialize the HostZoomMap with per-host zoom levels from the persisted
  // zoom-level preference values.
  const base::DictionaryValue* host_zoom_dictionaries =
      pref_service_->GetDictionary(prefs::kPartitionPerHostZoomLevels);
  const base::DictionaryValue* host_zoom_dictionary = nullptr;
  if (host_zoom_dictionaries->GetDictionary(partition_key_,
                                            &host_zoom_dictionary)) {
    // Since we're calling this before setting up zoom_subscription_ below we
    // don't need to worry that host_zoom_dictionary is indirectly affected
    // by calls to HostZoomMap::SetZoomLevelForHost().
    ExtractPerHostZoomLevels(host_zoom_dictionary,
                             true /* sanitize_partition_host_zoom_levels */);
  }
  zoom_subscription_ = host_zoom_map_->AddZoomLevelChangedCallback(base::Bind(
      &ChromeZoomLevelPrefs::OnZoomLevelChanged, base::Unretained(this)));
}
