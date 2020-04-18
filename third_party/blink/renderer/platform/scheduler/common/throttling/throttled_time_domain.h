// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_THROTTLED_TIME_DOMAIN_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_THROTTLED_TIME_DOMAIN_H_

#include "base/macros.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"

namespace blink {
namespace scheduler {

// A time domain for throttled tasks. behaves like an RealTimeDomain except it
// relies on the owner (TaskQueueThrottler) to schedule wake-ups.
class PLATFORM_EXPORT ThrottledTimeDomain
    : public base::sequence_manager::RealTimeDomain {
 public:
  ThrottledTimeDomain();
  ~ThrottledTimeDomain() override;

  void SetNextTaskRunTime(base::TimeTicks run_time);

  // TimeDomain implementation:
  const char* GetName() const override;
  void RequestWakeUpAt(base::TimeTicks now, base::TimeTicks run_time) override;
  void CancelWakeUpAt(base::TimeTicks run_time) override;
  base::Optional<base::TimeDelta> DelayTillNextTask(
      base::sequence_manager::LazyNow* lazy_now) override;

  using TimeDomain::WakeUpReadyDelayedQueues;

 private:
  // Next task run time provided by task queue throttler. Note that it does not
  // get reset, so it is valid only when in the future.
  base::Optional<base::TimeTicks> next_task_run_time_;

  DISALLOW_COPY_AND_ASSIGN(ThrottledTimeDomain);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_THROTTLED_TIME_DOMAIN_H_
