// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/upgrade_detector_impl.h"

#include <initializer_list>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/tick_clock.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/upgrade_observer.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_WIN)
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/test/scoped_install_details.h"
#endif  // defined(OS_WIN)

namespace {

class TestUpgradeDetectorImpl : public UpgradeDetectorImpl {
 public:
  explicit TestUpgradeDetectorImpl(const base::TickClock* tick_clock)
      : UpgradeDetectorImpl(tick_clock) {}
  ~TestUpgradeDetectorImpl() override = default;

  // Exposed for testing.
  using UpgradeDetectorImpl::UPGRADE_AVAILABLE_REGULAR;
  using UpgradeDetectorImpl::UpgradeDetected;
  using UpgradeDetectorImpl::OnExperimentChangesDetected;
  using UpgradeDetectorImpl::NotifyOnUpgradeWithTimePassed;
  using UpgradeDetectorImpl::GetThresholdForLevel;
  using UpgradeDetectorImpl::tick_clock;

  // UpgradeDetector:
  void TriggerCriticalUpdate() override {
    ++trigger_critical_update_call_count_;
  }

  int trigger_critical_update_call_count() const {
    return trigger_critical_update_call_count_;
  }

 private:
  // How many times TriggerCriticalUpdate() has been called. Expected to either
  // be 0 or 1.
  int trigger_critical_update_call_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestUpgradeDetectorImpl);
};

class TestUpgradeNotificationListener : public UpgradeObserver {
 public:
  explicit TestUpgradeNotificationListener(UpgradeDetector* detector)
      : notifications_count_(0), detector_(detector) {
    DCHECK(detector_);
    detector_->AddObserver(this);
  }
  ~TestUpgradeNotificationListener() override {
    detector_->RemoveObserver(this);
  }

  int notification_count() const { return notifications_count_; }

 private:
  // UpgradeObserver:
  void OnUpgradeRecommended() override { ++notifications_count_; }

  // The number of upgrade recommended notifications received.
  int notifications_count_;

  UpgradeDetector* detector_;

  DISALLOW_COPY_AND_ASSIGN(TestUpgradeNotificationListener);
};

class MockUpgradeObserver : public UpgradeObserver {
 public:
  explicit MockUpgradeObserver(UpgradeDetector* upgrade_detector)
      : upgrade_detector_(upgrade_detector) {
    upgrade_detector_->AddObserver(this);
  }
  ~MockUpgradeObserver() override { upgrade_detector_->RemoveObserver(this); }
  MOCK_METHOD0(OnUpdateOverCellularAvailable, void());
  MOCK_METHOD0(OnUpdateOverCellularOneTimePermissionGranted, void());
  MOCK_METHOD0(OnUpgradeRecommended, void());
  MOCK_METHOD0(OnCriticalUpgradeInstalled, void());
  MOCK_METHOD0(OnOutdatedInstall, void());
  MOCK_METHOD0(OnOutdatedInstallNoAutoUpdate, void());

 private:
  UpgradeDetector* const upgrade_detector_;
  DISALLOW_COPY_AND_ASSIGN(MockUpgradeObserver);
};

}  // namespace

class UpgradeDetectorImplTest : public ::testing::Test {
 protected:
  UpgradeDetectorImplTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME),
        scoped_local_state_(TestingBrowserProcess::GetGlobal()) {
    // Disable the detector's check to see if autoupdates are inabled.
    // Without this, tests put the detector into an invalid state by detecting
    // upgrades before the detection task completes.
    scoped_local_state_.Get()->SetUserPref(prefs::kAttemptedToEnableAutoupdate,
                                           std::make_unique<base::Value>(true));
  }

  const base::TickClock* GetMockTickClock() {
    return scoped_task_environment_.GetMockTickClock();
  }

  // Sets the browser.relaunch_notification_period preference in Local State to
  // |value|.
  void SetNotificationPeriodPref(base::TimeDelta value) {
    if (value.is_zero()) {
      scoped_local_state_.Get()->RemoveManagedPref(
          prefs::kRelaunchNotificationPeriod);
    } else {
      scoped_local_state_.Get()->SetManagedPref(
          prefs::kRelaunchNotificationPeriod,
          std::make_unique<base::Value>(
              base::saturated_cast<int>(value.InMilliseconds())));
    }
  }

  void RunUntilIdle() { scoped_task_environment_.RunUntilIdle(); }

  // Fast-forwards virtual time by |delta|.
  void FastForwardBy(base::TimeDelta delta) {
    scoped_task_environment_.FastForwardBy(delta);
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ScopedTestingLocalState scoped_local_state_;

  DISALLOW_COPY_AND_ASSIGN(UpgradeDetectorImplTest);
};

TEST_F(UpgradeDetectorImplTest, VariationsChanges) {
  TestUpgradeDetectorImpl detector(GetMockTickClock());
  TestUpgradeNotificationListener notifications_listener(&detector);
  EXPECT_FALSE(detector.notify_upgrade());
  EXPECT_EQ(0, notifications_listener.notification_count());

  detector.OnExperimentChangesDetected(
      variations::VariationsService::Observer::BEST_EFFORT);
  EXPECT_FALSE(detector.notify_upgrade());
  EXPECT_EQ(0, notifications_listener.notification_count());

  detector.NotifyOnUpgradeWithTimePassed(base::TimeDelta::FromDays(30));
  EXPECT_TRUE(detector.notify_upgrade());
  EXPECT_EQ(1, notifications_listener.notification_count());
  EXPECT_EQ(0, detector.trigger_critical_update_call_count());

  // Execute tasks posted by |detector| referencing it while it's still in
  // scope.
  RunUntilIdle();
}

TEST_F(UpgradeDetectorImplTest, VariationsCriticalChanges) {
  TestUpgradeDetectorImpl detector(GetMockTickClock());
  TestUpgradeNotificationListener notifications_listener(&detector);
  EXPECT_FALSE(detector.notify_upgrade());
  EXPECT_EQ(0, notifications_listener.notification_count());

  detector.OnExperimentChangesDetected(
      variations::VariationsService::Observer::CRITICAL);
  EXPECT_FALSE(detector.notify_upgrade());
  EXPECT_EQ(0, notifications_listener.notification_count());

  detector.NotifyOnUpgradeWithTimePassed(base::TimeDelta::FromDays(30));
  EXPECT_TRUE(detector.notify_upgrade());
  EXPECT_EQ(1, notifications_listener.notification_count());
  EXPECT_EQ(1, detector.trigger_critical_update_call_count());

  // Execute tasks posted by |detector| referencing it while it's still in
  // scope.
  RunUntilIdle();
}

TEST_F(UpgradeDetectorImplTest, TestPeriodChanges) {
  // Fast forward a little bit to get away from zero ticks, which has special
  // meaning in the detector.
  FastForwardBy(base::TimeDelta::FromHours(1));

  TestUpgradeDetectorImpl upgrade_detector(GetMockTickClock());
  ::testing::StrictMock<MockUpgradeObserver> mock_observer(&upgrade_detector);

  // Changing the period when no upgrade has been detected updates the
  // thresholds and nothing else.
  SetNotificationPeriodPref(base::TimeDelta::FromHours(3));

  EXPECT_EQ(upgrade_detector.GetThresholdForLevel(
                UpgradeDetector::UPGRADE_ANNOYANCE_HIGH),
            base::TimeDelta::FromHours(3));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Back to default.
  SetNotificationPeriodPref(base::TimeDelta());
  EXPECT_EQ(upgrade_detector.GetThresholdForLevel(
                UpgradeDetector::UPGRADE_ANNOYANCE_HIGH),
            base::TimeDelta::FromDays(7));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Pretend that an upgrade was just detected now.
  upgrade_detector.UpgradeDetected(
      TestUpgradeDetectorImpl::UPGRADE_AVAILABLE_REGULAR);
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Fast forward an amount that is still in the "don't annoy me" period at the
  // default period.
  FastForwardBy(base::TimeDelta::FromHours(1));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Drop the period so that the current time is in the "low" annoyance level.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta::FromHours(3));
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_LOW);

  // Bring it back up.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta());
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_NONE);

  // Fast forward an amount that is still in the "don't annoy me" period at the
  // default period.
  FastForwardBy(base::TimeDelta::FromHours(1));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Drop the period so that the current time is in the "elevated" level.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta::FromHours(3));
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);

  // Bring it back up.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta());
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_NONE);

  // Fast forward an amount that is still in the "don't annoy me" period at the
  // default period.
  FastForwardBy(base::TimeDelta::FromHours(1));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Drop the period so that the current time is in the "high" level.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta::FromHours(3));
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);

  // Expect no new notifications even if some time passes.
  FastForwardBy(base::TimeDelta::FromHours(1));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Bring the period back up.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta());
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_NONE);

  // Fast forward an amount that is still in the "don't annoy me" period at the
  // default period.
  FastForwardBy(base::TimeDelta::FromHours(1));
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Drop the period so that the current time is deep in the "high" level.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta::FromHours(3));
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);

  // Bring it back up.
  EXPECT_CALL(mock_observer, OnUpgradeRecommended());
  SetNotificationPeriodPref(base::TimeDelta());
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_EQ(upgrade_detector.upgrade_notification_stage(),
            UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
}

#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
// Tests that the low threshold for unstable channels is less than that for
// stable channels.
TEST_F(UpgradeDetectorImplTest, TestUnstableChannelLowThreshold) {
  // Grab the low threshold for stable channel.
  base::TimeDelta default_low_threshold;
  {
    TestUpgradeDetectorImpl upgrade_detector(GetMockTickClock());
    default_low_threshold = upgrade_detector.GetThresholdForLevel(
        UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  }

  // Now make sure that the low threshold for canary is smaller.
  install_static::ScopedInstallDetails install_details(
      false, install_static::CANARY_INDEX);

  TestUpgradeDetectorImpl upgrade_detector(GetMockTickClock());
  EXPECT_LT(upgrade_detector.GetThresholdForLevel(
                UpgradeDetector::UPGRADE_ANNOYANCE_LOW),
            default_low_threshold);
}
#endif

// Appends the time and stage from detector to |notifications|.
ACTION_P2(AppendTicksAndStage, detector, notifications) {
  notifications->emplace_back(detector->tick_clock()->NowTicks(),
                              detector->upgrade_notification_stage());
}

// A value parameterized test fixture for running tests with different
// RelaunchNotificationPeriod settings.
class UpgradeDetectorImplTimerTest : public UpgradeDetectorImplTest,
                                     public ::testing::WithParamInterface<int> {
 protected:
  UpgradeDetectorImplTimerTest() {
    const int period_ms = GetParam();
    if (period_ms)
      SetNotificationPeriodPref(base::TimeDelta::FromMilliseconds(period_ms));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UpgradeDetectorImplTimerTest);
};

INSTANTIATE_TEST_CASE_P(,
                        UpgradeDetectorImplTimerTest,
                        ::testing::Values(0,           // Default period of 7d.
                                          11100000));  // 3:05:00.

// Tests that the notification timer is handled as desired.
TEST_P(UpgradeDetectorImplTimerTest, TestNotificationTimer) {
  using TimeAndStage =
      std::pair<base::TimeTicks,
                UpgradeDetector::UpgradeNotificationAnnoyanceLevel>;
  using Notifications = std::vector<TimeAndStage>;

  // Fast forward a little bit to get away from zero ticks, which has special
  // meaning in the detector.
  FastForwardBy(base::TimeDelta::FromHours(1));

  TestUpgradeDetectorImpl detector(GetMockTickClock());
  ::testing::StrictMock<MockUpgradeObserver> mock_observer(&detector);

  // Cache the thresholds for the detector's annoyance levels.
  const base::TimeDelta thresholds[3] = {
      detector.GetThresholdForLevel(UpgradeDetector::UPGRADE_ANNOYANCE_LOW),
      detector.GetThresholdForLevel(
          UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED),
      detector.GetThresholdForLevel(UpgradeDetector::UPGRADE_ANNOYANCE_HIGH),
  };

  // Pretend that there's an update.
  detector.UpgradeDetected(TestUpgradeDetectorImpl::UPGRADE_AVAILABLE_REGULAR);
  ::testing::Mock::VerifyAndClear(&mock_observer);

  // Fast foward to the time that low annoyance should be reached. One
  // notification should come in at exactly the low annoyance threshold.
  Notifications notifications;
  EXPECT_CALL(mock_observer, OnUpgradeRecommended())
      .WillOnce(AppendTicksAndStage(&detector, &notifications));
  FastForwardBy(thresholds[0]);
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_THAT(notifications,
              ::testing::ContainerEq(Notifications({TimeAndStage(
                  detector.upgrade_detected_time() + thresholds[0],
                  UpgradeDetector::UPGRADE_ANNOYANCE_LOW)})));

  // Move to the time that elevated annoyance should be reached. Notifications
  // at low annoyance should arrive every 20 minutes with one final notification
  // at elevated annoyance.
  notifications.clear();
  EXPECT_CALL(mock_observer, OnUpgradeRecommended())
      .WillRepeatedly(AppendTicksAndStage(&detector, &notifications));
  FastForwardBy(thresholds[1] - thresholds[0]);
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_THAT(notifications.size(), ::testing::Gt(1U));
  EXPECT_THAT((notifications.end() - 2)->second,
              ::testing::Eq(UpgradeDetector::UPGRADE_ANNOYANCE_LOW));
  EXPECT_THAT(notifications.back(),
              ::testing::Eq(
                  TimeAndStage(detector.upgrade_detected_time() + thresholds[1],
                               UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED)));

  // Move to the time that high annoyance should be reached. Notifications at
  // elevated annoyance should arrive every 20 minutes with one final
  // notification at high annoyance.
  notifications.clear();
  EXPECT_CALL(mock_observer, OnUpgradeRecommended())
      .WillRepeatedly(AppendTicksAndStage(&detector, &notifications));
  FastForwardBy(thresholds[2] - thresholds[1]);
  ::testing::Mock::VerifyAndClear(&mock_observer);
  EXPECT_THAT(notifications.size(), ::testing::Gt(1U));
  EXPECT_THAT((notifications.end() - 2)->second,
              ::testing::Eq(UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED));
  EXPECT_THAT(notifications.back(),
              ::testing::Eq(
                  TimeAndStage(detector.upgrade_detected_time() + thresholds[2],
                               UpgradeDetector::UPGRADE_ANNOYANCE_HIGH)));

  // No new notifications after high annoyance has been reached.
  FastForwardBy(thresholds[2]);
  ::testing::Mock::VerifyAndClear(&mock_observer);
}
