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

#include "google/cacheinvalidation/test/deterministic-scheduler.h"

namespace invalidation {

void DeterministicScheduler::StopScheduler() {
  run_state_.Stop();
  // Delete any tasks that haven't been run.
  while (!work_queue_.empty()) {
    TaskEntry top_elt = work_queue_.top();
    work_queue_.pop();
    delete top_elt.task;
  }
}

void DeterministicScheduler::Schedule(TimeDelta delay, Closure* task) {
  CHECK(IsCallbackRepeatable(task));
  CHECK(run_state_.IsStarted());
  TLOG(logger_, INFO, "(Now: %d) Enqueuing %p with delay %d",
       current_time_.ToInternalValue(), task, delay.InMilliseconds());
  work_queue_.push(TaskEntry(GetCurrentTime() + delay, current_id_++, task));
}

void DeterministicScheduler::PassTime(TimeDelta delta_time, TimeDelta step) {
  CHECK(delta_time >= TimeDelta()) << "cannot pass a negative amount of time";
  TimeDelta cumulative = TimeDelta();

  // Run tasks that are ready to run now.
  RunReadyTasks();

  // Advance in increments of |step| until doing so would cause us to go past
  // the requested |delta_time|.
  while ((cumulative + step) < delta_time) {
    ModifyTime(step);
    cumulative += step;
    RunReadyTasks();
  }

  // Then advance one final time to finish out the interval.
  ModifyTime(delta_time - cumulative);
  RunReadyTasks();
}

void DeterministicScheduler::RunReadyTasks() {
  running_internal_ = true;
  while (RunNextTask()) {
    continue;
  }
  running_internal_ = false;
}

bool DeterministicScheduler::RunNextTask() {
  if (!work_queue_.empty()) {
    // The queue is not empty, so get the first task and see if its scheduled
    // execution time has passed.
    TaskEntry top_elt = work_queue_.top();
    if (top_elt.time <= GetCurrentTime()) {
      // The task is scheduled to run in the past or present, so remove it
      // from the queue and run the task.
      work_queue_.pop();
      TLOG(logger_, FINE, "(Now: %d) Running task %p",
           current_time_.ToInternalValue(), top_elt.task);
      top_elt.task->Run();
      delete top_elt.task;
      return true;
    }
  }
  return false;
}

}  // namespace invalidation
