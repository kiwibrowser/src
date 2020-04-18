// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_POWER_PREFS_H_
#define CHROME_BROWSER_CHROMEOS_POWER_POWER_PREFS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/time/tick_clock.h"
#include "chromeos/dbus/power_manager_client.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PrefChangeRegistrar;
class Profile;

namespace power_manager {
class ScreenIdleState;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace chromeos {

class PowerPolicyController;

// Sends an updated power policy to the |power_policy_controller| whenever one
// of the power-related prefs changes.
class PowerPrefs : public PowerManagerClient::Observer,
                   public content::NotificationObserver {
 public:
  PowerPrefs(PowerPolicyController* power_policy_controller,
             PowerManagerClient* power_manager_client);
  ~PowerPrefs() override;

  // Register power prefs with default values applicable to a user profile.
  static void RegisterUserProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry);

  // Register power prefs with default values applicable to the login profile.
  static void RegisterLoginProfilePrefs(
      user_prefs::PrefRegistrySyncable* registry);

  void set_tick_clock_for_test(const base::TickClock* clock) {
    tick_clock_ = clock;
  }

  // PowerManagerClient::Observer:
  void ScreenIdleStateChanged(
      const power_manager::ScreenIdleState& proto) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void UpdatePowerPolicyFromPrefs();

 private:
  friend class PowerPrefsTest;

  // Register power prefs whose default values are the same in user profiles and
  // the login profile.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  void SetProfile(Profile* profile);

  PowerPolicyController* power_policy_controller_;  // Not owned.

  content::NotificationRegistrar notification_registrar_;

  ScopedObserver<PowerManagerClient, PowerManagerClient::Observer>
      power_manager_client_observer_;

  Profile* profile_ = nullptr;  // Not owned.
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  const base::TickClock* tick_clock_;  // Not owned.

  // Time at which the screen was locked. Unset if the screen is unlocked.
  base::TimeTicks screen_lock_time_;

  // Time at which the screen was last turned off due to user inactivity.
  // Unset if the screen isn't currently turned off due to user inactivity.
  base::TimeTicks screen_idle_off_time_;

  DISALLOW_COPY_AND_ASSIGN(PowerPrefs);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_POWER_PREFS_H_
