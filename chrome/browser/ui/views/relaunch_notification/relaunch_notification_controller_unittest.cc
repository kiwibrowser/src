// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/relaunch_notification/relaunch_notification_controller.h"

#include <memory>

#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// A delegate interface for handling the actions taken by the controller.
class ControllerDelegate {
 public:
  virtual ~ControllerDelegate() = default;
  virtual void ShowRelaunchRecommendedBubble() = 0;
  virtual void ShowRelaunchRequiredDialog() = 0;
  virtual void CloseWidget() = 0;
  virtual void OnRelaunchDeadlineExpired() = 0;

 protected:
  ControllerDelegate() = default;
};

// A fake controller that asks a delegate to do work.
class FakeRelaunchNotificationController
    : public RelaunchNotificationController {
 public:
  FakeRelaunchNotificationController(UpgradeDetector* upgrade_detector,
                                     const base::TickClock* tick_clock,
                                     ControllerDelegate* delegate)
      : RelaunchNotificationController(upgrade_detector, tick_clock),
        delegate_(delegate) {}

  using RelaunchNotificationController::kRelaunchGracePeriod;

 private:
  void ShowRelaunchRecommendedBubble() override {
    delegate_->ShowRelaunchRecommendedBubble();
  }

  void ShowRelaunchRequiredDialog() override {
    delegate_->ShowRelaunchRequiredDialog();
  }

  void CloseWidget() override { delegate_->CloseWidget(); }

  void OnRelaunchDeadlineExpired() override {
    delegate_->OnRelaunchDeadlineExpired();
  }

  ControllerDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(FakeRelaunchNotificationController);
};

// A mock delegate for testing.
class MockControllerDelegate : public ControllerDelegate {
 public:
  MOCK_METHOD0(ShowRelaunchRecommendedBubble, void());
  MOCK_METHOD0(ShowRelaunchRequiredDialog, void());
  MOCK_METHOD0(CloseWidget, void());
  MOCK_METHOD0(OnRelaunchDeadlineExpired, void());
};

// A fake UpgradeDetector.
class FakeUpgradeDetector : public UpgradeDetector {
 public:
  explicit FakeUpgradeDetector(const base::TickClock* tick_clock)
      : UpgradeDetector(tick_clock) {
    set_upgrade_detected_time(this->tick_clock()->NowTicks());
  }

  // UpgradeDetector:
  base::TimeDelta GetHighAnnoyanceLevelDelta() override {
    return high_threshold_ / 3;
  }

  base::TimeTicks GetHighAnnoyanceDeadline() override {
    return upgrade_detected_time() + high_threshold_;
  }

  // Sets the annoyance level to |level| and broadcasts the change to all
  // observers.
  void BroadcastLevelChange(UpgradeNotificationAnnoyanceLevel level) {
    set_upgrade_notification_stage(level);
    NotifyUpgrade();
  }

  // Sets the high annoyance threshold to |high_threshold| and broadcasts the
  // change to all observers.
  void BroadcastHighThresholdChange(base::TimeDelta high_threshold) {
    high_threshold_ = high_threshold;
    NotifyUpgrade();
  }

  base::TimeDelta high_threshold() const { return high_threshold_; }

 private:
  // UpgradeDetector:
  void OnRelaunchNotificationPeriodPrefChanged() override {}

  base::TimeDelta high_threshold_ = base::TimeDelta::FromDays(7);

  DISALLOW_COPY_AND_ASSIGN(FakeUpgradeDetector);
};

}  // namespace

// A test harness that provides facilities for manipulating the relaunch
// notification policy setting and for broadcasting upgrade notifications.
class RelaunchNotificationControllerTest : public ::testing::Test {
 protected:
  RelaunchNotificationControllerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::MOCK_TIME,
            base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED),
        scoped_local_state_(TestingBrowserProcess::GetGlobal()),
        upgrade_detector_(scoped_task_environment_.GetMockTickClock()) {}
  UpgradeDetector* upgrade_detector() { return &upgrade_detector_; }
  FakeUpgradeDetector& fake_upgrade_detector() { return upgrade_detector_; }

  // Sets the browser.relaunch_notification preference in Local State to
  // |value|.
  void SetNotificationPref(int value) {
    scoped_local_state_.Get()->SetManagedPref(
        prefs::kRelaunchNotification, std::make_unique<base::Value>(value));
  }

  // Returns the ScopedTaskEnvironment's MockTickClock.
  const base::TickClock* GetMockTickClock() {
    return scoped_task_environment_.GetMockTickClock();
  }

  // Fast-forwards virtual time by |delta|.
  void FastForwardBy(base::TimeDelta delta) {
    scoped_task_environment_.FastForwardBy(delta);
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ScopedTestingLocalState scoped_local_state_;
  FakeUpgradeDetector upgrade_detector_;

  DISALLOW_COPY_AND_ASSIGN(RelaunchNotificationControllerTest);
};

TEST_F(RelaunchNotificationControllerTest, CreateDestroy) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);
}

// Without the browser.relaunch_notification preference set, the controller
// should not be observing the UpgradeDetector, and should therefore never
// attempt to show any notifications.
TEST_F(RelaunchNotificationControllerTest, PolicyUnset) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
}

// With the browser.relaunch_notification preference set to 1, the controller
// should be observing the UpgradeDetector and should show "Requested"
// notifications on each level change.
TEST_F(RelaunchNotificationControllerTest, RecommendedByPolicy) {
  SetNotificationPref(1);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // Nothing shown if the level is broadcast at NONE.
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Show for each level change, but not for repeat notifications.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // The timer should be running to reshow at the detector's delta.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceLevelDelta());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceLevelDelta());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Drop back to elevated to stop the reshows and ensure there are none.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceLevelDelta());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And closed if the level drops back to none.
  EXPECT_CALL(mock_controller_delegate, CloseWidget());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// With the browser.relaunch_notification preference set to 2, the controller
// should be observing the UpgradeDetector and should show "Required"
// notifications on each level change.
TEST_F(RelaunchNotificationControllerTest, RequiredByPolicy) {
  SetNotificationPref(2);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // Nothing shown if the level is broadcast at NONE.
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Show for each level change, but not for repeat notifications.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And closed if the level drops back to none.
  EXPECT_CALL(mock_controller_delegate, CloseWidget());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_NONE);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// Flipping the policy should have no effect when at level NONE
TEST_F(RelaunchNotificationControllerTest, PolicyChangesNoUpgrade) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  SetNotificationPref(1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(3);  // Bogus value!
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(0);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// Policy changes at an elevated level should show the appropriate notification.
TEST_F(RelaunchNotificationControllerTest, PolicyChangesWithUpgrade) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  SetNotificationPref(1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, CloseWidget());
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  SetNotificationPref(2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  EXPECT_CALL(mock_controller_delegate, CloseWidget());
  SetNotificationPref(0);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// Relaunch is forced when the deadline is reached.
TEST_F(RelaunchNotificationControllerTest, RequiredDeadlineReached) {
  SetNotificationPref(2);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // As in the RequiredByPolicy test, the dialog should be shown.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And the relaunch should be forced after the deadline passes.
  EXPECT_CALL(mock_controller_delegate, OnRelaunchDeadlineExpired());
  FastForwardBy(fake_upgrade_detector().high_threshold() +
                FakeRelaunchNotificationController::kRelaunchGracePeriod);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// No forced relaunch if the dialog is closed.
TEST_F(RelaunchNotificationControllerTest, RequiredDeadlineReachedNoPolicy) {
  SetNotificationPref(2);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // As in the RequiredByPolicy test, the dialog should be shown.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And then closed if the policy is cleared.
  EXPECT_CALL(mock_controller_delegate, CloseWidget());
  SetNotificationPref(0);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And no relaunch should take place.
  FastForwardBy(fake_upgrade_detector().high_threshold() +
                FakeRelaunchNotificationController::kRelaunchGracePeriod);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// NotificationPeriod changes should do nothing at any policy setting when the
// annoyance level is at none.
TEST_F(RelaunchNotificationControllerTest, NonePeriodChange) {
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // Reduce the period.
  fake_upgrade_detector().BroadcastHighThresholdChange(
      base::TimeDelta::FromDays(1));
  FastForwardBy(fake_upgrade_detector().high_threshold());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(1);
  fake_upgrade_detector().BroadcastHighThresholdChange(
      base::TimeDelta::FromHours(23));
  FastForwardBy(fake_upgrade_detector().high_threshold());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  SetNotificationPref(2);
  fake_upgrade_detector().BroadcastHighThresholdChange(
      base::TimeDelta::FromHours(22));
  FastForwardBy(fake_upgrade_detector().high_threshold());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// NotificationPeriod changes impact reshows of the relaunch recommended bubble.
TEST_F(RelaunchNotificationControllerTest, PeriodChangeRecommended) {
  SetNotificationPref(1);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // Get up to high annoyance so that the reshow timer is running.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_HIGH);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Advance time partway to the reshow, but not all the way there.
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceLevelDelta() * 0.9);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Now shorten the period dramatically and expect an immediate reshow.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  fake_upgrade_detector().BroadcastHighThresholdChange(
      fake_upgrade_detector().high_threshold() / 10);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And expect another reshow at the new delta.
  base::TimeDelta short_reshow_delta =
      upgrade_detector()->GetHighAnnoyanceLevelDelta();
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  FastForwardBy(short_reshow_delta);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Now lengthen the period and expect no immediate reshow.
  fake_upgrade_detector().BroadcastHighThresholdChange(
      fake_upgrade_detector().high_threshold() * 10);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Move forward by the short delta to be sure there's no reshow there.
  FastForwardBy(short_reshow_delta);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Move forward the rest of the way to the new delta and expect a reshow.
  base::TimeDelta long_reshow_delta =
      upgrade_detector()->GetHighAnnoyanceLevelDelta();
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  FastForwardBy(long_reshow_delta - short_reshow_delta);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Similar to the above, move time forward a little bit.
  FastForwardBy(long_reshow_delta * 0.1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Shorten the period a bit, but not enough to force a reshow.
  fake_upgrade_detector().BroadcastHighThresholdChange(
      fake_upgrade_detector().high_threshold() * 0.9);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // And ensure that moving forward the rest of the way to the new delta causes
  // a reshow.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRecommendedBubble());
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceLevelDelta() -
                long_reshow_delta * 0.1);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}

// NotificationPeriod changes impact reshows of the relaunch required dialog.
TEST_F(RelaunchNotificationControllerTest, PeriodChangeRequired) {
  SetNotificationPref(2);
  ::testing::StrictMock<MockControllerDelegate> mock_controller_delegate;

  FakeRelaunchNotificationController controller(
      upgrade_detector(), GetMockTickClock(), &mock_controller_delegate);

  // Get up to low annoyance so that the relaunch timer is running.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_LOW);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Move forward partway to the current deadline. Nothing should happen.
  base::TimeTicks high_annoyance_deadline =
      upgrade_detector()->GetHighAnnoyanceDeadline();
  FastForwardBy((high_annoyance_deadline - GetMockTickClock()->NowTicks()) / 2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Lengthen the period, thereby pushing out the deadline.
  fake_upgrade_detector().BroadcastHighThresholdChange(
      fake_upgrade_detector().high_threshold() * 2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Ensure that nothing happens when the old deadline passes.
  FastForwardBy(high_annoyance_deadline +
                FakeRelaunchNotificationController::kRelaunchGracePeriod -
                GetMockTickClock()->NowTicks());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // But now we enter elevated annoyance level and show the dialog.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastLevelChange(
      UpgradeDetector::UPGRADE_ANNOYANCE_ELEVATED);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Jumping to the new deadline relaunches the browser.
  EXPECT_CALL(mock_controller_delegate, OnRelaunchDeadlineExpired());
  FastForwardBy(upgrade_detector()->GetHighAnnoyanceDeadline() +
                FakeRelaunchNotificationController::kRelaunchGracePeriod -
                GetMockTickClock()->NowTicks());
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);

  // Shorten the period, bringing in the deadline. Expect the dialog to show and
  // a relaunch after the grace period passes.
  EXPECT_CALL(mock_controller_delegate, ShowRelaunchRequiredDialog());
  fake_upgrade_detector().BroadcastHighThresholdChange(
      fake_upgrade_detector().high_threshold() / 2);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
  EXPECT_CALL(mock_controller_delegate, OnRelaunchDeadlineExpired());
  FastForwardBy(FakeRelaunchNotificationController::kRelaunchGracePeriod);
  ::testing::Mock::VerifyAndClear(&mock_controller_delegate);
}
