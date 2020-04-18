// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/public/non_main_thread_scheduler.h"

#include <utility>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/wake_up_budget_pool.h"
#include "third_party/blink/renderer/platform/scheduler/worker/worker_thread_scheduler.h"

namespace blink {
namespace scheduler {

NonMainThreadScheduler::NonMainThreadScheduler(
    std::unique_ptr<NonMainThreadSchedulerHelper> helper)
    : helper_(std::move(helper)) {}

NonMainThreadScheduler::~NonMainThreadScheduler() {
  DCHECK(worker_schedulers_.empty());
}

// static
std::unique_ptr<NonMainThreadScheduler> NonMainThreadScheduler::Create(
    WebThreadType thread_type,
    WorkerSchedulerProxy* proxy) {
  return std::make_unique<WorkerThreadScheduler>(
      thread_type,
      base::sequence_manager::TaskQueueManager::TakeOverCurrentThread(), proxy);
}

// static
NonMainThreadScheduler* NonMainThreadScheduler::Current() {
  DCHECK_NE(Platform::Current()->CurrentThread(),
            Platform::Current()->MainThread());
  return ThreadScheduler::Current()->AsNonMainThreadScheduler();
}

void NonMainThreadScheduler::Init() {
  InitImpl();

  // DefaultTaskQueue() is a virtual function, so it can't be called in the
  // constructor. Also, DefaultTaskQueue() checks if InitImpl() is called.
  // Therefore, v8_task_runner_ needs to be initialized here.
  v8_task_runner_ = TaskQueueWithTaskType::Create(
      DefaultTaskQueue(), TaskType::kMainThreadTaskQueueV8);
}

scoped_refptr<WorkerTaskQueue> NonMainThreadScheduler::CreateTaskRunner() {
  helper_->CheckOnValidThread();
  return helper_->NewTaskQueue(
      base::sequence_manager::TaskQueue::Spec("worker_tq")
          .SetShouldMonitorQuiescence(true)
          .SetTimeDomain(nullptr));
}

void NonMainThreadScheduler::RunIdleTask(blink::WebThread::IdleTask task,
                                         base::TimeTicks deadline) {
  std::move(task).Run((deadline - base::TimeTicks()).InSecondsF());
}

void NonMainThreadScheduler::PostIdleTask(const base::Location& location,
                                          blink::WebThread::IdleTask task) {
  IdleTaskRunner()->PostIdleTask(
      location,
      base::BindOnce(&NonMainThreadScheduler::RunIdleTask, std::move(task)));
}

void NonMainThreadScheduler::PostNonNestableIdleTask(
    const base::Location& location,
    blink::WebThread::IdleTask task) {
  IdleTaskRunner()->PostNonNestableIdleTask(
      location,
      base::BindOnce(&NonMainThreadScheduler::RunIdleTask, std::move(task)));
}

scoped_refptr<base::SingleThreadTaskRunner>
NonMainThreadScheduler::V8TaskRunner() {
  return v8_task_runner_;
}

scoped_refptr<base::SingleThreadTaskRunner>
NonMainThreadScheduler::CompositorTaskRunner() {
  return nullptr;
}

std::unique_ptr<blink::PageScheduler>
NonMainThreadScheduler::CreatePageScheduler(PageScheduler::Delegate* delegate) {
  NOTREACHED();
  return nullptr;
}

std::unique_ptr<NonMainThreadScheduler::RendererPauseHandle>
NonMainThreadScheduler::PauseScheduler() {
  return nullptr;
}

base::TimeTicks NonMainThreadScheduler::MonotonicallyIncreasingVirtualTime() {
  return base::TimeTicks::Now();
}

void NonMainThreadScheduler::RegisterWorkerScheduler(
    WorkerScheduler* worker_scheduler) {
  worker_schedulers_.insert(worker_scheduler);
}

void NonMainThreadScheduler::UnregisterWorkerScheduler(
    WorkerScheduler* worker_scheduler) {
  DCHECK(worker_schedulers_.find(worker_scheduler) != worker_schedulers_.end());
  worker_schedulers_.erase(worker_scheduler);
}

scoped_refptr<base::SingleThreadTaskRunner>
NonMainThreadScheduler::ControlTaskRunner() {
  return helper_->ControlWorkerTaskQueue();
}

void NonMainThreadScheduler::RegisterTimeDomain(
    base::sequence_manager::TimeDomain* time_domain) {
  return helper_->RegisterTimeDomain(time_domain);
}

void NonMainThreadScheduler::UnregisterTimeDomain(
    base::sequence_manager::TimeDomain* time_domain) {
  return helper_->UnregisterTimeDomain(time_domain);
}

base::sequence_manager::TimeDomain*
NonMainThreadScheduler::GetActiveTimeDomain() {
  return helper_->real_time_domain();
}

const base::TickClock* NonMainThreadScheduler::GetTickClock() {
  return helper_->GetClock();
}

}  // namespace scheduler
}  // namespace blink
