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

#include "google/cacheinvalidation/impl/exponential-backoff-delay-generator.h"

namespace invalidation {

TimeDelta ExponentialBackoffDelayGenerator::GetNextDelay() {
  TimeDelta delay = Scheduler::NoDelay();  // After a reset, delay is zero.
  if (in_retry_mode) {
    // We used to multiply the current_max_delay_ by the double, but this
    // implicitly truncated the double to an integer, which would always be 0.
    // By converting to and from milliseconds, we avoid this problem.
    delay = TimeDelta::FromMilliseconds(
        random_->RandDouble() * current_max_delay_.InMilliseconds());

    // Adjust the max for the next run.
    TimeDelta max_delay = initial_max_delay_ * max_exponential_factor_;
    if (current_max_delay_ <= max_delay) {  // Guard against overflow.
      current_max_delay_ *= 2;
      if (current_max_delay_ > max_delay) {
        current_max_delay_ = max_delay;
      }
    }
  }
  in_retry_mode = true;
  return delay;
}
}
