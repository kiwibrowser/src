// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/chrome_launcher_prefs.h"

#include <stddef.h>

#include <memory>
#include <set>
#include <utility>

#include "ash/public/cpp/ash_pref_names.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/prefs/pref_service_syncable_util.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "chrome/browser/ui/app_list/app_list_syncable_service_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/ash/launcher/launcher_controller_helper.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync/model/string_ordinal.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace {

std::unique_ptr<base::DictionaryValue> CreateAppDict(
    const std::string& app_id) {
  auto app_value = std::make_unique<base::DictionaryValue>();
  app_value->SetString(kPinnedAppsPrefAppIDPath, app_id);
  return app_value;
}

// App ID of default pinned apps.
const char* kDefaultPinnedApps[] = {
    extension_misc::kGmailAppId, extension_misc::kGoogleDocAppId,
    extension_misc::kYoutubeAppId, arc::kPlayStoreAppId};

std::unique_ptr<base::ListValue> CreateDefaultPinnedAppsList() {
  std::unique_ptr<base::ListValue> apps(new base::ListValue);
  for (size_t i = 0; i < arraysize(kDefaultPinnedApps); ++i)
    apps->Append(CreateAppDict(kDefaultPinnedApps[i]));

  return apps;
}

bool IsAppIdArcPackage(const std::string& app_id) {
  return app_id.find('.') != app_id.npos;
}

std::vector<std::string> GetActivitiesForPackage(
    const std::string& package,
    const std::vector<std::string>& all_arc_app_ids,
    const ArcAppListPrefs& app_list_pref) {
  std::vector<std::string> activities;
  for (const std::string& app_id : all_arc_app_ids) {
    const std::unique_ptr<ArcAppListPrefs::AppInfo> app_info =
        app_list_pref.GetApp(app_id);
    if (app_info->package_name == package) {
      activities.push_back(app_info->activity);
    }
  }
  return activities;
}

std::vector<ash::ShelfID> AppIdsToShelfIDs(
    const std::vector<std::string> app_ids) {
  std::vector<ash::ShelfID> shelf_ids(app_ids.size());
  for (size_t i = 0; i < app_ids.size(); ++i)
    shelf_ids[i] = ash::ShelfID(app_ids[i]);
  return shelf_ids;
}

struct PinInfo {
  PinInfo(const std::string& app_id, const syncer::StringOrdinal& item_ordinal)
      : app_id(app_id), item_ordinal(item_ordinal) {}

  std::string app_id;
  syncer::StringOrdinal item_ordinal;
};

struct ComparePinInfo {
  bool operator()(const PinInfo& pin1, const PinInfo& pin2) {
    return pin1.item_ordinal.LessThan(pin2.item_ordinal);
  }
};

// Helper class to keep apps in order of appearance and to provide fast way
// to check if app exists in the list.
class AppTracker {
 public:
  bool HasApp(const std::string& app_id) const {
    return app_set_.find(app_id) != app_set_.end();
  }

  void AddApp(const std::string& app_id) {
    if (HasApp(app_id))
      return;
    app_list_.push_back(app_id);
    app_set_.insert(app_id);
  }

  void MaybeAddApp(const std::string& app_id,
                   const LauncherControllerHelper* helper,
                   bool check_for_valid_app) {
    DCHECK_NE(kPinnedAppsPlaceholder, app_id);
    if (check_for_valid_app && !helper->IsValidIDForCurrentUser(app_id)) {
      return;
    }
    AddApp(app_id);
  }

  void MaybeAddAppFromPref(const base::DictionaryValue* app_pref,
                           const LauncherControllerHelper* helper,
                           bool check_for_valid_app) {
    std::string app_id;
    if (!app_pref->GetString(kPinnedAppsPrefAppIDPath, &app_id)) {
      LOG(ERROR) << "Cannot get app id from app pref entry.";
      return;
    }

    if (app_id == kPinnedAppsPlaceholder)
      return;

    bool pinned_by_policy = false;
    if (app_pref->GetBoolean(kPinnedAppsPrefPinnedByPolicy,
                             &pinned_by_policy) &&
        pinned_by_policy) {
      return;
    }

    MaybeAddApp(app_id, helper, check_for_valid_app);
  }

  const std::vector<std::string>& app_list() const { return app_list_; }

 private:
  std::vector<std::string> app_list_;
  std::set<std::string> app_set_;
};

}  // namespace

const char kPinnedAppsPrefAppIDPath[] = "id";
const char kPinnedAppsPrefPinnedByPolicy[] = "pinned_by_policy";
const char kPinnedAppsPlaceholder[] = "AppShelfIDPlaceholder--------";

void RegisterChromeLauncherUserPrefs(PrefRegistrySimple* registry) {
  // TODO: If we want to support multiple profiles this will likely need to be
  // pushed to local state and we'll need to track profile per item.
  registry->RegisterIntegerPref(
      prefs::kShelfChromeIconIndex, 0,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterListPref(prefs::kPinnedLauncherApps,
                             CreateDefaultPinnedAppsList(),
                             user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterListPref(prefs::kPolicyPinnedLauncherApps);
}

void InitLocalPref(PrefService* prefs, const char* local, const char* synced) {
  // Ash's prefs *should* have been propagated to Chrome by now, but maybe not.
  // This belongs in Ash, but it can't observe syncing changes: crbug.com/774657
  if (prefs->FindPreference(local) && prefs->FindPreference(synced) &&
      !prefs->FindPreference(local)->HasUserSetting()) {
    prefs->SetString(local, prefs->GetString(synced));
  }
}

// Helper that extracts app list from policy preferences.
void GetAppsPinnedByPolicy(const PrefService* prefs,
                           const LauncherControllerHelper* helper,
                           bool check_for_valid_app,
                           AppTracker* apps) {
  DCHECK(apps);
  DCHECK(apps->app_list().empty());

  const auto* policy_apps = prefs->GetList(prefs::kPolicyPinnedLauncherApps);
  if (!policy_apps)
    return;

  // Obtain here all ids of ARC apps because it takes linear time, and getting
  // them in the loop bellow would lead to quadratic complexity.
  const ArcAppListPrefs* const arc_app_list_pref = helper->GetArcAppListPrefs();
  const std::vector<std::string> all_arc_app_ids(
      arc_app_list_pref ? arc_app_list_pref->GetAppIds()
                        : std::vector<std::string>());

  std::string app_id;
  for (size_t i = 0; i < policy_apps->GetSize(); ++i) {
    const base::DictionaryValue* dictionary = nullptr;
    if (!policy_apps->GetDictionary(i, &dictionary) ||
        !dictionary->GetString(kPinnedAppsPrefAppIDPath, &app_id)) {
      LOG(ERROR) << "Cannot extract policy app info from prefs.";
      continue;
    }
    if (IsAppIdArcPackage(app_id)) {
      if (!arc_app_list_pref)
        continue;

      // We are dealing with package name, not with 32 characters ID.
      const std::string& arc_package = app_id;
      const std::vector<std::string> activities = GetActivitiesForPackage(
          arc_package, all_arc_app_ids, *arc_app_list_pref);
      for (const auto& activity : activities) {
        const std::string arc_app_id =
            ArcAppListPrefs::GetAppId(arc_package, activity);
        apps->MaybeAddApp(arc_app_id, helper, check_for_valid_app);
      }
    } else {
      apps->MaybeAddApp(app_id, helper, check_for_valid_app);
    }
  }
}

std::vector<std::string> GetPinnedAppsFromPrefsLegacy(
    const PrefService* prefs,
    const LauncherControllerHelper* helper,
    bool check_for_valid_app) {
  // Adding the app list item to the list of items requires that the ID is not
  // a valid and known ID for the extension system. The ID was constructed that
  // way - but just to make sure...
  DCHECK(!helper->IsValidIDForCurrentUser(kPinnedAppsPlaceholder));

  const auto* pinned_apps = prefs->GetList(prefs::kPinnedLauncherApps);

  // Get the sanitized preference value for the index of the Chrome app icon.
  const size_t chrome_icon_index = std::max<size_t>(
      0, std::min<size_t>(pinned_apps->GetSize(),
                          prefs->GetInteger(prefs::kShelfChromeIconIndex)));

  // Check if Chrome is in either of the the preferences lists.
  std::unique_ptr<base::Value> chrome_app(
      CreateAppDict(extension_misc::kChromeAppId));

  AppTracker apps;
  GetAppsPinnedByPolicy(prefs, helper, check_for_valid_app, &apps);

  std::string app_id;
  for (size_t i = 0; i < pinned_apps->GetSize(); ++i) {
    // We need to position the chrome icon relative to its place in the pinned
    // preference list - even if an item of that list isn't shown yet.
    if (i == chrome_icon_index)
      apps.AddApp(extension_misc::kChromeAppId);
    const base::DictionaryValue* app_pref = nullptr;
    if (!pinned_apps->GetDictionary(i, &app_pref)) {
      LOG(ERROR) << "There is no dictionary for app entry.";
      continue;
    }
    apps.MaybeAddAppFromPref(app_pref, helper, check_for_valid_app);
  }

  // If not added yet, the chrome item will be the last item in the list.
  apps.AddApp(extension_misc::kChromeAppId);
  return apps.app_list();
}

// Helper to create pin position that stays before any synced app, even if
// app is not currently visible on a device.
syncer::StringOrdinal GetFirstPinPosition(Profile* profile) {
  syncer::StringOrdinal position;
  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(profile);
  for (const auto& sync_peer : app_service->sync_items()) {
    if (!sync_peer.second->item_pin_ordinal.IsValid())
      continue;
    if (!position.IsValid() ||
        sync_peer.second->item_pin_ordinal.LessThan(position)) {
      position = sync_peer.second->item_pin_ordinal;
    }
  }

  return position.IsValid() ? position.CreateBefore()
                            : syncer::StringOrdinal::CreateInitialOrdinal();
}

// Helper to creates pin position that stays before any synced app, even if
// app is not currently visible on a device.
syncer::StringOrdinal GetLastPinPosition(Profile* profile) {
  syncer::StringOrdinal position;
  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(profile);
  for (const auto& sync_peer : app_service->sync_items()) {
    if (!sync_peer.second->item_pin_ordinal.IsValid())
      continue;
    if (!position.IsValid() ||
        sync_peer.second->item_pin_ordinal.GreaterThan(position)) {
      position = sync_peer.second->item_pin_ordinal;
    }
  }

  return position.IsValid() ? position.CreateAfter()
                            : syncer::StringOrdinal::CreateInitialOrdinal();
}

std::vector<std::string> ImportLegacyPinnedApps(
    const PrefService* prefs,
    LauncherControllerHelper* helper) {
  const std::vector<std::string> legacy_pins_all =
      GetPinnedAppsFromPrefsLegacy(prefs, helper, false);
  DCHECK(!legacy_pins_all.empty());

  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(helper->profile());

  std::vector<std::string> legacy_pins_valid;
  syncer::StringOrdinal last_position =
      syncer::StringOrdinal::CreateInitialOrdinal();
  // Convert to sync item record.
  for (const auto& app_id : legacy_pins_all) {
    DCHECK_NE(kPinnedAppsPlaceholder, app_id);
    app_service->SetPinPosition(app_id, last_position);
    last_position = last_position.CreateAfter();
    if (helper->IsValidIDForCurrentUser(app_id))
      legacy_pins_valid.push_back(app_id);
  }

  // Now process default apps.
  for (size_t i = 0; i < arraysize(kDefaultPinnedApps); ++i) {
    const std::string& app_id = kDefaultPinnedApps[i];
    // Check if it is already imported.
    if (app_service->GetPinPosition(app_id).IsValid())
      continue;
    // Check if it is present but not in legacy pin.
    if (helper->IsValidIDForCurrentUser(app_id))
      continue;
    app_service->SetPinPosition(app_id, last_position);
    last_position = last_position.CreateAfter();
  }

  return legacy_pins_valid;
}

std::vector<ash::ShelfID> GetPinnedAppsFromPrefs(
    const PrefService* prefs,
    LauncherControllerHelper* helper) {
  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(helper->profile());
  // Some unit tests may not have it or service may not be initialized.
  if (!app_service || !app_service->IsInitialized())
    return std::vector<ash::ShelfID>();

  std::vector<PinInfo> pin_infos;

  // Empty pins indicates that sync based pin model is used for the first
  // time. In normal workflow we have at least Chrome browser pin info.
  bool first_run = true;

  for (const auto& sync_peer : app_service->sync_items()) {
    if (!sync_peer.second->item_pin_ordinal.IsValid())
      continue;

    first_run = false;
    // Don't include apps that currently do not exist on device.
    if (sync_peer.first != extension_misc::kChromeAppId &&
        !helper->IsValidIDForCurrentUser(sync_peer.first)) {
      continue;
    }

    pin_infos.push_back(
        PinInfo(sync_peer.first, sync_peer.second->item_pin_ordinal));
  }

  if (first_run) {
    // Return default apps in case profile is not synced yet.
    sync_preferences::PrefServiceSyncable* const pref_service_syncable =
        PrefServiceSyncableFromProfile(helper->profile());
    if (!pref_service_syncable->IsSyncing()) {
      return AppIdsToShelfIDs(
          GetPinnedAppsFromPrefsLegacy(prefs, helper, true));
    }

    // We need to import legacy pins model and convert it to sync based
    // model.
    return AppIdsToShelfIDs(ImportLegacyPinnedApps(prefs, helper));
  }

  // Sort pins according their ordinals.
  std::sort(pin_infos.begin(), pin_infos.end(), ComparePinInfo());

  AppTracker policy_apps;
  GetAppsPinnedByPolicy(prefs, helper, true, &policy_apps);

  // Pinned by policy apps appear first, if they were not shown before.
  syncer::StringOrdinal front_position = GetFirstPinPosition(helper->profile());
  std::vector<std::string>::const_reverse_iterator it;
  for (it = policy_apps.app_list().rbegin();
       it != policy_apps.app_list().rend(); ++it) {
    const std::string& app_id = *it;
    if (app_id == kPinnedAppsPlaceholder)
      continue;

    // Check if we already processed current app.
    if (app_service->GetPinPosition(app_id).IsValid())
      continue;

    // Now move it to the front.
    pin_infos.insert(pin_infos.begin(), PinInfo(*it, front_position));
    app_service->SetPinPosition(app_id, front_position);
    front_position = front_position.CreateBefore();
  }

  // Now insert Chrome browser app if needed.
  if (!app_service->GetPinPosition(extension_misc::kChromeAppId).IsValid()) {
    pin_infos.insert(pin_infos.begin(),
                     PinInfo(extension_misc::kChromeAppId, front_position));
    app_service->SetPinPosition(extension_misc::kChromeAppId, front_position);
  }

  if (helper->IsValidIDForCurrentUser(arc::kPlayStoreAppId) &&
      !app_service->GetSyncItem(arc::kPlayStoreAppId)) {
    const syncer::StringOrdinal arc_host_position =
        GetLastPinPosition(helper->profile());
    pin_infos.insert(pin_infos.begin(),
                     PinInfo(arc::kPlayStoreAppId, arc_host_position));
    app_service->SetPinPosition(arc::kPlayStoreAppId, arc_host_position);
  }

  // Convert to ShelfID array.
  std::vector<std::string> pins(pin_infos.size());
  for (size_t i = 0; i < pin_infos.size(); ++i)
    pins[i] = pin_infos[i].app_id;

  return AppIdsToShelfIDs(pins);
}

void RemovePinPosition(Profile* profile, const ash::ShelfID& shelf_id) {
  DCHECK(profile);

  const std::string& app_id = shelf_id.app_id;
  if (!shelf_id.launch_id.empty()) {
    VLOG(2) << "Syncing remove pin for '" << app_id
            << "' with non-empty launch id '" << shelf_id.launch_id
            << "' is not supported.";
    return;
  }
  DCHECK(!app_id.empty());

  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(profile);
  app_service->SetPinPosition(app_id, syncer::StringOrdinal());
}

void SetPinPosition(Profile* profile,
                    const ash::ShelfID& shelf_id,
                    const ash::ShelfID& shelf_id_before,
                    const std::vector<ash::ShelfID>& shelf_ids_after) {
  DCHECK(profile);

  const std::string& app_id = shelf_id.app_id;
  if (!shelf_id.launch_id.empty()) {
    VLOG(2) << "Syncing set pin for '" << app_id
            << "' with non-empty launch id '" << shelf_id.launch_id
            << "' is not supported.";
    return;
  }

  const std::string& app_id_before = shelf_id_before.app_id;

  DCHECK(!app_id.empty());
  DCHECK_NE(app_id, app_id_before);

  app_list::AppListSyncableService* app_service =
      app_list::AppListSyncableServiceFactory::GetForProfile(profile);
  // Some unit tests may not have this service.
  if (!app_service)
    return;

  syncer::StringOrdinal position_before =
      app_id_before.empty() ? syncer::StringOrdinal()
                            : app_service->GetPinPosition(app_id_before);
  syncer::StringOrdinal position_after;
  for (const auto& shelf_id_after : shelf_ids_after) {
    const std::string& app_id_after = shelf_id_after.app_id;
    DCHECK_NE(app_id_after, app_id);
    DCHECK_NE(app_id_after, app_id_before);
    syncer::StringOrdinal position = app_service->GetPinPosition(app_id_after);
    DCHECK(position.IsValid());
    if (!position.IsValid()) {
      LOG(ERROR) << "Sync pin position was not found for " << app_id_after;
      continue;
    }
    if (!position_before.IsValid() || !position.Equals(position_before)) {
      position_after = position;
      break;
    }
  }

  syncer::StringOrdinal pin_position;
  if (position_before.IsValid() && position_after.IsValid())
    pin_position = position_before.CreateBetween(position_after);
  else if (position_before.IsValid())
    pin_position = position_before.CreateAfter();
  else if (position_after.IsValid())
    pin_position = position_after.CreateBefore();
  else
    pin_position = syncer::StringOrdinal::CreateInitialOrdinal();
  app_service->SetPinPosition(app_id, pin_position);
}
