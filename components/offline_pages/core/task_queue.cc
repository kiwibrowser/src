// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/task_queue.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

namespace offline_pages {

TaskQueue::TaskQueue(Delegate* delegate)
    : delegate_(delegate), weak_ptr_factory_(this) {
  DCHECK(delegate_);
}

TaskQueue::~TaskQueue() {}

void TaskQueue::AddTask(std::unique_ptr<Task> task) {
  task->SetTaskCompletionCallback(
      base::ThreadTaskRunnerHandle::Get(),
      base::Bind(&TaskQueue::TaskCompleted, weak_ptr_factory_.GetWeakPtr()));
  tasks_.push(std::move(task));
  StartTaskIfAvailable();
}

bool TaskQueue::HasPendingTasks() const {
  return !tasks_.empty() || HasRunningTask();
}

bool TaskQueue::HasRunningTask() const {
  return current_task_ != nullptr;
}

void TaskQueue::StartTaskIfAvailable() {
  DVLOG(2) << "running? " << HasRunningTask() << ", pending? "
           << HasPendingTasks() << " " << __func__;
  if (HasRunningTask())
    return;

  if (!HasPendingTasks()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&TaskQueue::InformTaskQueueIsIdle,
                              weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  current_task_ = std::move(tasks_.front());
  tasks_.pop();
  current_task_->Run();
}

void TaskQueue::TaskCompleted(Task* task) {
  DCHECK_EQ(task, current_task_.get());
  if (task == current_task_.get()) {
    current_task_.reset(nullptr);
    StartTaskIfAvailable();
  }
}

void TaskQueue::InformTaskQueueIsIdle() {
  delegate_->OnTaskQueueIsIdle();
}

}  // namespace offline_pages
