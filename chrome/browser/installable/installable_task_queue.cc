// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_task_queue.h"

InstallableTask::InstallableTask() {}
InstallableTask::InstallableTask(const InstallableParams& params,
                                 const InstallableCallback& callback)
    : params(params), callback(callback) {}
InstallableTask::~InstallableTask() {}
InstallableTask::InstallableTask(const InstallableTask& other) = default;
InstallableTask& InstallableTask::operator=(const InstallableTask& other) =
    default;

InstallableTaskQueue::InstallableTaskQueue() {}
InstallableTaskQueue::~InstallableTaskQueue() {}

void InstallableTaskQueue::Add(InstallableTask task) {
  tasks_.push_back(task);
}

void InstallableTaskQueue::PauseCurrent() {
  paused_tasks_.push_back(Current());
  Next();
}

void InstallableTaskQueue::UnpauseAll() {
  for (const auto& task : paused_tasks_)
    Add(task);

  paused_tasks_.clear();
}

bool InstallableTaskQueue::HasCurrent() const {
  return !tasks_.empty();
}

bool InstallableTaskQueue::HasPaused() const {
  return !paused_tasks_.empty();
}

InstallableTask& InstallableTaskQueue::Current() {
  DCHECK(!tasks_.empty());
  return tasks_[0];
}

void InstallableTaskQueue::Next() {
  DCHECK(!tasks_.empty());
  tasks_.erase(tasks_.begin());
}

void InstallableTaskQueue::Reset() {
  tasks_.clear();
  paused_tasks_.clear();
}
