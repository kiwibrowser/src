// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/timers/alarm_timer_chromeos.h"

#include <stdint.h>
#include <sys/timerfd.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/debug/task_annotator.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/pending_task.h"
#include "base/trace_event/trace_event.h"

namespace timers {

AlarmTimer::AlarmTimer(bool retain_user_task, bool is_repeating)
    : base::Timer(retain_user_task, is_repeating),
      alarm_fd_(timerfd_create(CLOCK_REALTIME_ALARM, 0)),
      weak_factory_(this) {}

AlarmTimer::~AlarmTimer() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  Stop();
}

void AlarmTimer::Stop() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());

  if (!base::Timer::is_running())
    return;

  if (!CanWakeFromSuspend()) {
    base::Timer::Stop();
    return;
  }

  // Cancel any previous callbacks.
  weak_factory_.InvalidateWeakPtrs();

  base::Timer::set_is_running(false);
  alarm_fd_watcher_.reset();
  pending_task_.reset();

  if (!base::Timer::retain_user_task())
    base::Timer::set_user_task(base::Closure());
}

void AlarmTimer::Reset() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(!base::Timer::user_task().is_null());

  if (!CanWakeFromSuspend()) {
    base::Timer::Reset();
    return;
  }

  // Cancel any previous callbacks and stop watching |alarm_fd_|.
  weak_factory_.InvalidateWeakPtrs();
  alarm_fd_watcher_.reset();

  // Ensure that the delay is not negative.
  const base::TimeDelta delay =
      std::max(base::TimeDelta(), base::Timer::GetCurrentDelay());

  // Set up the pending task.
  base::Timer::set_desired_run_time(
      delay.is_zero() ? base::TimeTicks() : base::TimeTicks::Now() + delay);
  pending_task_ = std::make_unique<base::PendingTask>(
      base::Timer::posted_from(), base::Timer::user_task(),
      base::Timer::desired_run_time());

  // Set |alarm_fd_| to be signaled when the delay expires. If the delay is
  // zero, |alarm_fd_| will never be signaled. This overrides the previous
  // delay, if any.
  itimerspec alarm_time = {};
  alarm_time.it_value.tv_sec = delay.InSeconds();
  alarm_time.it_value.tv_nsec =
      (delay.InMicroseconds() % base::Time::kMicrosecondsPerSecond) *
      base::Time::kNanosecondsPerMicrosecond;
  if (timerfd_settime(alarm_fd_, 0, &alarm_time, NULL) < 0)
    PLOG(ERROR) << "Error while setting alarm time.  Timer will not fire";

  // The timer is running.
  base::Timer::set_is_running(true);

  // If the delay is zero, post the task now.
  if (delay.is_zero()) {
    origin_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&AlarmTimer::OnTimerFired, weak_factory_.GetWeakPtr()));
  } else {
    // Otherwise, if the delay is not zero, generate a tracing event to indicate
    // that the task was posted and watch |alarm_fd_|.
    base::debug::TaskAnnotator().DidQueueTask("AlarmTimer::Reset",
                                              *pending_task_);
    alarm_fd_watcher_ = base::FileDescriptorWatcher::WatchReadable(
        alarm_fd_, base::Bind(&AlarmTimer::OnAlarmFdReadableWithoutBlocking,
                              weak_factory_.GetWeakPtr()));
  }
}

void AlarmTimer::OnAlarmFdReadableWithoutBlocking() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(base::Timer::IsRunning());

  // Read from |alarm_fd_| to ack the event.
  char val[sizeof(uint64_t)];
  if (!base::ReadFromFD(alarm_fd_, val, sizeof(uint64_t)))
    PLOG(DFATAL) << "Unable to read from timer file descriptor.";

  OnTimerFired();
}

void AlarmTimer::OnTimerFired() {
  DCHECK(origin_task_runner_->RunsTasksInCurrentSequence());
  DCHECK(base::Timer::IsRunning());
  DCHECK(pending_task_.get());

  // Take ownership of the PendingTask to prevent it from being deleted if the
  // AlarmTimer is deleted.
  const auto pending_user_task = std::move(pending_task_);

  base::WeakPtr<AlarmTimer> weak_ptr = weak_factory_.GetWeakPtr();

  // Run the task.
  TRACE_TASK_EXECUTION("AlarmTimer::OnTimerFired", *pending_user_task);
  base::debug::TaskAnnotator().RunTask("AlarmTimer::Reset",
                                       pending_user_task.get());

  // If the timer wasn't deleted, stopped or reset by the callback, reset or
  // stop it.
  if (weak_ptr.get()) {
    if (base::Timer::is_repeating())
      Reset();
    else
      Stop();
  }
}

bool AlarmTimer::CanWakeFromSuspend() const {
  return alarm_fd_ != -1;
}

OneShotAlarmTimer::OneShotAlarmTimer() : AlarmTimer(false, false) {
}

OneShotAlarmTimer::~OneShotAlarmTimer() {
}

RepeatingAlarmTimer::RepeatingAlarmTimer() : AlarmTimer(true, true) {
}

RepeatingAlarmTimer::~RepeatingAlarmTimer() {
}

SimpleAlarmTimer::SimpleAlarmTimer() : AlarmTimer(true, false) {
}

SimpleAlarmTimer::~SimpleAlarmTimer() {
}

}  // namespace timers
