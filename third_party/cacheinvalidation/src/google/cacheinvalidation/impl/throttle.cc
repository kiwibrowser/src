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

// Throttles calls to a function.

#include "google/cacheinvalidation/impl/throttle.h"

#include <algorithm>

#include "google/cacheinvalidation/include/system-resources.h"
#include "google/cacheinvalidation/deps/callback.h"

namespace invalidation {

using INVALIDATION_STL_NAMESPACE::max;

Throttle::Throttle(
    const RepeatedPtrField<RateLimitP>& rate_limits, Scheduler* scheduler,
    Closure* listener)
    : rate_limits_(rate_limits), scheduler_(scheduler), listener_(listener),
      timer_scheduled_(false) {

  // Find the largest 'count' in all of the rate limits, as this is the size of
  // the buffer of recent messages we need to retain.
  max_recent_events_ = 1;
  for (size_t i = 0; i < static_cast<size_t>(rate_limits_.size()); ++i) {
    const RateLimitP& rate_limit = rate_limits.Get(i);
    CHECK(rate_limit.window_ms() > rate_limit.count()) <<
        "Windows size too small";
    max_recent_events_ = max(static_cast<int>(max_recent_events_),
        rate_limits_.Get(i).count());
  }
}

void Throttle::Fire() {
  if (timer_scheduled_) {
    // We're already rate-limited and have a deferred call scheduled.  Just
    // return.  The flag will be reset when the deferred task runs.
    return;
  }
  // Go through all of the limits to see if we've hit one.  If so, schedule a
  // task to try again once that limit won't be violated.  If no limits would be
  // violated, send.
  Time now = scheduler_->GetCurrentTime();
  for (size_t i = 0; i < static_cast<size_t>(rate_limits_.size()); ++i) {
    RateLimitP rate_limit   = rate_limits_.Get(i);

    // We're now checking whether sending would violate a rate limit of 'count'
    // messages per 'window_size'.
    int count = rate_limit.count();
    TimeDelta window_size = TimeDelta::FromMilliseconds(rate_limit.window_ms());

    // First, see how many messages we've sent so far (up to the size of our
    // recent message buffer).
    int num_recent_messages = recent_event_times_.size();

    // Check whether we've sent enough messages yet that we even need to
    // consider this rate limit.
    if (num_recent_messages >= count) {
      // If we've sent enough messages to reach this limit, see how long ago we
      // sent the first message in the interval, and add sufficient delay to
      // avoid violating the rate limit.

      // We have sent at least 'count' messages.  See how long ago we sent the
      // 'count'-th last message.  This defines the start of a window in which
      // no more than 'count' messages may be sent.
      Time window_start = recent_event_times_[num_recent_messages - count];

      // The end of this window is 'window_size' after the start.
      Time window_end = window_start + window_size;

      // Check where the end of the window is relative to the current time.  If
      // the end of the window is in the future, then sending now would violate
      // the rate limit, so we must defer.
      TimeDelta window_end_from_now = window_end - now;
      if (window_end_from_now > TimeDelta::FromSeconds(0)) {
        // Rate limit would be violated, so schedule a task to try again.

        // Set the flag to indicate we have a deferred task scheduled.  No need
        // to continue checking other rate limits now.
        timer_scheduled_ = true;
        scheduler_->Schedule(
            window_end_from_now,
            NewPermanentCallback(this, &Throttle::RetryFire));
        return;
      }
    }
  }
  // We checked all the rate limits, and none would have been violated, so it's
  // safe to call the listener.
  listener_->Run();

  // Record the fact that we're triggering an event now.
  recent_event_times_.push_back(scheduler_->GetCurrentTime());

  // Only save up to max_recent_events_ event times.
  if (recent_event_times_.size() > max_recent_events_) {
    recent_event_times_.pop_front();
  }
}

}  // namespace invalidation
