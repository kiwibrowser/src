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

// An implementation of the Scheduler interface for unit testing (in a
// single-threaded environment).

#ifndef GOOGLE_CACHEINVALIDATION_TEST_DETERMINISTIC_SCHEDULER_H_
#define GOOGLE_CACHEINVALIDATION_TEST_DETERMINISTIC_SCHEDULER_H_

#include <stdint.h>

#include <queue>
#include <string>
#include <utility>

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/callback.h"
#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/deps/string_util.h"
#include "google/cacheinvalidation/deps/time.h"
#include "google/cacheinvalidation/impl/log-macro.h"
#include "google/cacheinvalidation/impl/run-state.h"

namespace invalidation {

// An entry in the work queue.  Ensures that tasks don't run until their
// scheduled time, and for a given time, they run in the order in which they
// were enqueued.
struct TaskEntry {
  TaskEntry(Time time, int64_t id, Closure* task)
      : time(time), id(id), task(task) {}

  bool operator<(const TaskEntry& other) const {
    // Priority queue returns *largest* element first.
    return (time > other.time) ||
        ((time == other.time) && (id > other.id));
  }
  Time time;  // the time at which to run
  int64_t id;  // the order in which this task was enqueued
  Closure* task;  // the task to be run
};

class DeterministicScheduler : public Scheduler {
 public:
  // Caller retains ownershup of |logger|.
  explicit DeterministicScheduler(Logger* logger)
      : current_id_(0), running_internal_(false), logger_(logger) {}

  virtual ~DeterministicScheduler() {
    StopScheduler();
  }

  virtual void SetSystemResources(SystemResources* resources) {
    // Nothing to do.
  }

  virtual Time GetCurrentTime() const {
    return current_time_;
  }

  void StartScheduler() {
    run_state_.Start();
  }

  void StopScheduler();

  virtual void Schedule(TimeDelta delay, Closure* task);

  virtual bool IsRunningOnThread() const {
    return running_internal_;
  }

  void SetInitialTime(Time new_time) {
    current_time_ = new_time;
  }

  // Passes |delta_time| in increments of at most |step|, executing all pending
  // tasks during that interval.
  void PassTime(TimeDelta delta_time, TimeDelta step);

  // Passes |delta_time| in default-sized increments, executing all pending
  // tasks.
  void PassTime(TimeDelta delta_time) {
    PassTime(delta_time, DefaultTimeStep());
  }

 private:
  // Runs all the work in the queue that should be executed by the current time.
  // Note that tasks run may enqueue additional immediate tasks, and this call
  // won't return until they've completed as well.  While these tasks are
  // running, the running_internal_ flag is set, so IsRunningOnThread()
  // will return true.
  void RunReadyTasks();

  // Default time step when simulating passage of time.  Chosen to be
  // significantly smaller than any scheduling interval used by the client
  // library.
  static TimeDelta DefaultTimeStep() {
    return TimeDelta::FromMilliseconds(10);
  }

  void ModifyTime(TimeDelta delta_time) {
    current_time_ += delta_time;
  }

  // Attempts to run a task, returning true is there was a task to run.
  bool RunNextTask();

  // The current time, which may be set by the test.
  Time current_time_;

  // The id number of the next task.
  uint64_t current_id_;

  // Whether or not the scheduler has been started/stopped.
  RunState run_state_;

  // Whether or not we're currently running internal tasks from the internal
  // queue.
  bool running_internal_;

  // A priority queue on which the actual tasks are enqueued.
  std::priority_queue<TaskEntry> work_queue_;

  // A logger.
  Logger* logger_;
};

// A simple deterministic scheduler that always indicates that it is on
// the correct thread.
class SimpleDeterministicScheduler : public DeterministicScheduler {
 public:
  explicit SimpleDeterministicScheduler(Logger* logger)
    : DeterministicScheduler(logger) {}

  virtual bool IsRunningOnThread() const {
    return true;
  }
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_TEST_DETERMINISTIC_SCHEDULER_H_
