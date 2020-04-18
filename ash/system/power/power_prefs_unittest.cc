// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_prefs.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/session/session_controller.h"
#include "ash/session/test_session_controller_client.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/test/ash_test_base.h"
#include "base/macros.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "components/prefs/pref_service.h"

using session_manager::SessionState;

namespace ash {

namespace {

// Screen lock state that determines which delays are used by
// GetExpectedPowerPolicyForPrefs().
enum class ScreenLockState {
  LOCKED,
  UNLOCKED,
};

PrefService* GetSigninScreenPrefService() {
  return Shell::Get()->session_controller()->GetSigninScreenPrefService();
}

PrefService* GetLastActiveUserPrefService() {
  return Shell::Get()->session_controller()->GetLastActiveUserPrefService();
}

std::string GetExpectedPowerPolicyForPrefs(PrefService* prefs,
                                           ScreenLockState screen_lock_state) {
  power_manager::PowerManagementPolicy expected_policy;
  expected_policy.mutable_ac_delays()->set_screen_dim_ms(
      prefs->GetInteger(screen_lock_state == ScreenLockState::LOCKED
                            ? prefs::kPowerLockScreenDimDelayMs
                            : prefs::kPowerAcScreenDimDelayMs));
  expected_policy.mutable_ac_delays()->set_screen_off_ms(
      prefs->GetInteger(screen_lock_state == ScreenLockState::LOCKED
                            ? prefs::kPowerLockScreenOffDelayMs
                            : prefs::kPowerAcScreenOffDelayMs));
  expected_policy.mutable_ac_delays()->set_screen_lock_ms(
      prefs->GetInteger(prefs::kPowerAcScreenLockDelayMs));
  expected_policy.mutable_ac_delays()->set_idle_warning_ms(
      prefs->GetInteger(prefs::kPowerAcIdleWarningDelayMs));
  expected_policy.mutable_ac_delays()->set_idle_ms(
      prefs->GetInteger(prefs::kPowerAcIdleDelayMs));
  expected_policy.mutable_battery_delays()->set_screen_dim_ms(
      prefs->GetInteger(screen_lock_state == ScreenLockState::LOCKED
                            ? prefs::kPowerLockScreenDimDelayMs
                            : prefs::kPowerBatteryScreenDimDelayMs));
  expected_policy.mutable_battery_delays()->set_screen_off_ms(
      prefs->GetInteger(screen_lock_state == ScreenLockState::LOCKED
                            ? prefs::kPowerLockScreenOffDelayMs
                            : prefs::kPowerBatteryScreenOffDelayMs));
  expected_policy.mutable_battery_delays()->set_screen_lock_ms(
      prefs->GetInteger(prefs::kPowerBatteryScreenLockDelayMs));
  expected_policy.mutable_battery_delays()->set_idle_warning_ms(
      prefs->GetInteger(prefs::kPowerBatteryIdleWarningDelayMs));
  expected_policy.mutable_battery_delays()->set_idle_ms(
      prefs->GetInteger(prefs::kPowerBatteryIdleDelayMs));
  expected_policy.set_ac_idle_action(
      static_cast<power_manager::PowerManagementPolicy_Action>(
          prefs->GetInteger(prefs::kPowerAcIdleAction)));
  expected_policy.set_battery_idle_action(
      static_cast<power_manager::PowerManagementPolicy_Action>(
          prefs->GetInteger(prefs::kPowerBatteryIdleAction)));
  expected_policy.set_lid_closed_action(
      static_cast<power_manager::PowerManagementPolicy_Action>(
          prefs->GetInteger(prefs::kPowerLidClosedAction)));
  expected_policy.set_use_audio_activity(
      prefs->GetBoolean(prefs::kPowerUseAudioActivity));
  expected_policy.set_use_video_activity(
      prefs->GetBoolean(prefs::kPowerUseVideoActivity));
  expected_policy.set_presentation_screen_dim_delay_factor(
      prefs->GetDouble(prefs::kPowerPresentationScreenDimDelayFactor));
  expected_policy.set_user_activity_screen_dim_delay_factor(
      prefs->GetDouble(prefs::kPowerUserActivityScreenDimDelayFactor));
  expected_policy.set_wait_for_initial_user_activity(
      prefs->GetBoolean(prefs::kPowerWaitForInitialUserActivity));
  expected_policy.set_force_nonzero_brightness_for_user_activity(
      prefs->GetBoolean(prefs::kPowerForceNonzeroBrightnessForUserActivity));
  expected_policy.set_reason("Prefs");
  return chromeos::PowerPolicyController::GetPolicyDebugString(expected_policy);
}

bool GetExpectedAllowScreenWakeLocksForPrefs(PrefService* prefs) {
  return prefs->GetBoolean(prefs::kPowerAllowScreenWakeLocks);
}

}  // namespace

class PowerPrefsTest : public NoSessionAshTestBase {
 protected:
  PowerPrefsTest() = default;
  ~PowerPrefsTest() override = default;

  // NoSessionAshTestBase:
  void SetUp() override {
    fake_power_manager_client_ = new chromeos::FakePowerManagerClient;
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        base::WrapUnique(fake_power_manager_client_));

    NoSessionAshTestBase::SetUp();

    power_policy_controller_ = chromeos::PowerPolicyController::Get();
    power_prefs_ = ShellTestApi(Shell::Get()).power_prefs();

    // Advance the clock an arbitrary amount of time so it won't report zero.
    tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
    power_prefs_->set_tick_clock_for_test(&tick_clock_);

    // Get to Login screen.
    GetSessionControllerClient()->SetSessionState(SessionState::LOGIN_PRIMARY);
  }

  std::string GetCurrentPowerPolicy() const {
    return chromeos::PowerPolicyController::GetPolicyDebugString(
        fake_power_manager_client_->policy());
  }

  bool GetCurrentAllowScreenWakeLocks() const {
    return power_policy_controller_->honor_screen_wake_locks_for_test();
  }

  std::vector<power_manager::PowerManagementPolicy_Action>
  GetCurrentPowerPolicyActions() const {
    return {fake_power_manager_client_->policy().ac_idle_action(),
            fake_power_manager_client_->policy().battery_idle_action(),
            fake_power_manager_client_->policy().lid_closed_action()};
  }

  void SetLockedState(ScreenLockState lock_state) {
    GetSessionControllerClient()->SetSessionState(
        lock_state == ScreenLockState::LOCKED ? SessionState::LOCKED
                                              : SessionState::ACTIVE);
  }

  void NotifyScreenIdleOffChanged(bool off) {
    power_manager::ScreenIdleState proto;
    proto.set_off(off);
    fake_power_manager_client_->SendScreenIdleStateChanged(proto);
  }

  // Owned by chromeos::DBusThreadManager.
  chromeos::FakePowerManagerClient* fake_power_manager_client_;

  chromeos::PowerPolicyController* power_policy_controller_ =
      nullptr;                         // Not owned.
  PowerPrefs* power_prefs_ = nullptr;  // Not owned.
  base::SimpleTestTickClock tick_clock_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerPrefsTest);
};

TEST_F(PowerPrefsTest, LoginScreen) {
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  EXPECT_EQ(GetSigninScreenPrefService(), prefs);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForPrefs(prefs),
            GetCurrentAllowScreenWakeLocks());

  // Lock the screen and check that the expected delays are used.
  SetLockedState(ScreenLockState::LOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::LOCKED),
            GetCurrentPowerPolicy());

  // Unlock the screen.
  SetLockedState(ScreenLockState::UNLOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());
}

TEST_F(PowerPrefsTest, UserSession) {
  SimulateUserLogin("user@test.com");
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  EXPECT_EQ(GetLastActiveUserPrefService(), prefs);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForPrefs(prefs),
            GetCurrentAllowScreenWakeLocks());
}

TEST_F(PowerPrefsTest, AvoidLockDelaysAfterInactivity) {
  SimulateUserLogin("user@test.com");
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  EXPECT_EQ(GetLastActiveUserPrefService(), prefs);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());

  // If the screen was already off due to inactivity when it was locked, we
  // should continue using the unlocked delays.
  NotifyScreenIdleOffChanged(true);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  SetLockedState(ScreenLockState::LOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());

  // If the screen turns on while still locked, we should switch to the locked
  // delays.
  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  NotifyScreenIdleOffChanged(false);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::LOCKED),
            GetCurrentPowerPolicy());

  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  SetLockedState(ScreenLockState::UNLOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForPrefs(prefs, ScreenLockState::UNLOCKED),
            GetCurrentPowerPolicy());
}

TEST_F(PowerPrefsTest, DisabledLockScreen) {
  SimulateUserLogin("user@test.com");
  PrefService* prefs =
      Shell::Get()->session_controller()->GetActivePrefService();
  EXPECT_EQ(GetLastActiveUserPrefService(), prefs);

  // Verify that the power policy actions are set to default values initially.
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The automatic screen locking is enabled, but, as the lock screen is
  // allowed, the power policy actions still have the default values.
  prefs->SetBoolean(prefs::kEnableAutoScreenLock, true);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The lock screen is disabled, but, as automatic screen locking is not
  // enabled, the power policy actions still have the default values.
  prefs->ClearPref(prefs::kEnableAutoScreenLock);
  prefs->SetBoolean(prefs::kAllowScreenLock, false);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The automatic screen locking is enabled and the lock screen is disabled, so
  // the power policy actions are set now to stop the user session.
  prefs->SetBoolean(prefs::kEnableAutoScreenLock, true);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_STOP_SESSION),
            GetCurrentPowerPolicyActions());
}

}  // namespace ash
