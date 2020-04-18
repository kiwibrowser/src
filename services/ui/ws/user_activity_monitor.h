// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_USER_ACTIVITY_MONITOR_H_
#define SERVICES_UI_WS_USER_ACTIVITY_MONITOR_H_

#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/ui/public/interfaces/user_activity_monitor.mojom.h"

namespace ui {
namespace ws {

namespace test {
class UserActivityMonitorTestApi;
}

class UserActivityMonitor : public mojom::UserActivityMonitor {
 public:
  // |now_clock| is used to get the timestamp. If |now_clock| is nullptr, then
  // DefaultTickClock is used.
  explicit UserActivityMonitor(
      std::unique_ptr<const base::TickClock> now_clock);
  ~UserActivityMonitor() override;

  // Should be called whenever some input event is received from the user.
  void OnUserActivity();

  // Provides an implementation for the remote request.
  void Add(mojom::UserActivityMonitorRequest request);

  // mojom::UserActivityMonitor:
  void AddUserActivityObserver(
      uint32_t delay_between_notify_secs,
      mojom::UserActivityObserverPtr observer) override;
  void AddUserIdleObserver(uint32_t idleness_in_minutes,
                           mojom::UserIdleObserverPtr observer) override;

 private:
  friend class test::UserActivityMonitorTestApi;

  // Makes sure the idle timer is running.
  void ActivateIdleTimer();

  // Called every minute when |idle_timer_| is active.
  void OnMinuteTimer();

  void OnActivityObserverDisconnected(mojom::UserActivityObserver* observer);
  void OnIdleObserverDisconnected(mojom::UserIdleObserver* observer);

  mojo::BindingSet<mojom::UserActivityMonitor> bindings_;
  std::unique_ptr<const base::TickClock> now_clock_;

  struct ActivityObserverInfo {
    base::TimeTicks last_activity_notification;
    base::TimeDelta delay;
  };
  std::vector<std::pair<ActivityObserverInfo, mojom::UserActivityObserverPtr>>
      activity_observers_;

  struct IdleObserverInfo {
    base::TimeTicks last_idle_state_notification;
    base::TimeDelta idle_duration;
    mojom::UserIdleObserver::IdleState idle_state;
  };
  std::vector<std::pair<IdleObserverInfo, mojom::UserIdleObserverPtr>>
      idle_observers_;
  // Timer used to determine user's idleness. The timer is run only when at
  // least one of the idle-observers are notified ACTIVE.
  base::RepeatingTimer idle_timer_;
  base::TimeTicks last_activity_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityMonitor);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_USER_ACTIVITY_MONITOR_H_
