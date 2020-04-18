// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"

#include "base/bind.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager_impl.h"

namespace base {
namespace sequence_manager {

RealTimeDomain::RealTimeDomain() : task_queue_manager_(nullptr) {}

RealTimeDomain::~RealTimeDomain() = default;

void RealTimeDomain::OnRegisterWithTaskQueueManager(
    TaskQueueManagerImpl* task_queue_manager) {
  task_queue_manager_ = task_queue_manager;
  DCHECK(task_queue_manager_);
}

LazyNow RealTimeDomain::CreateLazyNow() const {
  return task_queue_manager_->CreateLazyNow();
}

TimeTicks RealTimeDomain::Now() const {
  return task_queue_manager_->NowTicks();
}

void RealTimeDomain::RequestWakeUpAt(TimeTicks now, TimeTicks run_time) {
  // NOTE this is only called if the scheduled runtime is sooner than any
  // previously scheduled runtime, or there is no (outstanding) previously
  // scheduled runtime.
  task_queue_manager_->MaybeScheduleDelayedWork(FROM_HERE, this, now, run_time);
}

void RealTimeDomain::CancelWakeUpAt(TimeTicks run_time) {
  task_queue_manager_->CancelDelayedWork(this, run_time);
}

Optional<TimeDelta> RealTimeDomain::DelayTillNextTask(LazyNow* lazy_now) {
  TimeTicks next_run_time;
  if (!NextScheduledRunTime(&next_run_time))
    return nullopt;

  TimeTicks now = lazy_now->Now();
  if (now >= next_run_time)
    return TimeDelta();  // Makes DoWork post an immediate continuation.

  TimeDelta delay = next_run_time - now;
  TRACE_EVENT1("sequence_manager", "RealTimeDomain::DelayTillNextTask",
               "delay_ms", delay.InMillisecondsF());

  // The next task is sometime in the future. DoWork will make sure it gets
  // run at the right time.
  return delay;
}

void RealTimeDomain::AsValueIntoInternal(
    trace_event::TracedValue* state) const {}

const char* RealTimeDomain::GetName() const {
  return "RealTimeDomain";
}
}  // namespace sequence_manager
}  // namespace base
