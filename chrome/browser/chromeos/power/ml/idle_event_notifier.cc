// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include "base/logging.h"
#include "base/time/default_clock.h"
#include "chrome/browser/chromeos/power/ml/real_boot_clock.h"
#include "chrome/browser/chromeos/power/ml/recent_events_counter.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace chromeos {
namespace power {
namespace ml {

using TimeSinceBoot = base::TimeDelta;

constexpr base::TimeDelta IdleEventNotifier::kIdleDelay;
constexpr base::TimeDelta IdleEventNotifier::kUserInputEventsDuration;
constexpr int IdleEventNotifier::kNumUserInputEventsBuckets;

struct IdleEventNotifier::ActivityDataInternal {
  // Use base::Time here because we later need to convert them to local time
  // since midnight.
  base::Time last_activity_time;
  // Zero if there's no user activity before idle event.
  base::Time last_user_activity_time;

  TimeSinceBoot last_activity_since_boot;
  base::Optional<TimeSinceBoot> earliest_activity_since_boot;
  base::Optional<TimeSinceBoot> last_key_since_boot;
  base::Optional<TimeSinceBoot> last_mouse_since_boot;
  base::Optional<TimeSinceBoot> last_touch_since_boot;
  base::Optional<TimeSinceBoot> video_start_time;
  base::Optional<TimeSinceBoot> video_end_time;
};

IdleEventNotifier::ActivityData::ActivityData() {}

IdleEventNotifier::ActivityData::ActivityData(const ActivityData& input_data) {
  last_activity_day = input_data.last_activity_day;
  last_activity_time_of_day = input_data.last_activity_time_of_day;
  last_user_activity_time_of_day = input_data.last_user_activity_time_of_day;
  recent_time_active = input_data.recent_time_active;
  time_since_last_key = input_data.time_since_last_key;
  time_since_last_mouse = input_data.time_since_last_mouse;
  time_since_last_touch = input_data.time_since_last_touch;
  video_playing_time = input_data.video_playing_time;
  time_since_video_ended = input_data.time_since_video_ended;
  key_events_in_last_hour = input_data.key_events_in_last_hour;
  mouse_events_in_last_hour = input_data.mouse_events_in_last_hour;
  touch_events_in_last_hour = input_data.touch_events_in_last_hour;
}

IdleEventNotifier::IdleEventNotifier(
    PowerManagerClient* power_manager_client,
    ui::UserActivityDetector* detector,
    viz::mojom::VideoDetectorObserverRequest request)
    : clock_(base::DefaultClock::GetInstance()),
      boot_clock_(std::make_unique<RealBootClock>()),
      power_manager_client_observer_(this),
      user_activity_observer_(this),
      internal_data_(std::make_unique<ActivityDataInternal>()),
      binding_(this, std::move(request)),
      key_counter_(
          std::make_unique<RecentEventsCounter>(kUserInputEventsDuration,
                                                kNumUserInputEventsBuckets)),
      mouse_counter_(
          std::make_unique<RecentEventsCounter>(kUserInputEventsDuration,
                                                kNumUserInputEventsBuckets)),
      touch_counter_(
          std::make_unique<RecentEventsCounter>(kUserInputEventsDuration,
                                                kNumUserInputEventsBuckets)) {
  DCHECK(power_manager_client);
  power_manager_client_observer_.Add(power_manager_client);
  DCHECK(detector);
  user_activity_observer_.Add(detector);
}

IdleEventNotifier::~IdleEventNotifier() = default;

void IdleEventNotifier::SetClockForTesting(
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    base::Clock* test_clock,
    std::unique_ptr<BootClock> test_boot_clock) {
  idle_delay_timer_.SetTaskRunner(task_runner);
  clock_ = test_clock;
  boot_clock_ = std::move(test_boot_clock);
}

void IdleEventNotifier::AddObserver(Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void IdleEventNotifier::RemoveObserver(Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void IdleEventNotifier::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& /* timestamp */) {
  // Ignore lid-close event, as we will observe suspend signal.
  if (state == chromeos::PowerManagerClient::LidState::OPEN) {
    UpdateActivityData(ActivityType::USER_OTHER);
    ResetIdleDelayTimer();
  }
}

void IdleEventNotifier::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  if (external_power_ != proto.external_power()) {
    external_power_ = proto.external_power();
    UpdateActivityData(ActivityType::USER_OTHER);
    ResetIdleDelayTimer();
  }
}

void IdleEventNotifier::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  if (idle_delay_timer_.IsRunning()) {
    // This means the user hasn't been idle for more than idle_delay. Regardless
    // of the reason of suspend, we won't be emitting any idle event now or
    // following SuspendDone. We stop the timer to prevent it from resuming in
    // SuspendDone.
    idle_delay_timer_.AbandonAndStop();
  }
}

void IdleEventNotifier::SuspendDone(const base::TimeDelta& sleep_duration) {
  if (idle_delay_timer_.IsRunning()) {
    idle_delay_timer_.AbandonAndStop();
  }

  // SuspendDone is triggered by user opening the lid (or other user
  // activities).
  if (sleep_duration >= kIdleDelay) {
    ResetTimestampsPerIdleEvent();
    if (video_playing_) {
      // This could happen when user closes the lid while video is playing.
      // If OnVideoActivityEnded is not received before system is suspended, we
      // could have |video_playing_| = true. If |sleep_duration| < kIdleDelay,
      // we consider video never stopped. Otherwise, we treat it as a new video
      // playing session.
      internal_data_->video_start_time = boot_clock_->GetTimeSinceBoot();
    }
  }

  UpdateActivityData(ActivityType::USER_OTHER);
  ResetIdleDelayTimer();
}

void IdleEventNotifier::OnUserActivity(const ui::Event* event) {
  if (!event)
    return;
  // Get the type of activity first then reset timer.
  ActivityType type = ActivityType::USER_OTHER;
  if (event->IsKeyEvent()) {
    type = ActivityType::KEY;
  } else if (event->IsMouseEvent()) {
    type = ActivityType::MOUSE;
  } else if (event->IsTouchEvent()) {
    type = ActivityType::TOUCH;
  }
  UpdateActivityData(type);
  ResetIdleDelayTimer();
}

void IdleEventNotifier::OnVideoActivityStarted() {
  if (video_playing_) {
    NOTREACHED() << "Duplicate start of video activity";
    return;
  }
  video_playing_ = true;
  UpdateActivityData(ActivityType::VIDEO);
}

void IdleEventNotifier::OnVideoActivityEnded() {
  if (!video_playing_) {
    NOTREACHED() << "Duplicate end of video activity";
    return;
  }
  video_playing_ = false;
  UpdateActivityData(ActivityType::VIDEO);
  ResetIdleDelayTimer();
}

IdleEventNotifier::ActivityData IdleEventNotifier::ConvertActivityData(
    const ActivityDataInternal& internal_data) const {
  const base::TimeDelta time_since_boot = boot_clock_->GetTimeSinceBoot();
  ActivityData data;

  base::Time::Exploded exploded;
  internal_data.last_activity_time.LocalExplode(&exploded);
  data.last_activity_day =
      static_cast<UserActivityEvent_Features_DayOfWeek>(exploded.day_of_week);

  data.last_activity_time_of_day =
      internal_data.last_activity_time -
      internal_data.last_activity_time.LocalMidnight();

  if (!internal_data.last_user_activity_time.is_null()) {
    data.last_user_activity_time_of_day =
        internal_data.last_user_activity_time -
        internal_data.last_user_activity_time.LocalMidnight();
  }

  if (internal_data.earliest_activity_since_boot) {
    data.recent_time_active =
        internal_data.last_activity_since_boot -
        internal_data.earliest_activity_since_boot.value();
  } else {
    data.recent_time_active = base::TimeDelta();
  }

  if (internal_data.last_key_since_boot) {
    data.time_since_last_key =
        time_since_boot - internal_data.last_key_since_boot.value();
  }

  if (internal_data.last_mouse_since_boot) {
    data.time_since_last_mouse =
        time_since_boot - internal_data.last_mouse_since_boot.value();
  }

  if (internal_data.last_touch_since_boot) {
    data.time_since_last_touch =
        time_since_boot - internal_data.last_touch_since_boot.value();
  }

  if (internal_data_->video_start_time && internal_data_->video_end_time) {
    DCHECK(!video_playing_);
    data.video_playing_time = internal_data_->video_end_time.value() -
                              internal_data_->video_start_time.value();
    data.time_since_video_ended =
        time_since_boot - internal_data_->video_end_time.value();
  }

  data.key_events_in_last_hour = key_counter_->GetTotal(time_since_boot);
  data.mouse_events_in_last_hour = mouse_counter_->GetTotal(time_since_boot);
  data.touch_events_in_last_hour = touch_counter_->GetTotal(time_since_boot);

  return data;
}

void IdleEventNotifier::ResetIdleDelayTimer() {
  // According to the policy, if there's video playing we should never dim or
  // turn off the screen (or transition to any other lower energy state).
  if (video_playing_)
    return;

  if (idle_delay_timer_.IsRunning()) {
    idle_delay_timer_.AbandonAndStop();
  }
  idle_delay_timer_.Start(FROM_HERE, kIdleDelay, this,
                          &IdleEventNotifier::OnIdleDelayTimeout);
}

void IdleEventNotifier::OnIdleDelayTimeout() {
  if (video_playing_)
    return;

  const ActivityData data = ConvertActivityData(*internal_data_);

  for (auto& observer : observers_)
    observer.OnIdleEventObserved(data);
  ResetTimestampsPerIdleEvent();
}

void IdleEventNotifier::UpdateActivityData(ActivityType type) {
  base::Time now = clock_->Now();
  DCHECK(internal_data_);
  internal_data_->last_activity_time = now;

  const base::TimeDelta time_since_boot = boot_clock_->GetTimeSinceBoot();

  internal_data_->last_activity_since_boot = time_since_boot;
  if (!internal_data_->earliest_activity_since_boot) {
    internal_data_->earliest_activity_since_boot = time_since_boot;
  }

  if (type == ActivityType::VIDEO) {
    if (video_playing_) {
      if (!internal_data_->video_start_time ||
          (internal_data_->video_end_time &&
           (time_since_boot - internal_data_->video_end_time.value() >=
            kIdleDelay))) {
        internal_data_->video_start_time = time_since_boot;
      }
    } else {
      internal_data_->video_end_time = time_since_boot;
    }
    return;
  }

  // All other activity is user-initiated.
  internal_data_->last_user_activity_time = now;

  switch (type) {
    case ActivityType::KEY:
      internal_data_->last_key_since_boot = time_since_boot;
      key_counter_->Log(time_since_boot);
      break;
    case ActivityType::MOUSE:
      internal_data_->last_mouse_since_boot = time_since_boot;
      mouse_counter_->Log(time_since_boot);
      break;
    case ActivityType::TOUCH:
      internal_data_->last_touch_since_boot = time_since_boot;
      touch_counter_->Log(time_since_boot);
      break;
    default:
      // We don't track other activity types.
      return;
  }
}

// Only clears out |last_activity_since_boot| and
// |earliest_activity_since_boot| because they are used to calculate recent
// time active, which should be reset between idle events.
void IdleEventNotifier::ResetTimestampsPerIdleEvent() {
  internal_data_->last_activity_since_boot = base::TimeDelta();
  internal_data_->earliest_activity_since_boot = base::nullopt;
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
