// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/worker/worker_thread_scheduler.h"

#include <memory>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager.h"
#include "third_party/blink/renderer/platform/scheduler/child/default_params.h"
#include "third_party/blink/renderer/platform/scheduler/child/features.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler_proxy.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"
#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_helper.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

namespace {
// Workers could be short-lived, set a shorter interval than
// the renderer thread.
constexpr base::TimeDelta kUnspecifiedWorkerThreadLoadTrackerReportingInterval =
    base::TimeDelta::FromSeconds(1);

void ReportWorkerTaskLoad(base::TimeTicks time, double load) {
  int load_percentage = static_cast<int>(load * 100);
  DCHECK_LE(load_percentage, 100);
  // TODO(kinuko): Maybe we also want to separately log when the associated
  // tab is in foreground and when not.
  UMA_HISTOGRAM_PERCENTAGE("WorkerScheduler.WorkerThreadLoad", load_percentage);
}

// TODO(scheduler-dev): Remove conversions when Blink starts using
// base::TimeTicks instead of doubles for time.
base::TimeTicks MonotonicTimeInSecondsToTimeTicks(
    double monotonic_time_in_seconds) {
  return base::TimeTicks() +
         base::TimeDelta::FromSecondsD(monotonic_time_in_seconds);
}

}  // namespace

WorkerThreadScheduler::WorkerThreadScheduler(
    WebThreadType thread_type,
    std::unique_ptr<base::sequence_manager::TaskQueueManager>
        task_queue_manager,
    WorkerSchedulerProxy* proxy)
    : NonMainThreadScheduler(std::make_unique<NonMainThreadSchedulerHelper>(
          std::move(task_queue_manager),
          this,
          TaskType::kWorkerThreadTaskQueueDefault)),
      idle_helper_(helper_.get(),
                   this,
                   "WorkerSchedulerIdlePeriod",
                   base::TimeDelta::FromMilliseconds(300),
                   helper_->NewTaskQueue(TaskQueue::Spec("worker_idle_tq"))),
      idle_canceled_delayed_task_sweeper_(helper_.get(),
                                          idle_helper_.IdleTaskRunner()),
      load_tracker_(helper_->NowTicks(),
                    base::BindRepeating(&ReportWorkerTaskLoad),
                    kUnspecifiedWorkerThreadLoadTrackerReportingInterval),
      throttling_state_(proxy ? proxy->throttling_state()
                              : FrameScheduler::ThrottlingState::kNotThrottled),
      worker_metrics_helper_(thread_type),
      default_task_runner_(TaskQueueWithTaskType::Create(
          helper_->DefaultWorkerTaskQueue(),
          TaskType::kWorkerThreadTaskQueueDefault)),
      weak_factory_(this) {
  thread_start_time_ = helper_->NowTicks();
  load_tracker_.Resume(thread_start_time_);
  helper_->AddTaskTimeObserver(this);

  if (proxy) {
    worker_metrics_helper_.SetParentFrameType(proxy->parent_frame_type());
    proxy->OnWorkerSchedulerCreated(GetWeakPtr());
  }

  if (thread_type == WebThreadType::kDedicatedWorkerThread &&
      base::FeatureList::IsEnabled(kDedicatedWorkerThrottling)) {
    CreateTaskQueueThrottler();
  }

  TRACE_EVENT_OBJECT_CREATED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("worker.scheduler"), "WorkerScheduler", this);
}

WorkerThreadScheduler::~WorkerThreadScheduler() {
  TRACE_EVENT_OBJECT_DELETED_WITH_ID(
      TRACE_DISABLED_BY_DEFAULT("worker.scheduler"), "WorkerScheduler", this);

  helper_->RemoveTaskTimeObserver(this);
}

scoped_refptr<base::SingleThreadTaskRunner>
WorkerThreadScheduler::DefaultTaskRunner() {
  return default_task_runner_;
}

scoped_refptr<SingleThreadIdleTaskRunner>
WorkerThreadScheduler::IdleTaskRunner() {
  DCHECK(initialized_);
  return idle_helper_.IdleTaskRunner();
}

scoped_refptr<base::SingleThreadTaskRunner>
WorkerThreadScheduler::IPCTaskRunner() {
  return base::ThreadTaskRunnerHandle::Get();
}

bool WorkerThreadScheduler::CanExceedIdleDeadlineIfRequired() const {
  DCHECK(initialized_);
  return idle_helper_.CanExceedIdleDeadlineIfRequired();
}

bool WorkerThreadScheduler::ShouldYieldForHighPriorityWork() {
  // We don't consider any work as being high priority on workers.
  return false;
}

void WorkerThreadScheduler::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  DCHECK(initialized_);
  helper_->AddTaskObserver(task_observer);
}

void WorkerThreadScheduler::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  DCHECK(initialized_);
  helper_->RemoveTaskObserver(task_observer);
}

void WorkerThreadScheduler::Shutdown() {
  DCHECK(initialized_);
  load_tracker_.RecordIdle(helper_->NowTicks());
  base::TimeTicks end_time = helper_->NowTicks();
  base::TimeDelta delta = end_time - thread_start_time_;

  // The lifetime could be radically different for different workers,
  // some workers could be short-lived (but last at least 1 sec in
  // Service Workers case) or could be around as long as the tab is open.
  UMA_HISTOGRAM_CUSTOM_TIMES(
      "WorkerThread.Runtime", delta, base::TimeDelta::FromSeconds(1),
      base::TimeDelta::FromDays(1), 50 /* bucket count */);
  task_queue_throttler_.reset();
  helper_->Shutdown();
}

scoped_refptr<WorkerTaskQueue> WorkerThreadScheduler::DefaultTaskQueue() {
  DCHECK(initialized_);
  return helper_->DefaultWorkerTaskQueue();
}

void WorkerThreadScheduler::InitImpl() {
  initialized_ = true;
  idle_helper_.EnableLongIdlePeriod();
}

void WorkerThreadScheduler::OnTaskCompleted(
    WorkerTaskQueue* worker_task_queue,
    const TaskQueue::Task& task,
    base::TimeTicks start,
    base::TimeTicks end,
    base::Optional<base::TimeDelta> thread_time) {
  worker_metrics_helper_.RecordTaskMetrics(worker_task_queue, task, start, end,
                                           thread_time);
}

SchedulerHelper* WorkerThreadScheduler::GetSchedulerHelperForTesting() {
  return helper_.get();
}

bool WorkerThreadScheduler::CanEnterLongIdlePeriod(base::TimeTicks,
                                                   base::TimeDelta*) {
  return true;
}

base::TimeTicks WorkerThreadScheduler::CurrentIdleTaskDeadlineForTesting()
    const {
  return idle_helper_.CurrentIdleTaskDeadline();
}

void WorkerThreadScheduler::WillProcessTask(double start_time) {}

void WorkerThreadScheduler::DidProcessTask(double start_time, double end_time) {
  base::TimeTicks start_time_ticks =
      MonotonicTimeInSecondsToTimeTicks(start_time);
  base::TimeTicks end_time_ticks = MonotonicTimeInSecondsToTimeTicks(end_time);

  load_tracker_.RecordTaskTime(start_time_ticks, end_time_ticks);
}

void WorkerThreadScheduler::OnThrottlingStateChanged(
    FrameScheduler::ThrottlingState throttling_state) {
  if (throttling_state_ == throttling_state)
    return;
  throttling_state_ = throttling_state;

  for (WorkerScheduler* worker_scheduler : worker_schedulers_)
    worker_scheduler->OnThrottlingStateChanged(throttling_state);
}

void WorkerThreadScheduler::RegisterWorkerScheduler(
    WorkerScheduler* worker_scheduler) {
  NonMainThreadScheduler::RegisterWorkerScheduler(worker_scheduler);
  worker_scheduler->OnThrottlingStateChanged(throttling_state_);
}

scoped_refptr<WorkerTaskQueue> WorkerThreadScheduler::ControlTaskQueue() {
  return helper_->ControlWorkerTaskQueue();
}

base::WeakPtr<WorkerThreadScheduler> WorkerThreadScheduler::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void WorkerThreadScheduler::CreateTaskQueueThrottler() {
  if (task_queue_throttler_)
    return;
  task_queue_throttler_ = std::make_unique<TaskQueueThrottler>(
      this, &traceable_variable_controller_);
  wake_up_budget_pool_ =
      task_queue_throttler_->CreateWakeUpBudgetPool("worker_wake_up_pool");
}

}  // namespace scheduler
}  // namespace blink
