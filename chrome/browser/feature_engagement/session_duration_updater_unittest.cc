// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/feature_engagement/session_duration_updater.h"

#include <memory>

#include "chrome/browser/feature_engagement/session_duration_updater.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace feature_engagement {

namespace {

constexpr char kTestObservedSessionTimeKey[] = "test_observed_session_time_key";

class TestObserver : public SessionDurationUpdater::Observer {
 public:
  TestObserver()
      : pref_service_(
            std::make_unique<sync_preferences::TestingPrefServiceSyncable>()),
        session_duration_updater_(pref_service_.get(),
                                  kTestObservedSessionTimeKey),
        session_duration_observer_(this) {
    SessionDurationUpdater::RegisterProfilePrefs(pref_service_->registry());
  }

  void AddSessionDurationObserver() {
    session_duration_observer_.Add(&session_duration_updater_);
  }

  void RemoveSessionDurationObserver() {
    session_duration_observer_.Remove(&session_duration_updater_);
  }

  // SessionDurationUpdater::Observer:
  void OnSessionEnded(base::TimeDelta total_session_time) override {}

  PrefService* GetPrefs() { return pref_service_.get(); }

  SessionDurationUpdater* GetSessionDurationUpdater() {
    return &session_duration_updater_;
  }

  void SetBaseObservedSessionTime(base::TimeDelta base_value) {
    DictionaryPrefUpdate update(GetPrefs(), prefs::kObservedSessionTime);
    update->SetKey(kTestObservedSessionTimeKey,
                   base::Value(static_cast<double>(base_value.InSeconds())));
  }

 private:
  const std::unique_ptr<sync_preferences::TestingPrefServiceSyncable>
      pref_service_;
  SessionDurationUpdater session_duration_updater_;
  ScopedObserver<SessionDurationUpdater, SessionDurationUpdater::Observer>
      session_duration_observer_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class SessionDurationUpdaterTest : public testing::Test {
 public:
  SessionDurationUpdaterTest() = default;
  ~SessionDurationUpdaterTest() override = default;

  // testing::Test:
  void SetUp() override {
    // Start the DesktopSessionDurationTracker to track active session time.
    metrics::DesktopSessionDurationTracker::Initialize();

    test_observer_ = std::make_unique<TestObserver>();
    test_observer_->AddSessionDurationObserver();
  }

  void TearDown() override {
    test_observer_->RemoveSessionDurationObserver();
    metrics::DesktopSessionDurationTracker::CleanupForTesting();
  }

 protected:
  std::unique_ptr<TestObserver> test_observer_;

  DISALLOW_COPY_AND_ASSIGN(SessionDurationUpdaterTest);
};

}  // namespace

// kObservedSessionTime should be 0 on initalization and 50 after simulation.
TEST_F(SessionDurationUpdaterTest, TimeAdded) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(0, test_observer_->GetSessionDurationUpdater()
                   ->GetRecordedObservedSessionTime()
                   .InSeconds());

  // Tests 50 seconds passing with an observer added.
  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, test_observer_->GetSessionDurationUpdater()
                    ->GetRecordedObservedSessionTime()
                    .InSeconds());
}

// Observed session time should be equal to base value on initalization and 100
// after simulation.
TEST_F(SessionDurationUpdaterTest, TimeAddedWithBaseValue) {
  test_observer_->SetBaseObservedSessionTime(
      base::TimeDelta::FromSeconds(50.0));

  // Tests the pref is registered to 50 seconds before any session time passes.
  EXPECT_EQ(50, test_observer_->GetSessionDurationUpdater()
                    ->GetRecordedObservedSessionTime()
                    .InSeconds());

  // Tests 50 seconds passing with an observer added.
  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(100, test_observer_->GetSessionDurationUpdater()
                     ->GetRecordedObservedSessionTime()
                     .InSeconds());
}

// kObservedSessionTime should not be updated when SessionDurationUpdater has
// no observers, but should start updating again if another observer is added.
TEST_F(SessionDurationUpdaterTest, AddingAndRemovingObservers) {
  // Tests 50 seconds passing with an observer added.
  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, test_observer_->GetSessionDurationUpdater()
                    ->GetRecordedObservedSessionTime()
                    .InSeconds());

  // Tests 50 seconds passing without any observers. No time should be added to
  // the pref in this case.
  test_observer_->RemoveSessionDurationObserver();

  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, test_observer_->GetSessionDurationUpdater()
                    ->GetRecordedObservedSessionTime()
                    .InSeconds());
  // Tests 50 seconds passing with an observer re-added. Time should be added
  // again now.
  test_observer_->AddSessionDurationObserver();

  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(100, test_observer_->GetSessionDurationUpdater()
                     ->GetRecordedObservedSessionTime()
                     .InSeconds());
}

// Cumulative elapsed session time should be 0 on initalization and 50 after
// simulation.
TEST_F(SessionDurationUpdaterTest, GetCumulativeElapsedSessionTime) {
  // Tests the pref is registered to 0 before any session time passes.
  EXPECT_EQ(0, test_observer_->GetSessionDurationUpdater()
                   ->GetCumulativeElapsedSessionTime()
                   .InSeconds());

  // Tests 50 seconds passing with an observer added.
  test_observer_->GetSessionDurationUpdater()->OnSessionEnded(
      base::TimeDelta::FromSeconds(50));

  EXPECT_EQ(50, test_observer_->GetSessionDurationUpdater()
                    ->GetCumulativeElapsedSessionTime()
                    .InSeconds());
}

}  // namespace feature_engagement
