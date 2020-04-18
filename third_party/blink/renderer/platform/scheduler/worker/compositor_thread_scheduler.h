// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_COMPOSITOR_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_COMPOSITOR_THREAD_SCHEDULER_H_

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/scheduler/single_thread_idle_task_runner.h"
#include "third_party/blink/public/platform/web_thread_type.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/child/compositor_metrics_helper.h"
#include "third_party/blink/renderer/platform/scheduler/public/non_main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/util/task_duration_metric_reporter.h"

namespace base {
class Thread;
}

namespace blink {
namespace scheduler {

class PLATFORM_EXPORT CompositorThreadScheduler
    : public NonMainThreadScheduler,
      public SingleThreadIdleTaskRunner::Delegate {
 public:
  CompositorThreadScheduler(
      base::Thread* thread,
      std::unique_ptr<base::sequence_manager::TaskQueueManager>
          task_queue_manager);

  ~CompositorThreadScheduler() override;

  // NonMainThreadScheduler:
  scoped_refptr<WorkerTaskQueue> DefaultTaskQueue() override;
  void OnTaskCompleted(WorkerTaskQueue* worker_task_queue,
                       const base::sequence_manager::TaskQueue::Task& task,
                       base::TimeTicks start,
                       base::TimeTicks end,
                       base::Optional<base::TimeDelta> thread_time) override;

  // WebThreadScheduler:
  scoped_refptr<base::SingleThreadTaskRunner> DefaultTaskRunner() override;
  scoped_refptr<scheduler::SingleThreadIdleTaskRunner> IdleTaskRunner()
      override;
  scoped_refptr<base::SingleThreadTaskRunner> IPCTaskRunner() override;
  bool ShouldYieldForHighPriorityWork() override;
  bool CanExceedIdleDeadlineIfRequired() const override;
  void AddTaskObserver(base::MessageLoop::TaskObserver* task_observer) override;
  void RemoveTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override;
  void Shutdown() override;

  // SingleThreadIdleTaskRunner::Delegate:
  void OnIdleTaskPosted() override;
  base::TimeTicks WillProcessIdleTask() override;
  void DidProcessIdleTask() override;
  base::TimeTicks NowTicks() override;

 protected:
  // NonMainThreadScheduler:
  void InitImpl() override;

 private:
  base::Thread* thread_;

  CompositorMetricsHelper compositor_metrics_helper_;
  scoped_refptr<base::SingleThreadTaskRunner> default_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(CompositorThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_COMPOSITOR_THREAD_SCHEDULER_H_
