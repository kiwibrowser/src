// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/power_prefs.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/test/simple_test_tick_clock.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/extensions/extension_special_storage_policy.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/policy.pb.h"
#include "chromeos/dbus/power_policy_controller.h"
#include "components/account_id/account_id.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

class PowerPrefsTest : public testing::Test {
 protected:
  // Screen lock state that determines which delays are used by
  // GetExpectedPowerPolicyForProfile().
  enum ScreenLockState {
    LOCKED,
    UNLOCKED,
  };

  PowerPrefsTest();

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  const Profile* GetProfile() const;

  std::string GetExpectedPowerPolicyForProfile(
      Profile* profile,
      ScreenLockState screen_lock_state) const;
  std::string GetCurrentPowerPolicy() const;
  bool GetExpectedAllowScreenWakeLocksForProfile(Profile* profile) const;
  bool GetCurrentAllowScreenWakeLocks() const;

  // Notify |power_prefs_| about various events.
  void NotifySessionStarted();
  void NotifyProfileDestroyed(Profile* profile);
  void NotifyLockStateChanged(Profile* profile, ScreenLockState state);
  void NotifyLoginOrLockScreenShown();
  void NotifyScreenIdleOffChanged(bool off);

  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfileManager profile_manager_;
  PowerPolicyController* power_policy_controller_ = nullptr;  // Not owned.
  std::unique_ptr<FakePowerManagerClient> fake_power_manager_client_;
  base::SimpleTestTickClock tick_clock_;

  std::unique_ptr<PowerPrefs> power_prefs_;

  DISALLOW_COPY_AND_ASSIGN(PowerPrefsTest);
};

PowerPrefsTest::PowerPrefsTest()
    : profile_manager_(TestingBrowserProcess::GetGlobal()),
      fake_power_manager_client_(std::make_unique<FakePowerManagerClient>()) {}

void PowerPrefsTest::SetUp() {
  testing::Test::SetUp();

  PowerPolicyController::Initialize(fake_power_manager_client_.get());
  power_policy_controller_ = PowerPolicyController::Get();

  ASSERT_TRUE(profile_manager_.SetUp());

  power_prefs_ = std::make_unique<PowerPrefs>(power_policy_controller_,
                                              fake_power_manager_client_.get());

  // Advance the clock an arbitrary amount of time so it won't report zero.
  tick_clock_.Advance(base::TimeDelta::FromSeconds(1));
  power_prefs_->set_tick_clock_for_test(&tick_clock_);

  EXPECT_FALSE(GetProfile());
  EXPECT_EQ(PowerPolicyController::GetPolicyDebugString(
                power_manager::PowerManagementPolicy()),
            GetCurrentPowerPolicy());
}

void PowerPrefsTest::TearDown() {
  power_prefs_.reset();
  PowerPolicyController::Shutdown();
  testing::Test::TearDown();
}

const Profile* PowerPrefsTest::GetProfile() const {
  return power_prefs_->profile_;
}

std::string PowerPrefsTest::GetExpectedPowerPolicyForProfile(
    Profile* profile,
    ScreenLockState screen_lock_state) const {
  const PrefService* prefs = profile->GetPrefs();
  power_manager::PowerManagementPolicy expected_policy;
  expected_policy.mutable_ac_delays()->set_screen_dim_ms(prefs->GetInteger(
      screen_lock_state == LOCKED ? prefs::kPowerLockScreenDimDelayMs
                                  : prefs::kPowerAcScreenDimDelayMs));
  expected_policy.mutable_ac_delays()->set_screen_off_ms(prefs->GetInteger(
      screen_lock_state == LOCKED ? prefs::kPowerLockScreenOffDelayMs
                                  : prefs::kPowerAcScreenOffDelayMs));
  expected_policy.mutable_ac_delays()->set_screen_lock_ms(
      prefs->GetInteger(prefs::kPowerAcScreenLockDelayMs));
  expected_policy.mutable_ac_delays()->set_idle_warning_ms(
      prefs->GetInteger(prefs::kPowerAcIdleWarningDelayMs));
  expected_policy.mutable_ac_delays()->set_idle_ms(
      prefs->GetInteger(prefs::kPowerAcIdleDelayMs));
  expected_policy.mutable_battery_delays()->set_screen_dim_ms(prefs->GetInteger(
      screen_lock_state == LOCKED ? prefs::kPowerLockScreenDimDelayMs
                                  : prefs::kPowerBatteryScreenDimDelayMs));
  expected_policy.mutable_battery_delays()->set_screen_off_ms(prefs->GetInteger(
      screen_lock_state == LOCKED ? prefs::kPowerLockScreenOffDelayMs
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
  return PowerPolicyController::GetPolicyDebugString(expected_policy);
}

std::string PowerPrefsTest::GetCurrentPowerPolicy() const {
  return PowerPolicyController::GetPolicyDebugString(
      fake_power_manager_client_->policy());
}

bool PowerPrefsTest::GetCurrentAllowScreenWakeLocks() const {
  return power_policy_controller_->honor_screen_wake_locks_;
}

bool PowerPrefsTest::GetExpectedAllowScreenWakeLocksForProfile(
    Profile* profile) const {
  return profile->GetPrefs()->GetBoolean(prefs::kPowerAllowScreenWakeLocks);
}

void PowerPrefsTest::NotifySessionStarted() {
  power_prefs_->Observe(chrome::NOTIFICATION_SESSION_STARTED,
                        content::Source<PowerPrefsTest>(this),
                        content::NotificationService::NoDetails());
}

void PowerPrefsTest::NotifyProfileDestroyed(Profile* profile) {
  power_prefs_->Observe(chrome::NOTIFICATION_PROFILE_DESTROYED,
                        content::Source<Profile>(profile),
                        content::NotificationService::NoDetails());
}

void PowerPrefsTest::NotifyLockStateChanged(Profile* profile,
                                            ScreenLockState state) {
  bool locked = (state == LOCKED);
  power_prefs_->Observe(chrome::NOTIFICATION_SCREEN_LOCK_STATE_CHANGED,
                        content::Source<Profile>(profile),
                        content::Details<bool>(&locked));
}

void PowerPrefsTest::NotifyLoginOrLockScreenShown() {
  power_prefs_->Observe(chrome::NOTIFICATION_LOGIN_OR_LOCK_WEBUI_VISIBLE,
                        content::Source<PowerPrefsTest>(this),
                        content::NotificationService::NoDetails());
}

void PowerPrefsTest::NotifyScreenIdleOffChanged(bool off) {
  power_manager::ScreenIdleState proto;
  proto.set_off(off);
  fake_power_manager_client_->SendScreenIdleStateChanged(proto);
}

TEST_F(PowerPrefsTest, LoginScreen) {
  // Set up login profile.
  std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      login_profile_prefs(new sync_preferences::TestingPrefServiceSyncable);
  RegisterLoginProfilePrefs(login_profile_prefs->registry());
  TestingProfile::Builder builder;
  builder.SetPath(
      profile_manager_.profiles_dir().AppendASCII(chrome::kInitialProfile));
  builder.SetPrefService(std::move(login_profile_prefs));
  TestingProfile* login_profile = builder.BuildIncognito(
      profile_manager_.CreateTestingProfile(chrome::kInitialProfile));

  // Inform power_prefs_ that the login screen is being shown.
  NotifyLoginOrLockScreenShown();
  EXPECT_EQ(login_profile, GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(login_profile, UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForProfile(login_profile),
            GetCurrentAllowScreenWakeLocks());

  TestingProfile* other_profile =
      profile_manager_.CreateTestingProfile("other");

  // Inform power_prefs_ that an unrelated profile has been destroyed and verify
  // that the login profile's power prefs are still being used.
  NotifyProfileDestroyed(other_profile);
  EXPECT_EQ(login_profile, GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(login_profile, UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForProfile(login_profile),
            GetCurrentAllowScreenWakeLocks());

  // Lock the screen and check that the expected delays are used.
  NotifyLockStateChanged(login_profile, LOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(login_profile, LOCKED),
            GetCurrentPowerPolicy());

  // Unlock the screen.
  NotifyLockStateChanged(login_profile, UNLOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(login_profile, UNLOCKED),
            GetCurrentPowerPolicy());

  // Inform power_prefs_ that the login profile has been destroyed.
  // The login profile's prefs should still be used.
  NotifyProfileDestroyed(login_profile);
  EXPECT_FALSE(GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(login_profile, UNLOCKED),
            GetCurrentPowerPolicy());
}

class PowerPrefsUserSessionTest : public PowerPrefsTest {
 public:
  PowerPrefsUserSessionTest();

 protected:
  // PowerPrefsTest:
  void SetUp() override;

  std::vector<power_manager::PowerManagementPolicy_Action>
  GetCurrentPowerPolicyActions() const {
    return {fake_power_manager_client_->policy().ac_idle_action(),
            fake_power_manager_client_->policy().battery_idle_action(),
            fake_power_manager_client_->policy().lid_closed_action()};
  }

  TestingProfile* user_profile_ = nullptr;
  TestingProfile* second_user_profile_ = nullptr;

 private:
  FakeChromeUserManager* user_manager_;
  user_manager::ScopedUserManager user_manager_enabler_;

  DISALLOW_COPY_AND_ASSIGN(PowerPrefsUserSessionTest);
};

PowerPrefsUserSessionTest::PowerPrefsUserSessionTest()
    : user_manager_(new FakeChromeUserManager),
      user_manager_enabler_(base::WrapUnique(user_manager_)) {}

void PowerPrefsUserSessionTest::SetUp() {
  PowerPrefsTest::SetUp();

  const char test_user1[] = "test-user1@example.com";
  const AccountId test_account_id1(AccountId::FromUserEmail(test_user1));
  user_manager_->AddUser(test_account_id1);
  user_manager_->LoginUser(test_account_id1);
  user_profile_ =
      profile_manager_.CreateTestingProfile(test_account_id1.GetUserEmail());

  const char test_user2[] = "test-user2@example.com";
  const AccountId test_account_id2(AccountId::FromUserEmail(test_user2));
  user_manager_->AddUser(test_account_id2);
  user_manager_->LoginUser(test_account_id2);
  second_user_profile_ =
      profile_manager_.CreateTestingProfile(test_account_id2.GetUserEmail());

  profile_manager_.SetLoggedIn(true);
}

TEST_F(PowerPrefsUserSessionTest, Basic) {
  NotifySessionStarted();

  EXPECT_EQ(user_profile_, GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForProfile(user_profile_),
            GetCurrentAllowScreenWakeLocks());

  // Inform power_prefs_ that an unrelated profile has been destroyed.
  NotifyProfileDestroyed(second_user_profile_);

  // Verify that the user profile's power prefs are still being used.
  EXPECT_EQ(user_profile_, GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForProfile(user_profile_),
            GetCurrentAllowScreenWakeLocks());

  // Simulate the login screen coming up as part of screen locking.
  NotifyLoginOrLockScreenShown();

  // Verify that power policy didn't revert to login screen settings.
  EXPECT_EQ(user_profile_, GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());
  EXPECT_EQ(GetExpectedAllowScreenWakeLocksForProfile(user_profile_),
            GetCurrentAllowScreenWakeLocks());

  // Inform power_prefs_ that the session has ended and the user profile has
  // been destroyed.
  NotifyProfileDestroyed(user_profile_);

  // The user profile's prefs should still be used.
  EXPECT_FALSE(GetProfile());
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());
}

TEST_F(PowerPrefsUserSessionTest, AvoidLockDelaysAfterInactivity) {
  NotifySessionStarted();
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());

  // If the screen was already off due to inactivity when it was locked, we
  // should continue using the unlocked delays.
  NotifyScreenIdleOffChanged(true);
  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  NotifyLockStateChanged(user_profile_, LOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());

  // If the screen turns on while still locked, we should switch to the locked
  // delays.
  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  NotifyScreenIdleOffChanged(false);
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, LOCKED),
            GetCurrentPowerPolicy());

  tick_clock_.Advance(base::TimeDelta::FromSeconds(5));
  NotifyLockStateChanged(user_profile_, UNLOCKED);
  EXPECT_EQ(GetExpectedPowerPolicyForProfile(user_profile_, UNLOCKED),
            GetCurrentPowerPolicy());
}

TEST_F(PowerPrefsUserSessionTest, DisabledLockScreen) {
  NotifySessionStarted();
  EXPECT_EQ(user_profile_, GetProfile());

  // Verify that the power policy actions are set to default values initially.
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The automatic screen locking is enabled, but, as the lock screen is
  // allowed, the power policy actions still have the default values.
  user_profile_->GetPrefs()->SetBoolean(prefs::kEnableAutoScreenLock, true);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The lock screen is disabled, but, as automatic screen locking is not
  // enabled, the power policy actions still have the default values.
  user_profile_->GetPrefs()->ClearPref(prefs::kEnableAutoScreenLock);
  user_profile_->GetPrefs()->SetBoolean(prefs::kAllowScreenLock, false);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_SUSPEND),
            GetCurrentPowerPolicyActions());

  // The automatic screen locking is enabled and the lock screen is disabled, so
  // the power policy actions are set now to stop the user session.
  user_profile_->GetPrefs()->SetBoolean(prefs::kEnableAutoScreenLock, true);
  EXPECT_EQ(std::vector<power_manager::PowerManagementPolicy_Action>(
                3, power_manager::PowerManagementPolicy_Action_STOP_SESSION),
            GetCurrentPowerPolicyActions());
}

}  // namespace chromeos
