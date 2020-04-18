// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_timeout_timer.h"

#include "base/stl_util.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "services/network/public/cpp/features.h"

namespace content {

namespace {

int NextEventId() {
  // Event id should not start from zero since HashMap in Blink requires
  // non-zero keys.
  static int s_next_event_id = 1;
  CHECK_LT(s_next_event_id, std::numeric_limits<int>::max());
  return s_next_event_id++;
}

}  // namespace

// static
constexpr base::TimeDelta ServiceWorkerTimeoutTimer::kIdleDelay;
constexpr base::TimeDelta ServiceWorkerTimeoutTimer::kEventTimeout;
constexpr base::TimeDelta ServiceWorkerTimeoutTimer::kUpdateInterval;

ServiceWorkerTimeoutTimer::ServiceWorkerTimeoutTimer(
    base::RepeatingClosure idle_callback)
    : ServiceWorkerTimeoutTimer(std::move(idle_callback),
                                base::DefaultTickClock::GetInstance()) {}

ServiceWorkerTimeoutTimer::ServiceWorkerTimeoutTimer(
    base::RepeatingClosure idle_callback,
    const base::TickClock* tick_clock)
    : idle_callback_(std::move(idle_callback)), tick_clock_(tick_clock) {
  // |idle_callback_| will be invoked if no event happens in |kIdleDelay|.
  idle_time_ = tick_clock_->NowTicks() + kIdleDelay;
  timer_.Start(FROM_HERE, kUpdateInterval,
               base::BindRepeating(&ServiceWorkerTimeoutTimer::UpdateStatus,
                                   base::Unretained(this)));
}

ServiceWorkerTimeoutTimer::~ServiceWorkerTimeoutTimer() {
  // Abort all callbacks.
  for (auto& event : inflight_events_)
    std::move(event.abort_callback).Run();
};

int ServiceWorkerTimeoutTimer::StartEvent(
    base::OnceCallback<void(int /* event_id */)> abort_callback) {
  return StartEventWithCustomTimeout(std::move(abort_callback), kEventTimeout);
}

int ServiceWorkerTimeoutTimer::StartEventWithCustomTimeout(
    base::OnceCallback<void(int /* event_id */)> abort_callback,
    base::TimeDelta timeout) {
  if (did_idle_timeout()) {
    idle_time_ = base::TimeTicks();
    did_idle_timeout_ = false;
    while (!pending_tasks_.empty()) {
      std::move(pending_tasks_.front()).Run();
      pending_tasks_.pop();
    }
  }

  idle_time_ = base::TimeTicks();
  const int event_id = NextEventId();
  std::set<EventInfo>::iterator iter;
  bool is_inserted;
  std::tie(iter, is_inserted) = inflight_events_.emplace(
      event_id, tick_clock_->NowTicks() + timeout,
      base::BindOnce(std::move(abort_callback), event_id));
  DCHECK(is_inserted);
  id_event_map_.emplace(event_id, iter);
  return event_id;
}

void ServiceWorkerTimeoutTimer::EndEvent(int event_id) {
  auto iter = id_event_map_.find(event_id);
  DCHECK(iter != id_event_map_.end());
  inflight_events_.erase(iter->second);
  id_event_map_.erase(iter);
  if (inflight_events_.empty()) {
    idle_time_ = tick_clock_->NowTicks() + kIdleDelay;
    MaybeTriggerIdleTimer();
  }
}

void ServiceWorkerTimeoutTimer::PushPendingTask(
    base::OnceClosure pending_task) {
  DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
  DCHECK(did_idle_timeout());
  pending_tasks_.emplace(std::move(pending_task));
}

void ServiceWorkerTimeoutTimer::SetIdleTimerDelayToZero() {
  DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
  zero_idle_timer_delay_ = true;
  if (inflight_events_.empty())
    MaybeTriggerIdleTimer();
}

void ServiceWorkerTimeoutTimer::UpdateStatus() {
  if (!ServiceWorkerUtils::IsServicificationEnabled())
    return;

  base::TimeTicks now = tick_clock_->NowTicks();

  // Abort all events exceeding |kEventTimeout|.
  auto iter = inflight_events_.begin();
  while (iter != inflight_events_.end() && iter->expiration_time <= now) {
    int event_id = iter->id;
    base::OnceClosure callback = std::move(iter->abort_callback);
    iter = inflight_events_.erase(iter);
    id_event_map_.erase(event_id);
    std::move(callback).Run();
    // Shut down the worker as soon as possible since the worker may have gone
    // into bad state.
    zero_idle_timer_delay_ = true;
  }

  // If |inflight_events_| is empty, the worker is now idle.
  if (inflight_events_.empty() && idle_time_.is_null()) {
    idle_time_ = tick_clock_->NowTicks() + kIdleDelay;
    if (MaybeTriggerIdleTimer())
      return;
  }

  if (!idle_time_.is_null() && idle_time_ < now) {
    did_idle_timeout_ = true;
    idle_callback_.Run();
  }
}

bool ServiceWorkerTimeoutTimer::MaybeTriggerIdleTimer() {
  DCHECK(inflight_events_.empty());
  if (!zero_idle_timer_delay_)
    return false;

  did_idle_timeout_ = true;
  idle_callback_.Run();
  return true;
}

ServiceWorkerTimeoutTimer::EventInfo::EventInfo(
    int id,
    base::TimeTicks expiration_time,
    base::OnceClosure abort_callback)
    : id(id),
      expiration_time(expiration_time),
      abort_callback(std::move(abort_callback)) {}

ServiceWorkerTimeoutTimer::EventInfo::~EventInfo() = default;

bool ServiceWorkerTimeoutTimer::EventInfo::operator<(
    const EventInfo& other) const {
  if (expiration_time == other.expiration_time)
    return id < other.id;
  return expiration_time < other.expiration_time;
}

}  // namespace content
