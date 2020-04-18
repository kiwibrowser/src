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

// An abstraction that keeps track of whether the caller is started or stopped
// and only allows the following transitions NOT_STARTED -> STARTED ->
// STOPPED. This class is thread-safe.

#ifndef GOOGLE_CACHEINVALIDATION_IMPL_RUN_STATE_H_
#define GOOGLE_CACHEINVALIDATION_IMPL_RUN_STATE_H_

#include "google/cacheinvalidation/client.pb.h"
#include "google/cacheinvalidation/deps/logging.h"
#include "google/cacheinvalidation/deps/mutex.h"
#include "google/cacheinvalidation/deps/string_util.h"

namespace invalidation {

using ::ipc::invalidation::RunStateP_State;
using ::ipc::invalidation::RunStateP_State_NOT_STARTED;
using ::ipc::invalidation::RunStateP_State_STARTED;
using ::ipc::invalidation::RunStateP_State_STOPPED;

class RunState {
 public:
  RunState() : current_state_(RunStateP_State_NOT_STARTED) {}

  /* Marks the current state to be STARTED.
   *
   * REQUIRES: Current state is NOT_STARTED.
   */
  void Start() {
    MutexLock m(&lock_);
    CHECK(current_state_ == RunStateP_State_NOT_STARTED) << "Cannot start: "
        << current_state_;
    current_state_ = RunStateP_State_STARTED;
  }

  /* Marks the current state to be STOPPED.
   *
   * REQUIRES: Current state is STARTED.
   */
  void Stop() {
    MutexLock m(&lock_);
    CHECK(current_state_ == RunStateP_State_STARTED) << "Cannot stop: "
        << current_state_;
    current_state_ = RunStateP_State_STOPPED;
  }

  /* Returns true iff Start has been called on this but Stop has not been
   * called.
   */
  bool IsStarted() const {
    // Don't treat locking a mutex as mutation.
    MutexLock m((Mutex *) &lock_);  // NOLINT
    return current_state_ == RunStateP_State_STARTED;
  }

  /* Returns true iff Start and Stop have been called on this object. */
  bool IsStopped() const {
    // Don't treat locking a mutex as mutation.
    MutexLock m((Mutex *) &lock_);  // NOLINT
    return current_state_ == RunStateP_State_STOPPED;
  }

  string ToString() {
    return StringPrintf("<RunState %d>", current_state_);
  }

 private:
  RunStateP_State current_state_;
  Mutex lock_;
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_IMPL_RUN_STATE_H_
