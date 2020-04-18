// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APP_MODE_ARC_ARC_KIOSK_APP_MANAGER_H_
#define CHROME_BROWSER_CHROMEOS_APP_MODE_ARC_ARC_KIOSK_APP_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/app_mode/arc/arc_kiosk_app_data.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "components/account_id/account_id.h"

class PrefRegistrySimple;

namespace chromeos {

// Keeps track of Android apps that are to be launched in kiosk mode.
// For removed apps deletes appropriate cryptohome. The information about
// kiosk apps are received from CrosSettings. For each app, the system
// creates a user in whose context the app then runs.
class ArcKioskAppManager {
 public:
  using Apps = std::vector<ArcKioskAppData*>;

  class ArcKioskAppManagerObserver {
   public:
    virtual void OnArcKioskAppsChanged() {}

   protected:
    virtual ~ArcKioskAppManagerObserver() = default;
  };

  static const char kArcKioskDictionaryName[];

  static ArcKioskAppManager* Get();

  ArcKioskAppManager();
  ~ArcKioskAppManager();

  // Registers kiosk app entries in local state.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Removes cryptohomes which could not be removed during the previous session.
  static void RemoveObsoleteCryptohomes();

  // Returns auto launched account id. If there is none, account is invalid,
  // thus is_valid() returns empty AccountId.
  const AccountId& GetAutoLaunchAccountId() const;

  // Returns app that should be started for given account id.
  const ArcKioskAppData* GetAppByAccountId(const AccountId& account_id);

  void GetAllApps(Apps* apps) const;

  void UpdateNameAndIcon(const std::string& app_id,
                         const std::string& name,
                         const gfx::ImageSkia& icon);

  void AddObserver(ArcKioskAppManagerObserver* observer);
  void RemoveObserver(ArcKioskAppManagerObserver* observer);

  bool current_app_was_auto_launched_with_zero_delay() const {
    return auto_launched_with_zero_delay_;
  }

  // Adds an app with the given meta data directly, skips meta data fetching
  // and sets the app as the auto launched one. Only for test.
  void AddAutoLaunchAppForTest(const std::string& app_id,
                               const policy::ArcKioskAppBasicInfo& app_info,
                               const AccountId& account_id);

 private:
  // Updates apps_ based on CrosSettings.
  void UpdateApps();

  // Removes cryptohomes of the removed apps. Terminates the session if
  // a removed app is running.
  void ClearRemovedApps(
      const std::map<std::string, std::unique_ptr<ArcKioskAppData>>& old_apps);

  std::vector<std::unique_ptr<ArcKioskAppData>> apps_;
  AccountId auto_launch_account_id_;
  bool auto_launched_with_zero_delay_ = false;
  base::ObserverList<ArcKioskAppManagerObserver, true> observers_;

  std::unique_ptr<CrosSettings::ObserverSubscription>
      local_accounts_subscription_;
  std::unique_ptr<CrosSettings::ObserverSubscription>
      local_account_auto_login_id_subscription_;

  DISALLOW_COPY_AND_ASSIGN(ArcKioskAppManager);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APP_MODE_ARC_ARC_KIOSK_APP_MANAGER_H_
