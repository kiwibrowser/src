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

// An abstraction for scheduling recurring tasks.
//

#include "google/cacheinvalidation/impl/recurring-task.h"
#include "google/cacheinvalidation/impl/log-macro.h"

namespace invalidation {

RecurringTask::RecurringTask(string name, Scheduler* scheduler, Logger* logger,
    Smearer* smearer, ExponentialBackoffDelayGenerator* delay_generator,
    TimeDelta initial_delay, TimeDelta timeout_delay) : name_(name),
    scheduler_(scheduler), logger_(logger), smearer_(smearer),
    delay_generator_(delay_generator), initial_delay_(initial_delay),
    timeout_delay_(timeout_delay), is_scheduled_(false) {
}

void RecurringTask::EnsureScheduled(string debug_reason) {
  RecurringTask::EnsureScheduled(false, debug_reason);
}

void RecurringTask::EnsureScheduled(bool is_retry, string debug_reason) {
  CHECK(scheduler_->IsRunningOnThread());
  if (is_scheduled_) {
    return;
  }
  TimeDelta delay;
  if (is_retry) {
    // For a retried task, determine the delay to be timeout + extra delay
    // (depending on whether a delay generator was provided or not).
    if (delay_generator_.get() != NULL) {
      delay = timeout_delay_ + delay_generator_->GetNextDelay();
    } else {
      delay = timeout_delay_ + smearer_->GetSmearedDelay(initial_delay_);
    }
  } else {
    delay = smearer_->GetSmearedDelay(initial_delay_);
  }

  TLOG(logger_, FINE, "[%s] Scheduling %d with a delay %d, Now = %d",
       debug_reason.c_str(), name_.c_str(), delay.ToInternalValue(),
       scheduler_->GetCurrentTime().ToInternalValue());
  scheduler_->Schedule(delay, NewPermanentCallback(this,
       &RecurringTask::RunTaskAndRescheduleIfNeeded));
  is_scheduled_ = true;
}

void RecurringTask::RunTaskAndRescheduleIfNeeded() {
  CHECK(scheduler_->IsRunningOnThread()) << "Not on scheduler thread";
  is_scheduled_ = false;

  // Run the task. If the task asks for a retry, reschedule it after at a
  // timeout delay. Otherwise, resets the delay_generator.
  if (RunTask()) {
    // The task asked to be rescheduled, so reschedule it after a timeout has
    // occurred.
    CHECK((delay_generator_ != NULL) ||
          (initial_delay_ > Scheduler::NoDelay()))
        << "Spinning: No exp back off and initial delay is zero";
    EnsureScheduled(true, "Retry");
  } else if (delay_generator_ != NULL) {
    // The task asked not to be rescheduled.  Treat it as having "succeeded"
    // and reset the delay generator.
    delay_generator_->Reset();
  }
}

}  // namespace invalidation
