// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/time_domain.h"

#include <set>

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/work_queue.h"

namespace base {
namespace sequence_manager {

TimeDomain::TimeDomain() = default;

TimeDomain::~TimeDomain() {
  DCHECK(main_thread_checker_.CalledOnValidThread());
}

void TimeDomain::RegisterQueue(internal::TaskQueueImpl* queue) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK_EQ(queue->GetTimeDomain(), this);
}

void TimeDomain::UnregisterQueue(internal::TaskQueueImpl* queue) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK_EQ(queue->GetTimeDomain(), this);

  LazyNow lazy_now = CreateLazyNow();
  ScheduleWakeUpForQueue(queue, nullopt, &lazy_now);
}

void TimeDomain::ScheduleWakeUpForQueue(
    internal::TaskQueueImpl* queue,
    Optional<internal::TaskQueueImpl::DelayedWakeUp> wake_up,
    LazyNow* lazy_now) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  DCHECK_EQ(queue->GetTimeDomain(), this);
  DCHECK(queue->IsQueueEnabled() || !wake_up);

  Optional<TimeTicks> previous_wake_up;
  if (!delayed_wake_up_queue_.empty())
    previous_wake_up = delayed_wake_up_queue_.Min().wake_up.time;

  if (wake_up) {
    // Insert a new wake-up into the heap.
    if (queue->heap_handle().IsValid()) {
      // O(log n)
      delayed_wake_up_queue_.ChangeKey(queue->heap_handle(),
                                       {wake_up.value(), queue});
    } else {
      // O(log n)
      delayed_wake_up_queue_.insert({wake_up.value(), queue});
    }
  } else {
    // Remove a wake-up from heap if present.
    if (queue->heap_handle().IsValid())
      delayed_wake_up_queue_.erase(queue->heap_handle());
  }

  Optional<TimeTicks> new_wake_up;
  if (!delayed_wake_up_queue_.empty())
    new_wake_up = delayed_wake_up_queue_.Min().wake_up.time;

  if (previous_wake_up == new_wake_up)
    return;

  if (previous_wake_up)
    CancelWakeUpAt(previous_wake_up.value());
  if (new_wake_up)
    RequestWakeUpAt(lazy_now->Now(), new_wake_up.value());
}

void TimeDomain::WakeUpReadyDelayedQueues(LazyNow* lazy_now) {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  // Wake up any queues with pending delayed work.  Note std::multimap stores
  // the elements sorted by key, so the begin() iterator points to the earliest
  // queue to wake-up.
  while (!delayed_wake_up_queue_.empty() &&
         delayed_wake_up_queue_.Min().wake_up.time <= lazy_now->Now()) {
    internal::TaskQueueImpl* queue = delayed_wake_up_queue_.Min().queue;
    queue->WakeUpForDelayedWork(lazy_now);
  }
}

bool TimeDomain::NextScheduledRunTime(TimeTicks* out_time) const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (delayed_wake_up_queue_.empty())
    return false;

  *out_time = delayed_wake_up_queue_.Min().wake_up.time;
  return true;
}

bool TimeDomain::NextScheduledTaskQueue(
    internal::TaskQueueImpl** out_task_queue) const {
  DCHECK(main_thread_checker_.CalledOnValidThread());
  if (delayed_wake_up_queue_.empty())
    return false;

  *out_task_queue = delayed_wake_up_queue_.Min().queue;
  return true;
}

void TimeDomain::AsValueInto(trace_event::TracedValue* state) const {
  state->BeginDictionary();
  state->SetString("name", GetName());
  state->SetInteger("registered_delay_count", delayed_wake_up_queue_.size());
  if (!delayed_wake_up_queue_.empty()) {
    TimeDelta delay = delayed_wake_up_queue_.Min().wake_up.time - Now();
    state->SetDouble("next_delay_ms", delay.InMillisecondsF());
  }
  AsValueIntoInternal(state);
  state->EndDictionary();
}

}  // namespace sequence_manager
}  // namespace base
