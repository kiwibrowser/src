// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_helper.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_task_queue.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

NonMainThreadSchedulerHelper::NonMainThreadSchedulerHelper(
    std::unique_ptr<base::sequence_manager::TaskQueueManager>
        task_queue_manager,
    NonMainThreadScheduler* non_main_thread_scheduler,
    TaskType default_task_type)
    : SchedulerHelper(std::move(task_queue_manager)),
      non_main_thread_scheduler_(non_main_thread_scheduler),
      default_task_queue_(NewTaskQueue(TaskQueue::Spec("worker_default_tq")
                                           .SetShouldMonitorQuiescence(true))),
      control_task_queue_(NewTaskQueue(TaskQueue::Spec("worker_control_tq")
                                           .SetShouldNotifyObservers(false))) {
  InitDefaultQueues(default_task_queue_, control_task_queue_,
                    default_task_type);
}

NonMainThreadSchedulerHelper::~NonMainThreadSchedulerHelper() {
  control_task_queue_->ShutdownTaskQueue();
  default_task_queue_->ShutdownTaskQueue();
}

scoped_refptr<WorkerTaskQueue>
NonMainThreadSchedulerHelper::DefaultWorkerTaskQueue() {
  return default_task_queue_;
}

scoped_refptr<TaskQueue> NonMainThreadSchedulerHelper::DefaultTaskQueue() {
  return default_task_queue_;
}

scoped_refptr<WorkerTaskQueue>
NonMainThreadSchedulerHelper::ControlWorkerTaskQueue() {
  return control_task_queue_;
}

scoped_refptr<TaskQueue> NonMainThreadSchedulerHelper::ControlTaskQueue() {
  return control_task_queue_;
}

scoped_refptr<WorkerTaskQueue> NonMainThreadSchedulerHelper::NewTaskQueue(
    const TaskQueue::Spec& spec) {
  return task_queue_manager_->CreateTaskQueue<WorkerTaskQueue>(
      spec, non_main_thread_scheduler_);
}

}  // namespace scheduler
}  // namespace blink
