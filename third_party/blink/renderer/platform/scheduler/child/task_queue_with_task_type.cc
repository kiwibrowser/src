// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"

namespace blink {
namespace scheduler {

scoped_refptr<TaskQueueWithTaskType> TaskQueueWithTaskType::Create(
    scoped_refptr<base::sequence_manager::TaskQueue> task_queue,
    TaskType task_type) {
  return base::WrapRefCounted(
      new TaskQueueWithTaskType(std::move(task_queue), task_type));
}

bool TaskQueueWithTaskType::RunsTasksInCurrentSequence() const {
  return task_queue_->RunsTasksInCurrentSequence();
}

TaskQueueWithTaskType::TaskQueueWithTaskType(
    scoped_refptr<base::sequence_manager::TaskQueue> task_queue,
    TaskType task_type)
    : task_queue_(std::move(task_queue)), task_type_(task_type) {}

TaskQueueWithTaskType::~TaskQueueWithTaskType() = default;

bool TaskQueueWithTaskType::PostDelayedTask(const base::Location& location,
                                            base::OnceClosure task,
                                            base::TimeDelta delay) {
  return task_queue_->PostTaskWithMetadata(
      base::sequence_manager::TaskQueue::PostedTask(
          std::move(task), location, delay, base::Nestable::kNestable,
          static_cast<int>(task_type_)));
}

bool TaskQueueWithTaskType::PostNonNestableDelayedTask(
    const base::Location& location,
    base::OnceClosure task,
    base::TimeDelta delay) {
  return task_queue_->PostTaskWithMetadata(
      base::sequence_manager::TaskQueue::PostedTask(
          std::move(task), location, delay, base::Nestable::kNonNestable,
          static_cast<int>(task_type_)));
}

}  // namespace scheduler
}  // namespace blink
