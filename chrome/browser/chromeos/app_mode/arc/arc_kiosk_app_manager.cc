// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <chrome/browser/chromeos/app_mode/arc/arc_kiosk_app_manager.h>

#include <algorithm>
#include <utility>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chromeos/cryptohome/async_method_caller.h"
#include "chromeos/cryptohome/cryptohome_parameters.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/arc/arc_util.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_manager/user.h"
#include "components/user_manager/user_manager.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

namespace {

// Preference for the dictionary of user ids for which cryptohomes have to be
// removed upon browser restart.
constexpr char kArcKioskUsersToRemove[] = "arc-kiosk-users-to-remove";

void ScheduleDelayedCryptohomeRemoval(const cryptohome::Identification& id) {
  PrefService* const local_state = g_browser_process->local_state();
  ListPrefUpdate list_update(local_state, kArcKioskUsersToRemove);

  list_update->AppendString(id.id());
  local_state->CommitPendingWrite();
}

void CancelDelayedCryptohomeRemoval(const cryptohome::Identification& id) {
  PrefService* const local_state = g_browser_process->local_state();
  ListPrefUpdate list_update(local_state, kArcKioskUsersToRemove);
  list_update->Remove(base::Value(id.id()), nullptr);
  local_state->CommitPendingWrite();
}

void OnRemoveAppCryptohomeComplete(const cryptohome::Identification& id,
                                   const base::Closure& callback,
                                   bool success,
                                   cryptohome::MountError return_code) {
  if (success) {
    CancelDelayedCryptohomeRemoval(id);
  } else {
    ScheduleDelayedCryptohomeRemoval(id);
    LOG(ERROR) << "Remove one of the cryptohomes failed, return code: "
               << return_code;
  }
  if (!callback.is_null())
    callback.Run();
}

void PerformDelayedCryptohomeRemovals(bool service_is_available) {
  if (!service_is_available) {
    LOG(ERROR) << "Crypthomed is not available.";
    return;
  }

  PrefService* const local_state = g_browser_process->local_state();
  const base::ListValue* const list =
      local_state->GetList(kArcKioskUsersToRemove);
  for (base::ListValue::const_iterator it = list->begin(); it != list->end();
       ++it) {
    std::string entry;
    if (!it->GetAsString(&entry)) {
      LOG(ERROR) << "List of cryptohome ids is broken";
      continue;
    }
    const cryptohome::Identification cryptohome_id(
        cryptohome::Identification::FromString(entry));
    cryptohome::AsyncMethodCaller::GetInstance()->AsyncRemove(
        cryptohome_id, base::Bind(&OnRemoveAppCryptohomeComplete, cryptohome_id,
                                  base::Closure()));
  }
}

// This class is owned by ChromeBrowserMainPartsChromeos.
static ArcKioskAppManager* g_arc_kiosk_app_manager = nullptr;

}  // namespace

// static
const char ArcKioskAppManager::kArcKioskDictionaryName[] = "arc-kiosk";

// static
void ArcKioskAppManager::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kArcKioskDictionaryName);
  registry->RegisterListPref(kArcKioskUsersToRemove);
}

// static
void ArcKioskAppManager::RemoveObsoleteCryptohomes() {
  chromeos::CryptohomeClient* const client =
      chromeos::DBusThreadManager::Get()->GetCryptohomeClient();
  client->WaitForServiceToBeAvailable(
      base::Bind(&PerformDelayedCryptohomeRemovals));
}

// static
ArcKioskAppManager* ArcKioskAppManager::Get() {
  return g_arc_kiosk_app_manager;
}

ArcKioskAppManager::ArcKioskAppManager()
    : auto_launch_account_id_(EmptyAccountId()) {
  DCHECK(!g_arc_kiosk_app_manager);  // Only one instance is allowed.
  UpdateApps();
  local_accounts_subscription_ = CrosSettings::Get()->AddSettingsObserver(
      kAccountsPrefDeviceLocalAccounts,
      base::Bind(&ArcKioskAppManager::UpdateApps, base::Unretained(this)));
  local_account_auto_login_id_subscription_ =
      CrosSettings::Get()->AddSettingsObserver(
          kAccountsPrefDeviceLocalAccountAutoLoginId,
          base::Bind(&ArcKioskAppManager::UpdateApps, base::Unretained(this)));
  g_arc_kiosk_app_manager = this;
}

ArcKioskAppManager::~ArcKioskAppManager() {
  local_accounts_subscription_.reset();
  local_account_auto_login_id_subscription_.reset();
  apps_.clear();
  g_arc_kiosk_app_manager = nullptr;
}

const AccountId& ArcKioskAppManager::GetAutoLaunchAccountId() const {
  return auto_launch_account_id_;
}

const ArcKioskAppData* ArcKioskAppManager::GetAppByAccountId(
    const AccountId& account_id) {
  for (auto& app : apps_) {
    if (app->account_id() == account_id)
      return app.get();
  }
  return nullptr;
}

void ArcKioskAppManager::GetAllApps(Apps* apps) const {
  apps->clear();
  apps->reserve(apps_.size());
  for (auto& app : apps_)
    apps->push_back(app.get());
}

void ArcKioskAppManager::UpdateNameAndIcon(const std::string& app_id,
                                           const std::string& name,
                                           const gfx::ImageSkia& icon) {
  for (auto& app : apps_) {
    if (app->app_id() == app_id) {
      app->SetCache(name, icon);
      return;
    }
  }
}

void ArcKioskAppManager::AddObserver(ArcKioskAppManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void ArcKioskAppManager::RemoveObserver(ArcKioskAppManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void ArcKioskAppManager::AddAutoLaunchAppForTest(
    const std::string& app_id,
    const policy::ArcKioskAppBasicInfo& app_info,
    const AccountId& account_id) {
  for (auto it = apps_.begin(); it != apps_.end(); ++it) {
    if ((*it)->app_id() == app_id) {
      apps_.erase(it);
      break;
    }
  }

  apps_.emplace_back(std::make_unique<ArcKioskAppData>(
      app_id, app_info.package_name(), app_info.class_name(), app_info.action(),
      account_id, app_info.display_name()));

  auto_launch_account_id_ = account_id;
  auto_launched_with_zero_delay_ = true;
}

void ArcKioskAppManager::UpdateApps() {
  // Do not populate ARC kiosk apps if ARC kiosk apps can't be run on the
  // device.
  // Apps won't be added to kiosk Apps menu and won't be auto-launched.
  if (!arc::IsArcKioskAvailable()) {
    VLOG(1) << "Device doesn't support ARC kiosk";
    return;
  }

  // Store current apps. We will compare old and new apps to determine which
  // apps are new, and which were deleted.
  std::map<std::string, std::unique_ptr<ArcKioskAppData>> old_apps;
  for (auto& app : apps_)
    old_apps[app->app_id()] = std::move(app);
  apps_.clear();
  auto_launch_account_id_.clear();
  auto_launched_with_zero_delay_ = false;
  std::string auto_login_account_id_from_settings;
  CrosSettings::Get()->GetString(kAccountsPrefDeviceLocalAccountAutoLoginId,
                                 &auto_login_account_id_from_settings);

  // Re-populates |apps_| and reuses existing apps when possible.
  const std::vector<policy::DeviceLocalAccount> device_local_accounts =
      policy::GetDeviceLocalAccounts(CrosSettings::Get());
  for (auto account : device_local_accounts) {
    if (account.type != policy::DeviceLocalAccount::TYPE_ARC_KIOSK_APP)
      continue;

    const AccountId account_id(AccountId::FromUserEmail(account.user_id));

    if (account.account_id == auto_login_account_id_from_settings) {
      auto_launch_account_id_ = account_id;
      int auto_launch_delay = 0;
      CrosSettings::Get()->GetInteger(
          kAccountsPrefDeviceLocalAccountAutoLoginDelay, &auto_launch_delay);
      auto_launched_with_zero_delay_ = auto_launch_delay == 0;
    }

    const policy::ArcKioskAppBasicInfo& app_info = account.arc_kiosk_app_info;
    std::string app_id;
    if (!app_info.class_name().empty()) {
      app_id = ArcAppListPrefs::GetAppId(app_info.package_name(),
                                         app_info.class_name());
    } else {
      app_id =
          ArcAppListPrefs::GetAppId(app_info.package_name(), app_info.action());
    }
    auto old_it = old_apps.find(app_id);
    if (old_it != old_apps.end()) {
      apps_.push_back(std::move(old_it->second));
      old_apps.erase(old_it);
    } else {
      // Use package name when display name is not specified.
      std::string name = app_info.package_name();
      if (!app_info.display_name().empty())
        name = app_info.display_name();
      apps_.push_back(std::make_unique<ArcKioskAppData>(
          app_id, app_info.package_name(), app_info.class_name(),
          app_info.action(), account_id, name));
      apps_.back()->LoadFromCache();
    }
    CancelDelayedCryptohomeRemoval(cryptohome::Identification(account_id));
  }

  ClearRemovedApps(old_apps);

  for (auto& observer : observers_) {
    observer.OnArcKioskAppsChanged();
  }
}

void ArcKioskAppManager::ClearRemovedApps(
    const std::map<std::string, std::unique_ptr<ArcKioskAppData>>& old_apps) {
  // Check if currently active user must be deleted.
  bool active_user_to_be_deleted = false;
  const user_manager::User* active_user =
      user_manager::UserManager::Get()->GetActiveUser();
  if (active_user) {
    const AccountId active_account_id = active_user->GetAccountId();
    for (const auto& it : old_apps) {
      if (it.second->account_id() == active_account_id) {
        active_user_to_be_deleted = true;
        break;
      }
    }
  }

  // Remove cryptohome
  for (auto& entry : old_apps) {
    entry.second->ClearCache();
    const cryptohome::Identification cryptohome_id(entry.second->account_id());
    if (active_user_to_be_deleted) {
      // Schedule cryptohome removal after active user logout.
      ScheduleDelayedCryptohomeRemoval(cryptohome_id);
    } else {
      cryptohome::AsyncMethodCaller::GetInstance()->AsyncRemove(
          cryptohome_id, base::Bind(&OnRemoveAppCryptohomeComplete,
                                    cryptohome_id, base::Closure()));
    }
  }

  if (active_user_to_be_deleted)
    chrome::AttemptUserExit();
}

}  // namespace chromeos
