// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_thread_type.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_time_observer.h"
#include "third_party/blink/renderer/platform/scheduler/child/idle_canceled_delayed_task_sweeper.h"
#include "third_party/blink/renderer/platform/scheduler/child/idle_helper.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_metrics_helper.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/non_main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/util/task_duration_metric_reporter.h"
#include "third_party/blink/renderer/platform/scheduler/util/thread_load_tracker.h"

namespace base {
namespace sequence_manager {
class TaskQueueManager;
}
}  // namespace base

namespace blink {
namespace scheduler {

class WorkerSchedulerProxy;

class PLATFORM_EXPORT WorkerThreadScheduler
    : public NonMainThreadScheduler,
      public IdleHelper::Delegate,
      public base::sequence_manager::TaskTimeObserver {
 public:
  WorkerThreadScheduler(
      WebThreadType thread_type,
      std::unique_ptr<base::sequence_manager::TaskQueueManager>
          task_queue_manager,
      WorkerSchedulerProxy* proxy);
  ~WorkerThreadScheduler() override;

  // WebThreadScheduler implementation:
  scoped_refptr<base::SingleThreadTaskRunner> DefaultTaskRunner() override;
  scoped_refptr<SingleThreadIdleTaskRunner> IdleTaskRunner() override;
  scoped_refptr<base::SingleThreadTaskRunner> IPCTaskRunner() override;
  bool ShouldYieldForHighPriorityWork() override;
  bool CanExceedIdleDeadlineIfRequired() const override;
  void AddTaskObserver(base::MessageLoop::TaskObserver* task_observer) override;
  void RemoveTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override;
  void Shutdown() override;

  // NonMainThreadScheduler implementation:
  scoped_refptr<WorkerTaskQueue> DefaultTaskQueue() override;
  void OnTaskCompleted(WorkerTaskQueue* worker_task_queue,
                       const base::sequence_manager::TaskQueue::Task& task,
                       base::TimeTicks start,
                       base::TimeTicks end,
                       base::Optional<base::TimeDelta> thread_time) override;

  // TaskTimeObserver implementation:
  void WillProcessTask(double start_time) override;
  void DidProcessTask(double start_time, double end_time) override;

  SchedulerHelper* GetSchedulerHelperForTesting();
  base::TimeTicks CurrentIdleTaskDeadlineForTesting() const;

  // Virtual for test.
  virtual void OnThrottlingStateChanged(
      FrameScheduler::ThrottlingState throttling_state);

  // Returns the control task queue.  Tasks posted to this queue are executed
  // with the highest priority. Care must be taken to avoid starvation of other
  // task queues.
  scoped_refptr<WorkerTaskQueue> ControlTaskQueue();

 protected:
  // NonMainThreadScheduler implementation:
  void InitImpl() override;

  // IdleHelper::Delegate implementation:
  bool CanEnterLongIdlePeriod(
      base::TimeTicks now,
      base::TimeDelta* next_long_idle_period_delay_out) override;
  void IsNotQuiescent() override {}
  void OnIdlePeriodStarted() override {}
  void OnIdlePeriodEnded() override {}
  void OnPendingTasksChanged(bool new_state) override {}

  FrameScheduler::ThrottlingState throttling_state() const {
    return throttling_state_;
  }

  void RegisterWorkerScheduler(WorkerScheduler* worker_scheduler) override;

  void CreateTaskQueueThrottler();

 private:
  void MaybeStartLongIdlePeriod();

  base::WeakPtr<WorkerThreadScheduler> GetWeakPtr();

  IdleHelper idle_helper_;
  IdleCanceledDelayedTaskSweeper idle_canceled_delayed_task_sweeper_;
  ThreadLoadTracker load_tracker_;
  bool initialized_;
  base::TimeTicks thread_start_time_;
  scoped_refptr<WorkerTaskQueue> control_task_queue_;
  FrameScheduler::ThrottlingState throttling_state_;

  WorkerMetricsHelper worker_metrics_helper_;

  scoped_refptr<base::SingleThreadTaskRunner> default_task_runner_;

  base::WeakPtrFactory<WorkerThreadScheduler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WorkerThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_
