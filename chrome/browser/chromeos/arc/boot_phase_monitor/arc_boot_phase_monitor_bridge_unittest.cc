// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/boot_phase_monitor/arc_boot_phase_monitor_bridge.h"

#include <memory>

#include "base/command_line.h"
#include "base/threading/platform_thread.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_session_manager_client.h"
#include "components/arc/arc_prefs.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_util.h"
#include "components/arc/test/fake_arc_session.h"
#include "components/browser_sync/profile_sync_test_util.h"
#include "components/user_manager/scoped_user_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

class ArcBootPhaseMonitorBridgeTest : public testing::Test {
 public:
  ArcBootPhaseMonitorBridgeTest()
      : scoped_user_manager_(
            std::make_unique<chromeos::FakeChromeUserManager>()),
        arc_service_manager_(std::make_unique<ArcServiceManager>()),
        arc_session_manager_(std::make_unique<ArcSessionManager>(
            std::make_unique<ArcSessionRunner>(
                base::Bind(FakeArcSession::Create)))),
        testing_profile_(std::make_unique<TestingProfile>()),
        disable_cpu_restriction_counter_(0),
        record_uma_counter_(0) {
    chromeos::DBusThreadManager::GetSetterForTesting()->SetSessionManagerClient(
        std::make_unique<chromeos::FakeSessionManagerClient>());

    SetArcAvailableCommandLineForTesting(
        base::CommandLine::ForCurrentProcess());

    const AccountId account_id(AccountId::FromUserEmailGaiaId(
        testing_profile_->GetProfileUserName(), "1234567890"));
    GetFakeUserManager()->AddUser(account_id);
    GetFakeUserManager()->LoginUser(account_id);

    boot_phase_monitor_bridge_ =
        ArcBootPhaseMonitorBridge::GetForBrowserContextForTesting(
            testing_profile_.get());
    boot_phase_monitor_bridge_->SetDelegateForTesting(
        std::make_unique<TestDelegateImpl>(this));
  }

  ~ArcBootPhaseMonitorBridgeTest() override {
    boot_phase_monitor_bridge_->Shutdown();
    chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  ArcSessionManager* arc_session_manager() const {
    return arc_session_manager_.get();
  }
  ArcBootPhaseMonitorBridge* boot_phase_monitor_bridge() const {
    return boot_phase_monitor_bridge_;
  }
  size_t disable_cpu_restriction_counter() const {
    return disable_cpu_restriction_counter_;
  }
  size_t record_uma_counter() const { return record_uma_counter_; }
  base::TimeDelta last_time_delta() const { return last_time_delta_; }

  sync_preferences::TestingPrefServiceSyncable* GetPrefs() const {
    return testing_profile_->GetTestingPrefService();
  }

 private:
  class TestDelegateImpl : public ArcBootPhaseMonitorBridge::Delegate {
   public:
    explicit TestDelegateImpl(ArcBootPhaseMonitorBridgeTest* test)
        : test_(test) {}
    ~TestDelegateImpl() override = default;

    // ArcBootPhaseMonitorBridge::Delegate overrides:
    void DisableCpuRestriction() override {
      ++(test_->disable_cpu_restriction_counter_);
    }
    void RecordFirstAppLaunchDelayUMA(base::TimeDelta delta) override {
      test_->last_time_delta_ = delta;
      ++(test_->record_uma_counter_);
    }

   private:
    ArcBootPhaseMonitorBridgeTest* const test_;

    DISALLOW_COPY_AND_ASSIGN(TestDelegateImpl);
  };

  chromeos::FakeChromeUserManager* GetFakeUserManager() const {
    return static_cast<chromeos::FakeChromeUserManager*>(
        user_manager::UserManager::Get());
  }

  content::TestBrowserThreadBundle thread_bundle_;
  user_manager::ScopedUserManager scoped_user_manager_;
  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<ArcSessionManager> arc_session_manager_;
  std::unique_ptr<TestingProfile> testing_profile_;
  ArcBootPhaseMonitorBridge* boot_phase_monitor_bridge_;

  size_t disable_cpu_restriction_counter_;
  size_t record_uma_counter_;
  base::TimeDelta last_time_delta_;

  DISALLOW_COPY_AND_ASSIGN(ArcBootPhaseMonitorBridgeTest);
};

// Tests that ArcBootPhaseMonitorBridge can be constructed and destructed.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestConstructDestruct) {}

// Tests that the throttle is created on BootCompleted.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestThrottleCreation) {
  // Tell |arc_session_manager_| that this is not opt-in boot.
  arc_session_manager()->set_directly_started_for_testing(true);

  // Initially, |throttle_| is null.
  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // OnBootCompleted() creates it.
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_NE(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // ..but it's removed when the session stops.
  boot_phase_monitor_bridge()->OnArcSessionStopped(ArcStopReason::SHUTDOWN);
  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
}

// Tests the same but with OnSessionRestarting().
TEST_F(ArcBootPhaseMonitorBridgeTest, TestThrottleCreation_Restart) {
  // Tell |arc_session_manager_| that this is not opt-in boot.
  arc_session_manager()->set_directly_started_for_testing(true);

  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_NE(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // Call OnArcSessionRestarting() instead, and confirm that |throttle_| is
  // gone.
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
  boot_phase_monitor_bridge()->OnArcSessionRestarting();
  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  EXPECT_EQ(1U, disable_cpu_restriction_counter());
  // Also make sure that |throttle| is created again once restarting id done.
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_NE(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
}

// Tests that the throttle is created on ArcInitialStart when opting in.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestThrottleCreation_OptIn) {
  // Tell |arc_session_manager_| that this *is* opt-in boot.
  arc_session_manager()->set_directly_started_for_testing(false);

  // Initially, |throttle_| is null.
  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // OnArcInitialStart(), which is called when the user accepts ToS, creates
  // |throttle_|.
  boot_phase_monitor_bridge()->OnArcInitialStart();
  EXPECT_NE(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // ..and OnBootCompleted() does not delete it.
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_NE(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
  // ..but it's removed when the session stops.
  boot_phase_monitor_bridge()->OnArcSessionStopped(ArcStopReason::SHUTDOWN);
  EXPECT_EQ(nullptr, boot_phase_monitor_bridge()->throttle_for_testing());
}

// Tests that the UMA recording function is never called unless
// RecordFirstAppLaunchDelayUMA is called.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestRecordUMA_None) {
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->OnArcSessionStopped(ArcStopReason::SHUTDOWN);
  EXPECT_EQ(0U, record_uma_counter());
}

// Tests that RecordFirstAppLaunchDelayUMA() actually calls the UMA recording
// function (but only after OnBootCompleted.)
TEST_F(ArcBootPhaseMonitorBridgeTest, TestRecordUMA_AppLaunchBeforeBoot) {
  EXPECT_EQ(0U, record_uma_counter());
  // Calling RecordFirstAppLaunchDelayUMA() before boot shouldn't immediately
  // record UMA.
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(0U, record_uma_counter());
  // Sleep for 1ms just to make sure 0 won't be passed to RecordUMA().
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(1));
  // UMA recording should be done on BootCompleted.
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(1U, record_uma_counter());
  // In this case, |delta| passed to the UMA recording function should be >0.
  EXPECT_LT(base::TimeDelta(), last_time_delta());
}

// Tests the same with calling RecordFirstAppLaunchDelayUMA() after boot.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestRecordUMA_AppLaunchAfterBoot) {
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(0U, record_uma_counter());
  // Calling RecordFirstAppLaunchDelayUMA() after boot should immediately record
  // UMA.
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(1U, record_uma_counter());
  // In this case, |delta| passed to the UMA recording function should be 0.
  EXPECT_TRUE(last_time_delta().is_zero());
}

// Tests the same with calling RecordFirstAppLaunchDelayUMA() twice.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestRecordUMA_AppLaunchesBeforeAndAfterBoot) {
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(1U, record_uma_counter());
  EXPECT_LT(base::TimeDelta(), last_time_delta());
  // Call the record function again and check that the counter is not changed.
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(1U, record_uma_counter());
}

// Tests the same with calling RecordFirstAppLaunchDelayUMA() twice after boot.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestRecordUMA_AppLaunchesAfterBoot) {
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(0U, record_uma_counter());
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(1U, record_uma_counter());
  EXPECT_TRUE(last_time_delta().is_zero());
  // Call the record function again and check that the counter is not changed.
  boot_phase_monitor_bridge()->RecordFirstAppLaunchDelayUMAForTesting();
  EXPECT_EQ(1U, record_uma_counter());
}

// Tests that CPU restriction is disabled when tab restoration is done.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestOnSessionRestoreFinishedLoadingTabs) {
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
  boot_phase_monitor_bridge()->OnSessionRestoreFinishedLoadingTabs();
  EXPECT_EQ(1U, disable_cpu_restriction_counter());
}

// Tests that nothing happens if tab restoration is done after ARC boot.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestOnSessionRestoreFinishedLoadingTabs_BootFirst) {
  // Tell |arc_session_manager_| that this is not opt-in boot.
  arc_session_manager()->set_directly_started_for_testing(true);

  // However, OnSessionRestoreFinishedLoadingTabs() should be no-op when the
  // instance has already fully started.
  boot_phase_monitor_bridge()->OnBootCompleted();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
  boot_phase_monitor_bridge()->OnSessionRestoreFinishedLoadingTabs();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
}

// Tests that OnExtensionsReady() does nothing by default.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestOnExtensionsReady) {
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
}

// Tests that OnExtensionsReady() does nothing when prefs::kArcEnabled is not
// set.
TEST_F(ArcBootPhaseMonitorBridgeTest, TestOnExtensionsReady_ArcNotEnabled) {
  boot_phase_monitor_bridge()->OnArcPlayStoreEnabledChanged(false);
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
}

// Tests that OnExtensionsReady() does nothing when prefs::kArcEnabled is not
// managed.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestOnExtensionsReady_ArcEnabledButUnmanaged) {
  GetPrefs()->SetUserPref(prefs::kArcEnabled,
                          std::make_unique<base::Value>(true));
  boot_phase_monitor_bridge()->OnArcPlayStoreEnabledChanged(true);
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
}

// Tests that OnExtensionsReady() relaxes the CPU restriction when ARC is
// enabled by policy.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestOnExtensionsReady_ArcEnabledAndManaged) {
  GetPrefs()->SetManagedPref(prefs::kArcEnabled,
                             std::make_unique<base::Value>(true));
  boot_phase_monitor_bridge()->OnArcPlayStoreEnabledChanged(true);
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(1U, disable_cpu_restriction_counter());
}

// Does the same but in a reversed event order.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestOnExtensionsReady_ArcEnabledAndManaged2) {
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
  GetPrefs()->SetManagedPref(prefs::kArcEnabled,
                             std::make_unique<base::Value>(true));
  boot_phase_monitor_bridge()->OnArcPlayStoreEnabledChanged(true);
  EXPECT_EQ(1U, disable_cpu_restriction_counter());
}

// Does the same but after ARC boot is completed. This emulates the
// situation where ARC instance finishes booting before other conditions
// (|extentions_ready_| and |enabled_by_policy_|) are met.
// |disable_cpu_restriction_counter| should stay 0 in this case because
// ArcInstanceThrottle already has the control.
TEST_F(ArcBootPhaseMonitorBridgeTest,
       TestOnExtensionsReady_AfterBootCompleted) {
  // Tell |arc_session_manager_| that this is not opt-in boot.
  arc_session_manager()->set_directly_started_for_testing(true);

  GetPrefs()->SetManagedPref(prefs::kArcEnabled,
                             std::make_unique<base::Value>(true));
  boot_phase_monitor_bridge()->OnArcPlayStoreEnabledChanged(true);

  boot_phase_monitor_bridge()->OnBootCompleted();
  boot_phase_monitor_bridge()->OnExtensionsReadyForTesting();
  EXPECT_EQ(0U, disable_cpu_restriction_counter());
}

}  // namespace

}  // namespace arc
