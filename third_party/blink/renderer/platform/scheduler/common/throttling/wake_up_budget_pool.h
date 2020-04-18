// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_WAKE_UP_BUDGET_POOL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_WAKE_UP_BUDGET_POOL_H_

#include "third_party/blink/renderer/platform/scheduler/common/throttling/budget_pool.h"

#include "base/macros.h"
#include "base/optional.h"

namespace blink {
namespace scheduler {

// WakeUpBudgetPool represents a collection of task queues which share a limit
// on total cpu time.
class PLATFORM_EXPORT WakeUpBudgetPool : public BudgetPool {
 public:
  WakeUpBudgetPool(const char* name,
                   BudgetPoolController* budget_pool_controller,
                   base::TimeTicks now);
  ~WakeUpBudgetPool() override;

  // Note: this does not have an immediate effect and should be called only
  // during initialization of a WakeUpBudgetPool.
  void SetWakeUpRate(double wake_ups_per_second);

  // Note: this does not have an immediate effect and should be called only
  // during initialization of a WakeUpBudgetPool.
  void SetWakeUpDuration(base::TimeDelta duration);

  // BudgetPool implementation:
  void RecordTaskRunTime(base::sequence_manager::TaskQueue* queue,
                         base::TimeTicks start_time,
                         base::TimeTicks end_time) final;
  bool CanRunTasksAt(base::TimeTicks moment, bool is_wake_up) const final;
  base::Optional<base::TimeTicks> GetTimeTasksCanRunUntil(
      base::TimeTicks now,
      bool is_wake_up) const final;
  base::TimeTicks GetNextAllowedRunTime(
      base::TimeTicks desired_run_time) const final;
  void OnQueueNextWakeUpChanged(base::sequence_manager::TaskQueue* queue,
                                base::TimeTicks now,
                                base::TimeTicks desired_run_time) final;
  void OnWakeUp(base::TimeTicks now) final;
  void AsValueInto(base::trace_event::TracedValue* state,
                   base::TimeTicks now) const final;

 protected:
  QueueBlockType GetBlockType() const final;

 private:
  base::TimeDelta wake_up_interval_;
  base::TimeDelta wake_up_duration_;

  base::Optional<base::TimeTicks> last_wake_up_;

  DISALLOW_COPY_AND_ASSIGN(WakeUpBudgetPool);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_COMMON_THROTTLING_WAKE_UP_BUDGET_POOL_H_
