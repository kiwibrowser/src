// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_MEMORY_CONDITION_OBSERVER_H_
#define CONTENT_BROWSER_MEMORY_MEMORY_CONDITION_OBSERVER_H_

#include "base/cancelable_callback.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "content/browser/memory/memory_coordinator_impl.h"
#include "content/common/content_export.h"

namespace content {

// MemoryConditionObserver observes system memory usage and determines the
// current MemoryCondition. It dispatches the current condition if the condition
// has changed.
class CONTENT_EXPORT MemoryConditionObserver {
 public:
  // |coordinator| must outlive than this instance.
  MemoryConditionObserver(
      MemoryCoordinatorImpl* coordinator,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~MemoryConditionObserver();

  // Schedules a task to update memory condition. The task will be executed
  // after |delay| has passed.
  void ScheduleUpdateCondition(base::TimeDelta delay);

  // Called when the browser is foregrounded.
  void OnForegrounded();

  // Called when the browser is backgrounded.
  void OnBackgrounded();

 private:
  FRIEND_TEST_ALL_PREFIXES(MemoryCoordinatorImplTest, CalculateNextCondition);
  FRIEND_TEST_ALL_PREFIXES(MemoryCoordinatorImplTest, ForceSetMemoryCondition);
  FRIEND_TEST_ALL_PREFIXES(MemoryCoordinatorImplTest, DiscardTabUnderCritical);

  // Sets the monitoring interval and reschedules a task to update memory
  // condition.
  void SetMonitoringInterval(base::TimeDelta interval);

  // Calculates next memory condition from the amount of free memory using
  // a heuristic.
  MemoryCondition CalculateNextCondition();

  // Periodically called to update the memory condition.
  void UpdateCondition();

  MemoryCoordinatorImpl* coordinator_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::CancelableClosure update_condition_closure_;

  // The current interval of checking the amount of free memory.
  base::TimeDelta monitoring_interval_;
  // The value of |monitoring_interval_| when the browser is foregrounded.
  base::TimeDelta monitoring_interval_foregrounded_;
  // The value of |monitoring_interval_| when the browser is backgrounded.
  base::TimeDelta monitoring_interval_backgrounded_;

  DISALLOW_COPY_AND_ASSIGN(MemoryConditionObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_MEMORY_CONDITION_OBSERVER_H_
