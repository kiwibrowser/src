// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_TASK_COST_ESTIMATOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_TASK_COST_ESTIMATOR_H_

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "cc/base/rolling_time_delta_history.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace base {
class TickClock;
}

namespace blink {
namespace scheduler {

// Estimates the cost of running tasks based on historical timing data.
class PLATFORM_EXPORT TaskCostEstimator
    : public base::MessageLoop::TaskObserver {
 public:
  TaskCostEstimator(const base::TickClock* time_source,
                    int sample_count,
                    double estimation_percentile);
  ~TaskCostEstimator() override;

  base::TimeDelta expected_task_duration() const;

  // TaskObserver implementation:
  void WillProcessTask(const base::PendingTask& pending_task) override;
  void DidProcessTask(const base::PendingTask& pending_task) override;

  void Clear();

 private:
  cc::RollingTimeDeltaHistory rolling_time_delta_history_;
  const base::TickClock* time_source_;  // NOT OWNED
  int outstanding_task_count_;
  double estimation_percentile_;
  base::TimeTicks task_start_time_;
  base::TimeDelta expected_task_duration_;

  DISALLOW_COPY_AND_ASSIGN(TaskCostEstimator);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_MAIN_THREAD_TASK_COST_ESTIMATOR_H_
