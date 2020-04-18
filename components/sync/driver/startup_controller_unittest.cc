// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/driver/startup_controller.h"

#include <memory>

#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace syncer {

// These are coupled to the implementation of StartupController's
// GetEngineInitializationStateString which is used by about:sync. We use it
// as a convenient way to verify internal state and that the class is
// outputting the correct values for the debug string.
static const char kStateStringStarted[] = "Started";
static const char kStateStringDeferred[] = "Deferred";
static const char kStateStringNotStarted[] = "Not started";

class StartupControllerTest : public testing::Test {
 public:
  StartupControllerTest() : can_start_(false), started_(false) {}

  void SetUp() override {
    SyncPrefs::RegisterProfilePrefs(pref_service_.registry());
    sync_prefs_ = std::make_unique<SyncPrefs>(&pref_service_);
    controller_ = std::make_unique<StartupController>(
        sync_prefs_.get(),
        base::Bind(&StartupControllerTest::CanStart, base::Unretained(this)),
        base::Bind(&StartupControllerTest::FakeStartBackend,
                   base::Unretained(this)));
    controller_->Reset(UserTypes());
    controller_->OverrideFallbackTimeoutForTest(
        base::TimeDelta::FromSeconds(0));
  }

  bool CanStart() { return can_start_; }

  void SetCanStart(bool can_start) { can_start_ = can_start; }

  void FakeStartBackend() {
    started_ = true;
    sync_prefs()->SetFirstSetupComplete();
  }

  void ExpectStarted() {
    EXPECT_TRUE(started());
    EXPECT_EQ(kStateStringStarted,
              controller()->GetEngineInitializationStateString());
  }

  void ExpectStartDeferred() {
    const bool deferred_start =
        !base::CommandLine::ForCurrentProcess()->HasSwitch(
            switches::kSyncDisableDeferredStartup);
    EXPECT_EQ(!deferred_start, started());
    EXPECT_EQ(deferred_start ? kStateStringDeferred : kStateStringStarted,
              controller()->GetEngineInitializationStateString());
  }

  void ExpectNotStarted() {
    EXPECT_FALSE(started());
    EXPECT_EQ(kStateStringNotStarted,
              controller()->GetEngineInitializationStateString());
  }

  bool started() const { return started_; }
  void clear_started() { started_ = false; }
  StartupController* controller() { return controller_.get(); }
  SyncPrefs* sync_prefs() { return sync_prefs_.get(); }

 private:
  bool can_start_;
  bool started_;
  base::MessageLoop message_loop_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
  std::unique_ptr<SyncPrefs> sync_prefs_;
  std::unique_ptr<StartupController> controller_;
};

// Test that sync doesn't start if setup is not in progress or complete.
TEST_F(StartupControllerTest, NoSetupComplete) {
  controller()->TryStart();
  ExpectNotStarted();

  SetCanStart(true);
  controller()->TryStart();
  ExpectNotStarted();
}

// Test that sync defers if first setup is complete.
TEST_F(StartupControllerTest, DefersAfterFirstSetupComplete) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->TryStart();
  ExpectStartDeferred();
}

// Test that a data type triggering startup starts sync immediately.
TEST_F(StartupControllerTest, NoDeferralDataTypeTrigger) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->OnDataTypeRequestsSyncStartup(SESSIONS);
  ExpectStarted();
}

// Test that a data type trigger interrupts the deferral timer and starts
// sync immediately.
TEST_F(StartupControllerTest, DataTypeTriggerInterruptsDeferral) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->TryStart();
  ExpectStartDeferred();

  controller()->OnDataTypeRequestsSyncStartup(SESSIONS);
  ExpectStarted();

  // The fallback timer shouldn't result in another invocation of the closure
  // we passed to the StartupController.
  clear_started();
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(started());
}

// Test that the fallback timer starts sync in the event all
// conditions are met and no data type requests sync.
TEST_F(StartupControllerTest, FallbackTimer) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->TryStart();
  ExpectStartDeferred();

  base::RunLoop().RunUntilIdle();
  ExpectStarted();
}

// Test that we start immediately if sessions is disabled.
TEST_F(StartupControllerTest, NoDeferralWithoutSessionsSync) {
  ModelTypeSet types(UserTypes());
  // Disabling sessions means disabling 4 types due to groupings.
  types.Remove(SESSIONS);
  types.Remove(PROXY_TABS);
  types.Remove(TYPED_URLS);
  types.Remove(SUPERVISED_USER_SETTINGS);
  sync_prefs()->SetKeepEverythingSynced(false);
  sync_prefs()->SetPreferredDataTypes(UserTypes(), types);
  controller()->Reset(UserTypes());

  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->TryStart();
  ExpectStarted();
}

// Sanity check that the fallback timer doesn't fire before startup
// conditions are met.
TEST_F(StartupControllerTest, FallbackTimerWaits) {
  controller()->TryStart();
  ExpectNotStarted();
  base::RunLoop().RunUntilIdle();
  ExpectNotStarted();
}

// Test that sync starts immediately when setup in progress is true.
TEST_F(StartupControllerTest, NoDeferralSetupInProgressTrigger) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->SetSetupInProgress(true);
  ExpectStarted();
}

// Test that setup in progress being set to true interrupts the deferral timer
// and starts sync immediately.
TEST_F(StartupControllerTest, SetupInProgressTriggerInterruptsDeferral) {
  sync_prefs()->SetFirstSetupComplete();
  SetCanStart(true);
  controller()->TryStart();
  ExpectStartDeferred();

  controller()->SetSetupInProgress(true);
  ExpectStarted();
}

// Test that immediate startup can be forced.
TEST_F(StartupControllerTest, ForceImmediateStartup) {
  SetCanStart(true);
  controller()->TryStartImmediately();
  ExpectStarted();
}

// Test that setup-in-progress tracking is persistent across a Reset.
TEST_F(StartupControllerTest, ResetDuringSetup) {
  SetCanStart(true);

  // Simulate UI telling us setup is in progress.
  controller()->SetSetupInProgress(true);

  // This could happen if the UI triggers a stop-syncing permanently call.
  controller()->Reset(UserTypes());

  // From the UI's point of view, setup is still in progress.
  EXPECT_TRUE(controller()->IsSetupInProgress());
}

}  // namespace syncer
