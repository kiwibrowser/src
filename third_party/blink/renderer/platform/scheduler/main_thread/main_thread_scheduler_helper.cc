// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_helper.h"

#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_task_queue.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

MainThreadSchedulerHelper::MainThreadSchedulerHelper(
    std::unique_ptr<base::sequence_manager::TaskQueueManager>
        task_queue_manager,
    MainThreadSchedulerImpl* main_thread_scheduler)
    : SchedulerHelper(std::move(task_queue_manager)),
      main_thread_scheduler_(main_thread_scheduler),
      default_task_queue_(
          NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
                           MainThreadTaskQueue::QueueType::kDefault)
                           .SetShouldMonitorQuiescence(true))),
      control_task_queue_(
          NewTaskQueue(MainThreadTaskQueue::QueueCreationParams(
                           MainThreadTaskQueue::QueueType::kControl)
                           .SetShouldNotifyObservers(false))) {
  InitDefaultQueues(default_task_queue_, control_task_queue_,
                    TaskType::kMainThreadTaskQueueDefault);
  task_queue_manager_->EnableCrashKeys("blink_scheduler_task_file_name",
                                       "blink_scheduler_task_function_name");
}

MainThreadSchedulerHelper::~MainThreadSchedulerHelper() {
  control_task_queue_->ShutdownTaskQueue();
  default_task_queue_->ShutdownTaskQueue();
}

scoped_refptr<MainThreadTaskQueue>
MainThreadSchedulerHelper::DefaultMainThreadTaskQueue() {
  return default_task_queue_;
}

scoped_refptr<TaskQueue> MainThreadSchedulerHelper::DefaultTaskQueue() {
  return default_task_queue_;
}

scoped_refptr<MainThreadTaskQueue>
MainThreadSchedulerHelper::ControlMainThreadTaskQueue() {
  return control_task_queue_;
}

scoped_refptr<TaskQueue> MainThreadSchedulerHelper::ControlTaskQueue() {
  return control_task_queue_;
}

scoped_refptr<MainThreadTaskQueue> MainThreadSchedulerHelper::NewTaskQueue(
    const MainThreadTaskQueue::QueueCreationParams& params) {
  scoped_refptr<MainThreadTaskQueue> task_queue =
      task_queue_manager_->CreateTaskQueue<MainThreadTaskQueue>(
          params.spec, params, main_thread_scheduler_);
  if (params.fixed_priority)
    task_queue->SetQueuePriority(params.fixed_priority.value());
  if (params.used_for_important_tasks)
    task_queue->SetQueuePriority(TaskQueue::QueuePriority::kHighestPriority);
  return task_queue;
}

}  // namespace scheduler
}  // namespace blink
