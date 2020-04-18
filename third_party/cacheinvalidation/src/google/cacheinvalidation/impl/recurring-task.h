// Copyright 2012 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//
// An abstraction for scheduling recurring tasks. Combines idempotent scheduling
// and smearing with conditional retries and exponential backoff. Does not
// implement throttling. Designed to support a variety of use cases, including
// the following capabilities.
//
// * Idempotent scheduling, e.g., ensuring that a batching task is scheduled
//  exactly once.
// * Recurring tasks, e.g., periodic heartbeats.
// * Retriable actions aimed at state change, e.g., sending initialization
//  messages.
//
// Each instance of this class manages the state for a single task. See the
// invalidation-client-impl.cc for examples.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_RECURRING_TASK_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_RECURRING_TASK_H_

#include "base/macros.h"
#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/impl/exponential-backoff-delay-generator.h"
#include "google/cacheinvalidation/impl/smearer.h"

namespace invalidation {

class RecurringTask {
 public:
  /* Creates a recurring task with the given parameters. The specs of the
   * parameters are given in the instance variables.
   *
   * The created task is first scheduled with a smeared delay of
   * |initial_delay|. If the |this->run()| returns true on its execution, the
   * task is rescheduled after a |timeout_delay| + smeared delay of
   * |initial_delay| or |timeout_delay| + |delay_generator->GetNextDelay()|
   * depending on whether the |delay_generator| is null or not.
   *
   * Space for |scheduler|, |logger|, |smearer| is owned by the caller.
   * Space for |delay_generator| is owned by the callee.
   */
  RecurringTask(string name, Scheduler* scheduler, Logger* logger,
      Smearer* smearer, ExponentialBackoffDelayGenerator* delay_generator,
      TimeDelta initial_delay, TimeDelta timeout_delay);

  virtual ~RecurringTask() {}

  /* Run the task and return true if the task should be rescheduled after a
   * timeout. If false is returned, the task is not scheduled again until
   * |ensure_scheduled| is called again.
   */
  virtual bool RunTask() = 0;

  /* Ensures that the task is scheduled (with |debug_reason| as the reason to be
   * printed for debugging purposes). If the task has been scheduled, it is
   * not scheduled again.
   *
   * REQUIRES: Must be called from the scheduler thread.
   */
  void EnsureScheduled(string debug_reason);

  /* Space for the returned Smearer is still owned by this class. */
  Smearer* smearer() {
    return smearer_;
  }

 private:
  /* Run the task and check if it needs to be rescheduled. If so, reschedule it
   * after the appropriate delay.
   */
  void RunTaskAndRescheduleIfNeeded();

  /* Ensures that the task is scheduled if it is already not scheduled. If
   * already scheduled, this method is a no-op. If |is_retry| is |false|, smears
   * the |initial_delay_| and uses that delay for scheduling. If |is_retry| is
   * true, it determines the new delay to be
   * |timeout_delay_ + delay_generator.GetNextDelay()| if |delay_generator| is
   * non-null. If |delay_generator| is null, schedules the task after a delay of
   * |timeout_delay_| + smeared value of |initial_delay_|.
   *
   * REQUIRES: Must be called from the scheduler thread.
   */
  void EnsureScheduled(bool is_retry, string debug_reason);

  /* Name of the task (for debugging purposes mostly). */
  string name_;

  /* Scheduler for the scheduling the task as needed. */
  Scheduler* scheduler_;

  /* A logger. */
  Logger* logger_;

  /* A smearer for spreading the delays. */
  Smearer* smearer_;

  /* A delay generator for exponential backoff. */
  scoped_ptr<ExponentialBackoffDelayGenerator> delay_generator_;

  /*
   * The time after which the task is scheduled first. If no delayGenerator is
   * specified, this is also the delay used for retries.
   */
  TimeDelta initial_delay_;

  /* For a task that is retried, add this time to the delay. */
  TimeDelta timeout_delay_;

  /* If the task has been currently scheduled. */
  bool is_scheduled_;

  DISALLOW_COPY_AND_ASSIGN(RecurringTask);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_RECURRING_TASK_H_
