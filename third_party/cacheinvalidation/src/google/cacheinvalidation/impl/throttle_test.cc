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

// Tests the throttle.

#include "google/cacheinvalidation/deps/googletest.h"
#include "google/cacheinvalidation/impl/proto-helpers.h"
#include "google/cacheinvalidation/impl/throttle.h"
#include "google/cacheinvalidation/test/deterministic-scheduler.h"
#include "google/cacheinvalidation/test/test-logger.h"

namespace invalidation {

class ThrottleTest : public testing::Test {
 public:
  ThrottleTest() : call_count_(0) {}

  virtual ~ThrottleTest() {}

  // Increments the call count.
  void IncrementCounter() {
    ++call_count_;
  }

  // Increments the call count and checks state to ensure that rate limits are
  // being observed.
  void IncrementAndCheckRateLimits() {
    // Increment the call count.
    ++call_count_;
    // Check that we haven't been called within the last one second.
    Time now = scheduler_->GetCurrentTime();
    ASSERT_TRUE((now - last_call_time_) >= TimeDelta::FromSeconds(1));
    // Update the last time we were called to now.
    last_call_time_ = now;
    // Check that enough time has passed to allow the number of calls we've
    // received.
    Time min_time = start_time_ + TimeDelta::FromMinutes(
        (call_count_ - 1) / kMessagesPerMinute);
    ASSERT_TRUE(min_time <= now);
  }

  void SetUp() {
    logger_.reset(new TestLogger());
    scheduler_.reset(new DeterministicScheduler(logger_.get()));
    start_time_ = scheduler_->GetCurrentTime();
    call_count_ = 0;
    last_call_time_ = Time() - TimeDelta::FromHours(1);
    ProtoHelpers::InitRateLimitP(1000, kMessagesPerSecond, rate_limits_.Add());
    ProtoHelpers::InitRateLimitP(60 * 1000, kMessagesPerMinute,
        rate_limits_.Add());
  }

  int call_count_;
  Time start_time_;
  Time last_call_time_;
  scoped_ptr<DeterministicScheduler> scheduler_;
  scoped_ptr<Logger> logger_;
  RepeatedPtrField<RateLimitP> rate_limits_;

  static const int kMessagesPerSecond;
  static const int kMessagesPerMinute;
};

const int ThrottleTest::kMessagesPerSecond = 1;
const int ThrottleTest::kMessagesPerMinute = 6;

/* Make a throttler similar to what we expect the Ticl to use and check that it
 * behaves as expected when called at a number of specific times.  More
 * specifically:
 *
 * 1. Check that the first call to Fire() triggers a call immediately.
 * 2. Subsequent calls within the next one second don't trigger any calls.
 * 3. After one second, one (and only one) buffered call is triggered.
 * 4. If we Fire() slowly, each will trigger an immediate call until we reach
 *    the per-minute rate limit.
 * 5. However, after a minute, another call i.
 */
TEST_F(ThrottleTest, ThrottlingScripted) {
  scheduler_->StartScheduler();
  Closure* listener =
      NewPermanentCallback(this, &ThrottleTest::IncrementCounter);

  scoped_ptr<Throttle> throttle(
      new Throttle(rate_limits_, scheduler_.get(), listener));

  // The first time we fire(), it should call right away.
  throttle->Fire();
  scheduler_->PassTime(TimeDelta());
  ASSERT_EQ(1, call_count_);

  // However, if we now fire() a bunch more times within one second, there
  // should be no more calls to the listener ...
  TimeDelta short_interval = TimeDelta::FromMilliseconds(80);
  int fire_count = 10;
  ASSERT_TRUE(short_interval * fire_count < TimeDelta::FromSeconds(1));
  for (int i = 0; i < fire_count; ++i) {
    scheduler_->PassTime(short_interval);
    throttle->Fire();
    ASSERT_EQ(1, call_count_);
  }

  // Time since first event is now fireCount * intervalBetweenFires, i.e., 800.

  // ... until the short throttle interval passes, at which time it should be
  // called once more.
  scheduler_->PassTime(
      start_time_ + TimeDelta::FromSeconds(1) - scheduler_->GetCurrentTime());

  ASSERT_EQ(2, call_count_);

  // However, the prior fire() calls don't get queued up, so no more calls to
  // the listener will occur unless we fire() again.
  scheduler_->PassTime(TimeDelta::FromSeconds(2));
  ASSERT_EQ(2, call_count_);

  // At this point, we've fired twice within a few seconds.  We can fire
  // (kMessagesPerMinute - 2) more times within a minute until we get
  // throttled.
  TimeDelta long_interval = TimeDelta::FromSeconds(3);
  for (int i = 0; i < kMessagesPerMinute - 2; ++i) {
    throttle->Fire();
    ASSERT_EQ(3 + i, call_count_);
    scheduler_->PassTime(long_interval);
    ASSERT_EQ(3 + i, call_count_);
  }

  // Now we've sent kMessagesPerMinute times.  If we fire again, nothing should
  // happen.
  throttle->Fire();
  scheduler_->PassTime(TimeDelta());
  ASSERT_EQ(kMessagesPerMinute, call_count_);

  // Now if we fire slowly, we still shouldn't make calls, since we'd violate
  // the larger rate limit interval.
  int fire_attempts =
      ((start_time_ + TimeDelta::FromMinutes(1) - scheduler_->GetCurrentTime())
          / long_interval) - 1;
  // This value should be 20.
  for (int i = 0; i < fire_attempts; ++i) {
    scheduler_->PassTime(long_interval);
    throttle->Fire();
    ASSERT_EQ(kMessagesPerMinute, call_count_);
  }

  Time time_to_send_again = start_time_ + TimeDelta::FromMinutes(1);
  scheduler_->PassTime(time_to_send_again - scheduler_->GetCurrentTime());

  ASSERT_EQ(kMessagesPerMinute + 1, call_count_);
}

/* Test that if we keep calling fire() every millisecond, we never violate the
 * rate limits, and the expected number of total events is allowed through.
 */
TEST_F(ThrottleTest, ThrottlingStorm) {
  scheduler_->StartScheduler();
  Closure* listener =
      NewPermanentCallback(this, &ThrottleTest::IncrementAndCheckRateLimits);

  // Throttler allowing one call per second and six per minute.
  scoped_ptr<Throttle> throttle(
      new Throttle(rate_limits_, scheduler_.get(), listener));

  // For five minutes, call Fire() every ten milliseconds, and make sure the
  // rate limits are respected.
  TimeDelta fine_interval = TimeDelta::FromMilliseconds(10);
  int duration_minutes = 5;
  TimeDelta duration = TimeDelta::FromMinutes(duration_minutes);
  int num_iterations = duration / fine_interval;
  for (int i = 0; i < num_iterations; ++i) {
    throttle->Fire();
    scheduler_->PassTime(fine_interval);
  }

  // Expect kMessagesPerMinute to be sent per minute for duration_minutes, plus
  // one extra because we end on the precise boundary at which the next message
  // is allowed to be sent.
  ASSERT_EQ((kMessagesPerMinute * duration_minutes) + 1, call_count_);
}

}  // namespace invalidation
