// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_SCREEN_TIME_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_SCREEN_TIME_CONTROLLER_H_

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/child_accounts/usage_time_limit_processor.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/session_manager/core/session_manager_observer.h"

class PrefRegistrySimple;
class PrefService;

namespace content {
class BrowserContext;
}

namespace chromeos {

// The controller to track each user's screen time usage and inquiry time limit
// processor (a component to calculate state based on policy settings and time
// usage) when necessary to determine the current lock screen state.
// Schedule notifications and lock/unlock screen based on the processor output.
class ScreenTimeController : public KeyedService,
                             public session_manager::SessionManagerObserver {
 public:
  // Registers preferences.
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit ScreenTimeController(content::BrowserContext* context);
  ~ScreenTimeController() override;

  // Returns the screen time duration. This includes time in
  // kScreenTimeMinutesUsed plus time passed since |current_screen_start_time_|.
  base::TimeDelta GetScreenTimeDuration() const;

  // Call time limit processor for new state.
  void CheckTimeLimit();

 private:
  // The types of time limit notifications. |SCREEN_TIME| is used when the
  // the screen time limit is about to be used up, and |BED_TIME| is used when
  // the bed time is approaching.
  enum TimeLimitNotificationType { kScreenTime, kBedTime };

  // Show and update the lock screen when necessary.
  // |force_lock_by_policy|: If true, force to lock the screen based on the
  //                         screen time policy.
  // |come_back_time|:       When the screen is available again.
  void LockScreen(bool force_lock_by_policy, base::Time come_back_time);

  // Show a notification indicating the remaining screen time.
  void ShowNotification(ScreenTimeController::TimeLimitNotificationType type,
                        const base::TimeDelta& time_remaining);

  // Reset time tracking relevant prefs and local timestamps.
  void RefreshScreenLimit();

  // Called when the policy of time limits changes.
  void OnPolicyChanged();

  // Reset any currently running timers.
  void ResetTimers();

  // Save the screen time progress when screen is locked, or user sign out or
  // power down the device.
  void SaveScreenTimeProgressBeforeExit();

  // Save the screen time progress periodically in case of a crash or power
  // outage.
  void SaveScreenTimeProgressPeriodically();

  // Save the |state| to |prefs::kScreenTimeLastState|.
  void SaveCurrentStateToPref(const usage_time_limit::State& state);

  // Get the last calculated |state| from |prefs::kScreenTimeLastState|, if it
  // exists.
  base::Optional<usage_time_limit::State> GetLastStateFromPref();

  // session_manager::SessionManagerObserver:
  void OnSessionStateChanged() override;

  content::BrowserContext* context_;
  PrefService* pref_service_;
  base::OneShotTimer warning_notification_timer_;
  base::OneShotTimer exit_notification_timer_;
  base::OneShotTimer next_state_timer_;
  base::RepeatingTimer save_screen_time_timer_;

  // Timestamp to keep track of the screen start time for the current active
  // screen. This timestamp is periodically updated by
  // SaveScreenTimeProgressPeriodically(), and is cleared when user exits the
  // active screen(lock, sign out, shutdown).
  base::Time current_screen_start_time_;

  // Timestamp to keep track of the screen start time when user starts using
  // the device for the first time of the day.
  // Used to calculate the screen time limit and this will be refreshed by
  // RefreshScreenLimit();
  base::Time first_screen_start_time_;

  PrefChangeRegistrar pref_change_registrar_;

  DISALLOW_COPY_AND_ASSIGN(ScreenTimeController);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CHILD_ACCOUNTS_SCREEN_TIME_CONTROLLER_H_
