// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/common/scheduler_helper.h"

#include <utility>

#include "base/time/default_tick_clock.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::RealTimeDomain;
using base::sequence_manager::TaskQueue;
using base::sequence_manager::TaskQueueManager;
using base::sequence_manager::TaskTimeObserver;
using base::sequence_manager::TimeDomain;

SchedulerHelper::SchedulerHelper(
    std::unique_ptr<TaskQueueManager> task_queue_manager)
    : task_queue_manager_(std::move(task_queue_manager)), observer_(nullptr) {
  task_queue_manager_->SetWorkBatchSize(4);
}

void SchedulerHelper::InitDefaultQueues(
    scoped_refptr<TaskQueue> default_task_queue,
    scoped_refptr<TaskQueue> control_task_queue,
    TaskType default_task_type) {
  control_task_queue->SetQueuePriority(TaskQueue::kControlPriority);

  DCHECK(task_queue_manager_);
  task_queue_manager_->SetDefaultTaskRunner(
      TaskQueueWithTaskType::Create(default_task_queue, default_task_type));
}

SchedulerHelper::~SchedulerHelper() {
  Shutdown();
}

void SchedulerHelper::Shutdown() {
  CheckOnValidThread();
  if (!task_queue_manager_)
    return;
  task_queue_manager_->SetObserver(nullptr);
  task_queue_manager_.reset();
}

void SchedulerHelper::SetWorkBatchSizeForTesting(size_t work_batch_size) {
  CheckOnValidThread();
  DCHECK(task_queue_manager_.get());
  task_queue_manager_->SetWorkBatchSize(work_batch_size);
}

bool SchedulerHelper::GetAndClearSystemIsQuiescentBit() {
  CheckOnValidThread();
  DCHECK(task_queue_manager_.get());
  return task_queue_manager_->GetAndClearSystemIsQuiescentBit();
}

void SchedulerHelper::AddTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  CheckOnValidThread();
  if (task_queue_manager_)
    task_queue_manager_->AddTaskObserver(task_observer);
}

void SchedulerHelper::RemoveTaskObserver(
    base::MessageLoop::TaskObserver* task_observer) {
  CheckOnValidThread();
  if (task_queue_manager_)
    task_queue_manager_->RemoveTaskObserver(task_observer);
}

void SchedulerHelper::AddTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  if (task_queue_manager_)
    task_queue_manager_->AddTaskTimeObserver(task_time_observer);
}

void SchedulerHelper::RemoveTaskTimeObserver(
    TaskTimeObserver* task_time_observer) {
  if (task_queue_manager_)
    task_queue_manager_->RemoveTaskTimeObserver(task_time_observer);
}

void SchedulerHelper::SetObserver(Observer* observer) {
  CheckOnValidThread();
  observer_ = observer;
  DCHECK(task_queue_manager_);
  task_queue_manager_->SetObserver(this);
}

void SchedulerHelper::SweepCanceledDelayedTasks() {
  CheckOnValidThread();
  DCHECK(task_queue_manager_);
  task_queue_manager_->SweepCanceledDelayedTasks();
}

RealTimeDomain* SchedulerHelper::real_time_domain() const {
  CheckOnValidThread();
  DCHECK(task_queue_manager_);
  return task_queue_manager_->GetRealTimeDomain();
}

void SchedulerHelper::RegisterTimeDomain(TimeDomain* time_domain) {
  CheckOnValidThread();
  DCHECK(task_queue_manager_);
  task_queue_manager_->RegisterTimeDomain(time_domain);
}

void SchedulerHelper::UnregisterTimeDomain(TimeDomain* time_domain) {
  CheckOnValidThread();
  if (task_queue_manager_)
    task_queue_manager_->UnregisterTimeDomain(time_domain);
}

void SchedulerHelper::OnBeginNestedRunLoop() {
  if (observer_)
    observer_->OnBeginNestedRunLoop();
}

void SchedulerHelper::OnExitNestedRunLoop() {
  if (observer_)
    observer_->OnExitNestedRunLoop();
}

const base::TickClock* SchedulerHelper::GetClock() const {
  return task_queue_manager_->GetClock();
}

base::TimeTicks SchedulerHelper::NowTicks() const {
  if (task_queue_manager_)
    return task_queue_manager_->NowTicks();
  // We may need current time for tracing when shutting down worker thread.
  return base::TimeTicks::Now();
}

double SchedulerHelper::GetSamplingRateForRecordingCPUTime() const {
  if (task_queue_manager_)
    return task_queue_manager_->GetSamplingRateForRecordingCPUTime();
  return 0;
}

}  // namespace scheduler
}  // namespace blink
