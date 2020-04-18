// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/task.h"

#include "base/bind.h"
#include "base/threading/thread_task_runner_handle.h"

namespace offline_pages {

Task::Task() {}

Task::~Task() {}

void Task::SetTaskCompletionCallbackForTesting(
    scoped_refptr<base::SingleThreadTaskRunner> task_completion_runner,
    const TaskCompletionCallback& task_completion_callback) {
  SetTaskCompletionCallback(task_completion_runner, task_completion_callback);
}

void Task::SetTaskCompletionCallback(
    scoped_refptr<base::SingleThreadTaskRunner> task_completion_runner,
    const TaskCompletionCallback& task_completion_callback) {
  // Attempts to enforce that SetTaskCompletionCallback is at most called once
  // and enforces that reasonable values are set once that happens.
  DCHECK(!task_completion_runner_);
  DCHECK(task_completion_runner);
  DCHECK(task_completion_callback_.is_null());
  DCHECK(!task_completion_callback.is_null());
  task_completion_runner_ = task_completion_runner;
  task_completion_callback_ = task_completion_callback;
}

void Task::TaskComplete() {
  if (task_completion_callback_.is_null() || !task_completion_runner_)
    return;

  task_completion_runner_->PostTask(
      FROM_HERE, base::Bind(task_completion_callback_, this));
}

}  // namespace offline_pages
