// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace base {
class Clock;
}

namespace chromeos {
namespace power {
namespace ml {

class BootClock;
class RecentEventsCounter;

// This is time since midnight in the local time zone and may move back or
// forward when DST starts or stops.
using TimeOfDay = base::TimeDelta;

// IdleEventNotifier listens to signals and notifies its observers when an idle
// event is generated. An idle event is generated when the idle period reaches
// kIdleDelay. No further idle events will be generated until user becomes
// active again, followed by an idle period of kIdleDelay.
class IdleEventNotifier : public PowerManagerClient::Observer,
                          public ui::UserActivityObserver,
                          public viz::mojom::VideoDetectorObserver {
 public:
  // An idle event is generated after this idle period.
  static constexpr base::TimeDelta kIdleDelay =
      base::TimeDelta::FromSeconds(30);

  // Count number of key, mouse and touch events in the past hour.
  static constexpr base::TimeDelta kUserInputEventsDuration =
      base::TimeDelta::FromMinutes(60);

  // Granularity of input events is per minute.
  static constexpr int kNumUserInputEventsBuckets =
      kUserInputEventsDuration / base::TimeDelta::FromMinutes(1);

  struct ActivityData {
    ActivityData();

    ActivityData(const ActivityData& input_data);

    UserActivityEvent_Features_DayOfWeek last_activity_day =
        UserActivityEvent_Features_DayOfWeek_SUN;

    // Last activity time. This is the activity that triggers idle event.
    TimeOfDay last_activity_time_of_day;

    // Last user activity time of the sequence of activities ending in the last
    // activity. It could be different from |last_activity_time_of_day|
    // if the last activity is not a user activity (e.g. video). It is unset if
    // there is no user activity before the idle event is fired.
    base::Optional<TimeOfDay> last_user_activity_time_of_day;

    // Duration of activity up to the last activity.
    base::TimeDelta recent_time_active;

    // Duration from the last key/mouse/touch to the time when idle event is
    // generated.  It is unset if there is no key/mouse/touch activity before
    // the idle event.
    base::Optional<base::TimeDelta> time_since_last_key;
    base::Optional<base::TimeDelta> time_since_last_mouse;
    base::Optional<base::TimeDelta> time_since_last_touch;
    // How long recent video has been playing.
    base::TimeDelta video_playing_time;
    // Duration from when video ended. It is unset if video did not play
    // (|video_playing_time| = 0).
    base::Optional<base::TimeDelta> time_since_video_ended;

    int key_events_in_last_hour = 0;
    int mouse_events_in_last_hour = 0;
    int touch_events_in_last_hour = 0;
  };

  class Observer {
   public:
    // Called when an idle event is observed.
    virtual void OnIdleEventObserved(const ActivityData& activity_data) = 0;

   protected:
    virtual ~Observer() {}
  };

  IdleEventNotifier(PowerManagerClient* power_client,
                    ui::UserActivityDetector* detector,
                    viz::mojom::VideoDetectorObserverRequest request);
  ~IdleEventNotifier() override;

  // Set test clock so that we can check activity time.
  void SetClockForTesting(scoped_refptr<base::SequencedTaskRunner> task_runner,
                          base::Clock* test_clock,
                          std::unique_ptr<BootClock> test_boot_clock);

  // Adds or removes an observer.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // ui::UserActivityObserver overrides:
  void OnUserActivity(const ui::Event* event) override;

  // viz::mojom::VideoDetectorObserver overrides:
  void OnVideoActivityStarted() override;
  void OnVideoActivityEnded() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IdleEventNotifierTest, CheckInitialValues);
  friend class IdleEventNotifierTest;

  enum class ActivityType {
    USER_OTHER,  // All other user-related activities.
    KEY,
    MOUSE,
    VIDEO,
    TOUCH
  };

  struct ActivityDataInternal;

  ActivityData ConvertActivityData(
      const ActivityDataInternal& internal_data) const;

  // Resets |idle_delay_timer_|, it's called when a new activity is observed.
  void ResetIdleDelayTimer();

  // Called when |idle_delay_timer_| expires.
  void OnIdleDelayTimeout();

  // Updates all activity-related timestamps.
  void UpdateActivityData(ActivityType type);

  // Clears timestamps used to calculate |ActivityData::recent_time_active| so
  // that its duration is recalculated after user is inactive for more than
  // kIdleDelay or when suspend duration is longer than kIdleDelay.
  void ResetTimestampsPerIdleEvent();

  // It is base::DefaultClock, but will be set to a mock clock for tests.
  base::Clock* clock_;

  // It is RealBootClock, but will be set to FakeBootClock for tests.
  std::unique_ptr<BootClock> boot_clock_;

  base::OneShotTimer idle_delay_timer_;

  ScopedObserver<chromeos::PowerManagerClient,
                 chromeos::PowerManagerClient::Observer>
      power_manager_client_observer_;
  ScopedObserver<ui::UserActivityDetector, ui::UserActivityObserver>
      user_activity_observer_;

  // Last-received external power state. Changes are treated as user activity.
  base::Optional<power_manager::PowerSupplyProperties_ExternalPower>
      external_power_;

  base::ObserverList<Observer> observers_;

  // Holds activity timestamps while we monitor for idle events. It will be
  // converted to an ActivityData when an idle event is sent out.
  std::unique_ptr<ActivityDataInternal> internal_data_;

  // Whether video is playing.
  bool video_playing_ = false;
  mojo::Binding<viz::mojom::VideoDetectorObserver> binding_;

  std::unique_ptr<RecentEventsCounter> key_counter_;
  std::unique_ptr<RecentEventsCounter> mouse_counter_;
  std::unique_ptr<RecentEventsCounter> touch_counter_;

  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifier);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
