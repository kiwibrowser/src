// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include <memory>

#include "base/test/test_mock_time_task_runner.h"
#include "base/time/clock.h"
#include "chrome/browser/chromeos/power/ml/fake_boot_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace chromeos {
namespace power {
namespace ml {

namespace {

bool operator==(const IdleEventNotifier::ActivityData& x,
                const IdleEventNotifier::ActivityData& y) {
  return x.last_activity_day == y.last_activity_day &&
         x.last_activity_time_of_day == y.last_activity_time_of_day &&
         x.last_user_activity_time_of_day == y.last_user_activity_time_of_day &&
         x.recent_time_active == y.recent_time_active &&
         x.time_since_last_key == y.time_since_last_key &&
         x.time_since_last_mouse == y.time_since_last_mouse &&
         x.time_since_last_touch == y.time_since_last_touch &&
         x.video_playing_time == y.video_playing_time &&
         x.time_since_video_ended == y.time_since_video_ended &&
         x.key_events_in_last_hour == y.key_events_in_last_hour &&
         x.mouse_events_in_last_hour == y.mouse_events_in_last_hour &&
         x.touch_events_in_last_hour == y.touch_events_in_last_hour;
}

base::TimeDelta GetTimeSinceMidnight(base::Time time) {
  return time - time.LocalMidnight();
}

UserActivityEvent_Features_DayOfWeek GetDayOfWeek(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return static_cast<UserActivityEvent_Features_DayOfWeek>(
      exploded.day_of_week);
}

class TestObserver : public IdleEventNotifier::Observer {
 public:
  TestObserver() : idle_event_count_(0) {}
  ~TestObserver() override {}

  int idle_event_count() const { return idle_event_count_; }
  const IdleEventNotifier::ActivityData& activity_data() const {
    return activity_data_;
  }

  // IdleEventNotifier::Observer overrides:
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& activity_data) override {
    ++idle_event_count_;
    activity_data_ = activity_data;
  }

 private:
  int idle_event_count_;
  IdleEventNotifier::ActivityData activity_data_;
  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

class IdleEventNotifierTest : public testing::Test {
 public:
  IdleEventNotifierTest()
      : task_runner_(base::MakeRefCounted<base::TestMockTimeTaskRunner>(
            base::TestMockTimeTaskRunner::Type::kBoundToThread)),
        scoped_context_(task_runner_.get()) {
    viz::mojom::VideoDetectorObserverPtr observer;
    idle_event_notifier_ = std::make_unique<IdleEventNotifier>(
        &power_client_, &user_activity_detector_, mojo::MakeRequest(&observer));
    idle_event_notifier_->SetClockForTesting(
        task_runner_, task_runner_->GetMockClock(),
        std::make_unique<FakeBootClock>(task_runner_,
                                        base::TimeDelta::FromSeconds(10)));
    idle_event_notifier_->AddObserver(&test_observer_);
    ac_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_AC);
    disconnected_power_.set_external_power(
        power_manager::PowerSupplyProperties_ExternalPower_DISCONNECTED);
  }

  ~IdleEventNotifierTest() override {
    idle_event_notifier_.reset();
  }

 protected:
  void FastForwardAndCheckResults(
      int expected_idle_count,
      const IdleEventNotifier::ActivityData& expected_activity_data) {
    task_runner_->FastForwardUntilNoTasksRemain();
    EXPECT_EQ(expected_idle_count, test_observer_.idle_event_count());
    EXPECT_TRUE(expected_activity_data == test_observer_.activity_data());
  }

  const scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  const base::TestMockTimeTaskRunner::ScopedContext scoped_context_;

  TestObserver test_observer_;
  std::unique_ptr<IdleEventNotifier> idle_event_notifier_;
  power_manager::PowerSupplyProperties ac_power_;
  power_manager::PowerSupplyProperties disconnected_power_;
  FakePowerManagerClient power_client_;
  ui::UserActivityDetector user_activity_detector_;

 private:
  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifierTest);
};

// After initialization, |idle_delay_timer_| should not be running and
// |external_power_| is not set up.
TEST_F(IdleEventNotifierTest, CheckInitialValues) {
  EXPECT_FALSE(idle_event_notifier_->idle_delay_timer_.IsRunning());
  EXPECT_FALSE(idle_event_notifier_->external_power_);
}

// After lid is opened, an idle event will be fired following an idle period.
TEST_F(IdleEventNotifierTest, LidOpenEventReceived) {
  base::Time now = task_runner_->Now();
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN,
      base::TimeTicks::UnixEpoch());
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(1, data);
}

// Lid-closed event will not trigger any future idle event to be generated.
TEST_F(IdleEventNotifierTest, LidClosedEventReceived) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::CLOSED,
      base::TimeTicks::UnixEpoch());
  const IdleEventNotifier::ActivityData data;
  FastForwardAndCheckResults(0, data);
}

// Initially power source is unset, hence the 1st time power change signal is
// detected, it'll be treated as a user activity and an idle event will be fired
// following an idle period.
TEST_F(IdleEventNotifierTest, PowerChangedFirstSet) {
  base::Time now = task_runner_->Now();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(1, data);
}

// PowerChanged signal is received but source isn't changed, so it won't trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceNotChanged) {
  base::Time now = task_runner_->Now();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(1, data);
  idle_event_notifier_->PowerChanged(ac_power_);
  FastForwardAndCheckResults(1, data);
}

// PowerChanged signal is received and source is changed, so it will trigger
// a future idle event.
TEST_F(IdleEventNotifierTest, PowerSourceChanged) {
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->PowerChanged(ac_power_);
  IdleEventNotifier::ActivityData data_1;
  data_1.last_activity_day = GetDayOfWeek(now_1);
  const base::TimeDelta time_of_day_1 = GetTimeSinceMidnight(now_1);
  data_1.last_activity_time_of_day = time_of_day_1;
  data_1.last_user_activity_time_of_day = time_of_day_1;
  data_1.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(1, data_1);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(100));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->PowerChanged(disconnected_power_);
  IdleEventNotifier::ActivityData data_2;
  data_2.last_activity_day = GetDayOfWeek(now_2);
  const base::TimeDelta time_of_day_2 = GetTimeSinceMidnight(now_2);
  data_2.last_activity_time_of_day = time_of_day_2;
  data_2.last_user_activity_time_of_day = time_of_day_2;
  data_2.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(2, data_2);
}

// SuspendImminent will not trigger any future idle event.
TEST_F(IdleEventNotifierTest, SuspendImminent) {
  idle_event_notifier_->LidEventReceived(
      chromeos::PowerManagerClient::LidState::OPEN,
      base::TimeTicks::UnixEpoch());
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);
  const IdleEventNotifier::ActivityData data;
  FastForwardAndCheckResults(0, data);
}

// Short sleep duration behaves as if there was no idle period.
TEST_F(IdleEventNotifierTest, ShortSuspendDone) {
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->PowerChanged(ac_power_);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);

  task_runner_->FastForwardBy(IdleEventNotifier::kIdleDelay / 2);
  idle_event_notifier_->SuspendDone(IdleEventNotifier::kIdleDelay / 2);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(5));
  idle_event_notifier_->PowerChanged(disconnected_power_);
  base::Time now_2 = task_runner_->Now();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_2);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now_2);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = now_2 - now_1;
  FastForwardAndCheckResults(1, data);
}

// Long sleep duration behaves as if there was an idle period.
TEST_F(IdleEventNotifierTest, LongSuspendDone) {
  idle_event_notifier_->PowerChanged(ac_power_);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);

  task_runner_->FastForwardBy(IdleEventNotifier::kIdleDelay +
                              base::TimeDelta::FromSeconds(10));
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->SuspendDone(IdleEventNotifier::kIdleDelay +
                                    base::TimeDelta::FromSeconds(10));

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->PowerChanged(disconnected_power_);

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_2);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now_2);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = now_2 - now_1;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, UserActivityKey) {
  base::Time now = task_runner_->Now();
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  idle_event_notifier_->OnUserActivity(&key_event);
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  data.time_since_last_key = IdleEventNotifier::kIdleDelay;
  data.key_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, UserActivityMouse) {
  base::Time now = task_runner_->Now();
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);

  idle_event_notifier_->OnUserActivity(&mouse_event);
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  data.time_since_last_mouse = IdleEventNotifier::kIdleDelay;
  data.mouse_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, UserActivityOther) {
  base::Time now = task_runner_->Now();
  ui::GestureEvent gesture_event(0, 0, 0, base::TimeTicks(),
                                 ui::GestureEventDetails(ui::ET_GESTURE_TAP));

  idle_event_notifier_->OnUserActivity(&gesture_event);
  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = base::TimeDelta();
  FastForwardAndCheckResults(1, data);
}

// Two consecutive activities separated by 2sec only. Only 1 idle event with
// different timestamps for the two activities.
TEST_F(IdleEventNotifierTest, TwoQuickUserActivities) {
  base::Time now_1 = task_runner_->Now();
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  base::Time now_2 = task_runner_->Now();
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  idle_event_notifier_->OnUserActivity(&key_event);

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_2);
  const base::TimeDelta time_of_day = GetTimeSinceMidnight(now_2);
  data.last_activity_time_of_day = time_of_day;
  data.last_user_activity_time_of_day = time_of_day;
  data.recent_time_active = now_2 - now_1;
  data.time_since_last_key = IdleEventNotifier::kIdleDelay;
  data.time_since_last_mouse = IdleEventNotifier::kIdleDelay + (now_2 - now_1);
  data.key_events_in_last_hour = 1;
  data.mouse_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, ActivityWhileVideoPlaying) {
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  idle_event_notifier_->OnVideoActivityStarted();
  idle_event_notifier_->OnUserActivity(&mouse_event);
  const IdleEventNotifier::ActivityData data;
  FastForwardAndCheckResults(0, data);
}

// Activity is observed when video isn't playing, it will start idle timer. But
// before timer expires, video starts playing. So when timer expires, no idle
// event will be generated.
TEST_F(IdleEventNotifierTest, ActivityBeforeVideoStarts) {
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  idle_event_notifier_->OnUserActivity(&mouse_event);
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  idle_event_notifier_->OnVideoActivityStarted();
  const IdleEventNotifier::ActivityData data;
  FastForwardAndCheckResults(0, data);
}

// An activity is observed when video is playing. No idle timer will be
// triggered but last user activity time will be recorded. Video then stops
// playing that will trigger timer to start.
TEST_F(IdleEventNotifierTest, ActivityAfterVideoStarts) {
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(1));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  base::Time now_3 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_3);
  data.last_activity_time_of_day = GetTimeSinceMidnight(now_3);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(now_2);
  data.recent_time_active = now_3 - now_1;
  data.time_since_last_mouse = IdleEventNotifier::kIdleDelay + now_3 - now_2;
  data.video_playing_time = now_3 - now_1;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  data.mouse_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, IdleEventFieldReset) {
  base::Time now_1 = task_runner_->Now();
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  idle_event_notifier_->OnUserActivity(&key_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time now_2 = task_runner_->Now();
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  idle_event_notifier_->OnUserActivity(&mouse_event);

  IdleEventNotifier::ActivityData data_1;
  data_1.last_activity_day = GetDayOfWeek(now_2);
  const base::TimeDelta time_of_day_2 = GetTimeSinceMidnight(now_2);
  data_1.last_activity_time_of_day = time_of_day_2;
  data_1.last_user_activity_time_of_day = time_of_day_2;
  data_1.recent_time_active = now_2 - now_1;
  data_1.time_since_last_key = IdleEventNotifier::kIdleDelay + now_2 - now_1;
  data_1.time_since_last_mouse = IdleEventNotifier::kIdleDelay;
  data_1.key_events_in_last_hour = 1;
  data_1.mouse_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data_1);

  idle_event_notifier_->PowerChanged(ac_power_);
  base::Time now_3 = task_runner_->Now();

  IdleEventNotifier::ActivityData data_2;
  data_2.last_activity_day = GetDayOfWeek(now_3);
  const base::TimeDelta time_of_day_3 = GetTimeSinceMidnight(now_3);
  data_2.last_activity_time_of_day = time_of_day_3;
  data_2.last_user_activity_time_of_day = time_of_day_3;
  data_2.recent_time_active = base::TimeDelta();
  data_2.time_since_last_key = IdleEventNotifier::kIdleDelay + now_3 - now_1;
  data_2.time_since_last_mouse = IdleEventNotifier::kIdleDelay + now_3 - now_2;
  data_2.key_events_in_last_hour = 1;
  data_2.mouse_events_in_last_hour = 1;
  FastForwardAndCheckResults(2, data_2);
}

TEST_F(IdleEventNotifierTest, TwoConsecutiveVideoPlaying) {
  // Two video playing sessions with a gap shorter than kIdleDelay. They are
  // merged into one playing session.
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  idle_event_notifier_->OnVideoActivityEnded();

  task_runner_->FastForwardBy(IdleEventNotifier::kIdleDelay / 2);
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time now_3 = task_runner_->Now();
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  idle_event_notifier_->OnUserActivity(&mouse_event);

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_3);
  data.last_activity_time_of_day = GetTimeSinceMidnight(now_3);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(now_3);
  data.recent_time_active = now_3 - now_1;
  data.time_since_last_mouse = IdleEventNotifier::kIdleDelay;
  data.mouse_events_in_last_hour = 1;
  data.video_playing_time = now_2 - now_1;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay + now_3 - now_2;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, TwoVideoPlayingFarApartOneIdleEvent) {
  // Two video playing sessions with a gap larger than kIdleDelay. No idle event
  // is generated in between because of other user activities, and the last
  // video session is reported.
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  idle_event_notifier_->OnVideoActivityEnded();

  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  idle_event_notifier_->OnUserActivity(&mouse_event);
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  idle_event_notifier_->OnUserActivity(&mouse_event);

  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  base::Time now_3 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_3);
  data.last_activity_time_of_day = GetTimeSinceMidnight(now_3);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(now_2);
  data.recent_time_active = now_3 - now_1;
  data.time_since_last_mouse = IdleEventNotifier::kIdleDelay + now_3 - now_2;
  data.mouse_events_in_last_hour = 2;
  data.video_playing_time = now_3 - now_2;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, TwoVideoPlayingFarApartTwoIdleEvents) {
  // Two video playing sessions with a gap equal to kIdleDelay. An idle event
  // is generated in between, both video sessions are reported.
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data_1;
  data_1.last_activity_day = GetDayOfWeek(now_2);
  data_1.last_activity_time_of_day = GetTimeSinceMidnight(now_2);
  data_1.recent_time_active = now_2 - now_1;
  data_1.video_playing_time = now_2 - now_1;
  data_1.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  FastForwardAndCheckResults(1, data_1);

  base::Time now_3 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  base::Time now_4 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data_2;
  data_2.last_activity_day = GetDayOfWeek(now_4);
  data_2.last_activity_time_of_day = GetTimeSinceMidnight(now_4);
  data_2.recent_time_active = now_4 - now_3;
  data_2.video_playing_time = now_4 - now_3;
  data_2.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  FastForwardAndCheckResults(2, data_2);
}

TEST_F(IdleEventNotifierTest, VideoPlayingPausedByShortSuspend) {
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);

  task_runner_->FastForwardBy(IdleEventNotifier::kIdleDelay / 2);
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->SuspendDone(IdleEventNotifier::kIdleDelay / 2);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  base::Time now_3 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_3);
  data.last_activity_time_of_day = GetTimeSinceMidnight(now_3);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(now_2);
  data.recent_time_active = now_3 - now_1;
  data.video_playing_time = now_3 - now_1;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, VideoPlayingPausedByLongSuspend) {
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  idle_event_notifier_->SuspendImminent(
      power_manager::SuspendImminent_Reason_LID_CLOSED);

  task_runner_->FastForwardBy(2 * IdleEventNotifier::kIdleDelay);
  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->SuspendDone(2 * IdleEventNotifier::kIdleDelay);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(20));
  base::Time now_2 = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(now_2);
  data.last_activity_time_of_day = GetTimeSinceMidnight(now_2);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(now_1);
  data.recent_time_active = now_2 - now_1;
  data.video_playing_time = now_2 - now_1;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, UserInputEventsOneIdleEvent) {
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  ui::TouchEvent touch_event(
      ui::ET_TOUCH_PRESSED, gfx::Point(0, 0), base::TimeTicks::UnixEpoch(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));

  base::Time first_activity_time = task_runner_->Now();
  // This key event will be too old to be counted.
  idle_event_notifier_->OnUserActivity(&key_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time video_start_time = task_runner_->Now();
  // Keep playing video so we won't run into an idle event.
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_key_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&key_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_touch_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_mouse_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time video_end_time = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data;
  data.last_activity_day = GetDayOfWeek(video_end_time);
  data.last_activity_time_of_day = GetTimeSinceMidnight(video_end_time);
  data.last_user_activity_time_of_day = GetTimeSinceMidnight(last_mouse_time);
  data.recent_time_active = video_end_time - first_activity_time;
  data.video_playing_time = video_end_time - video_start_time;
  data.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  data.time_since_last_key =
      IdleEventNotifier::kIdleDelay + video_end_time - last_key_time;
  data.time_since_last_mouse =
      IdleEventNotifier::kIdleDelay + video_end_time - last_mouse_time;
  data.time_since_last_touch =
      IdleEventNotifier::kIdleDelay + video_end_time - last_touch_time;

  data.key_events_in_last_hour = 1;
  data.mouse_events_in_last_hour = 2;
  data.touch_events_in_last_hour = 3;

  FastForwardAndCheckResults(1, data);
}

TEST_F(IdleEventNotifierTest, UserInputEventsTwoIdleEvents) {
  ui::KeyEvent key_event(ui::ET_KEY_PRESSED, ui::VKEY_A, ui::EF_NONE);
  ui::MouseEvent mouse_event(ui::ET_MOUSE_EXITED, gfx::Point(0, 0),
                             gfx::Point(0, 0), base::TimeTicks(), 0, 0);
  ui::TouchEvent touch_event(
      ui::ET_TOUCH_PRESSED, gfx::Point(0, 0), base::TimeTicks::UnixEpoch(),
      ui::PointerDetails(ui::EventPointerType::POINTER_TYPE_TOUCH, 0));

  base::Time now_1 = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&key_event);

  IdleEventNotifier::ActivityData data_1;
  data_1.last_activity_day = GetDayOfWeek(now_1);
  data_1.last_activity_time_of_day = GetTimeSinceMidnight(now_1);
  data_1.last_user_activity_time_of_day = GetTimeSinceMidnight(now_1);
  data_1.recent_time_active = base::TimeDelta();
  data_1.time_since_last_key = IdleEventNotifier::kIdleDelay;
  data_1.key_events_in_last_hour = 1;
  FastForwardAndCheckResults(1, data_1);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_key_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&key_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time video_start_time = task_runner_->Now();
  // Keep playing video so we won't run into an idle event.
  idle_event_notifier_->OnVideoActivityStarted();

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_touch_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&touch_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromMinutes(11));
  base::Time last_mouse_time = task_runner_->Now();
  idle_event_notifier_->OnUserActivity(&mouse_event);

  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(10));
  base::Time video_end_time = task_runner_->Now();
  idle_event_notifier_->OnVideoActivityEnded();

  IdleEventNotifier::ActivityData data_2;
  data_2.last_activity_day = GetDayOfWeek(video_end_time);
  data_2.last_activity_time_of_day = GetTimeSinceMidnight(video_end_time);
  data_2.last_user_activity_time_of_day = GetTimeSinceMidnight(last_mouse_time);
  data_2.recent_time_active = video_end_time - last_key_time;
  data_2.video_playing_time = video_end_time - video_start_time;
  data_2.time_since_video_ended = IdleEventNotifier::kIdleDelay;
  data_2.time_since_last_key =
      IdleEventNotifier::kIdleDelay + video_end_time - last_key_time;
  data_2.time_since_last_mouse =
      IdleEventNotifier::kIdleDelay + video_end_time - last_mouse_time;
  data_2.time_since_last_touch =
      IdleEventNotifier::kIdleDelay + video_end_time - last_touch_time;

  data_2.key_events_in_last_hour = 1;
  data_2.mouse_events_in_last_hour = 2;
  data_2.touch_events_in_last_hour = 3;

  FastForwardAndCheckResults(2, data_2);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
