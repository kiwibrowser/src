// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_NON_MAIN_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_NON_MAIN_THREAD_SCHEDULER_H_

#include <memory>

#include "base/macros.h"
#include "third_party/blink/public/platform/scheduler/single_thread_idle_task_runner.h"
#include "third_party/blink/public/platform/scheduler/web_thread_scheduler.h"
#include "third_party/blink/public/platform/web_thread_type.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/common/thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/util/tracing_helper.h"
#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_helper.h"

namespace blink {
namespace scheduler {
class TaskQueueWithTaskType;
class WorkerSchedulerProxy;
class WorkerScheduler;
class TaskQueueThrottler;
class WakeUpBudgetPool;

// TODO(yutak): Rename this class to NonMainThreadSchedulerImpl and consider
// changing all non-impl scheduler classes to have only static methods.
class PLATFORM_EXPORT NonMainThreadScheduler : public ThreadSchedulerImpl {
 public:
  ~NonMainThreadScheduler() override;

  static std::unique_ptr<NonMainThreadScheduler> Create(
      WebThreadType thread_type,
      WorkerSchedulerProxy* proxy);

  // Same as ThreadScheduler::Current(), but this asserts the caller is on
  // a non-main thread.
  static NonMainThreadScheduler* Current();

  // Blink should use NonMainThreadScheduler::DefaultTaskQueue instead of
  // WebThreadScheduler::DefaultTaskRunner.
  virtual scoped_refptr<WorkerTaskQueue> DefaultTaskQueue() = 0;

  // Must be called before the scheduler can be used. Does any post construction
  // initialization needed such as initializing idle period detection.
  void Init();

  virtual void OnTaskCompleted(
      WorkerTaskQueue* worker_task_queue,
      const base::sequence_manager::TaskQueue::Task& task,
      base::TimeTicks start,
      base::TimeTicks end,
      base::Optional<base::TimeDelta> thread_time) = 0;

  // ThreadSchedulerImpl:
  scoped_refptr<base::SingleThreadTaskRunner> ControlTaskRunner() override;
  void RegisterTimeDomain(
      base::sequence_manager::TimeDomain* time_domain) override;
  void UnregisterTimeDomain(
      base::sequence_manager::TimeDomain* time_domain) override;
  base::sequence_manager::TimeDomain* GetActiveTimeDomain() override;
  const base::TickClock* GetTickClock() override;

  // ThreadScheduler implementation.
  // TODO(yutak): Some functions are only meaningful in main thread. Move them
  // to MainThreadScheduler.
  void PostIdleTask(const base::Location& location,
                    WebThread::IdleTask task) override;
  void PostNonNestableIdleTask(const base::Location& location,
                               WebThread::IdleTask task) override;
  scoped_refptr<base::SingleThreadTaskRunner> V8TaskRunner() override;
  scoped_refptr<base::SingleThreadTaskRunner> CompositorTaskRunner() override;
  std::unique_ptr<PageScheduler> CreatePageScheduler(
      PageScheduler::Delegate*) override;
  std::unique_ptr<RendererPauseHandle> PauseScheduler() override
      WARN_UNUSED_RESULT;

  // Returns TimeTicks::Now() by default.
  base::TimeTicks MonotonicallyIncreasingVirtualTime() override;

  NonMainThreadScheduler* AsNonMainThreadScheduler() override { return this; }

  // The following virtual methods are defined in *both* WebThreadScheduler
  // and ThreadScheduler, with identical interfaces and semantics. They are
  // overriden in a subclass, effectively implementing the virtual methods
  // in both classes at the same time. This is allowed in C++, as long as
  // there is only one final overrider (i.e. definitions in base classes are
  // not used in instantiated objects, since otherwise they may have multiple
  // definitions of the virtual function in question).
  //
  // virtual void Shutdown();

  scoped_refptr<WorkerTaskQueue> CreateTaskRunner();

  // TaskQueueThrottler might be null if throttling is not enabled or
  // not supported.
  TaskQueueThrottler* task_queue_throttler() const {
    return task_queue_throttler_.get();
  }
  WakeUpBudgetPool* wake_up_budget_pool() const { return wake_up_budget_pool_; }

 protected:
  explicit NonMainThreadScheduler(
      std::unique_ptr<NonMainThreadSchedulerHelper> helper);

  friend class WorkerScheduler;

  // Each WorkerScheduler should notify NonMainThreadScheduler when it is
  // created or destroyed.
  virtual void RegisterWorkerScheduler(WorkerScheduler* worker_scheduler);
  virtual void UnregisterWorkerScheduler(WorkerScheduler* worker_scheduler);

  // Called during Init() for delayed initialization for subclasses.
  virtual void InitImpl() = 0;

  // This controller should be initialized before any TraceableVariables
  // because they require one to initialize themselves.
  TraceableVariableController traceable_variable_controller_;

  std::unique_ptr<NonMainThreadSchedulerHelper> helper_;

  // Worker schedulers associated with this thread.
  std::unordered_set<WorkerScheduler*> worker_schedulers_;

  std::unique_ptr<TaskQueueThrottler> task_queue_throttler_;
  // Owned by |task_queue_throttler_|.
  WakeUpBudgetPool* wake_up_budget_pool_ = nullptr;

 private:
  static void RunIdleTask(WebThread::IdleTask task, base::TimeTicks deadline);
  scoped_refptr<TaskQueueWithTaskType> v8_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(NonMainThreadScheduler);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_NON_MAIN_THREAD_SCHEDULER_H_
