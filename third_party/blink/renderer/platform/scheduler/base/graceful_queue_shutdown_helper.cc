// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"

namespace base {
namespace sequence_manager {
namespace internal {

GracefulQueueShutdownHelper::GracefulQueueShutdownHelper()
    : task_queue_manager_deleted_(false) {}

GracefulQueueShutdownHelper::~GracefulQueueShutdownHelper() = default;

void GracefulQueueShutdownHelper::GracefullyShutdownTaskQueue(
    std::unique_ptr<internal::TaskQueueImpl> task_queue) {
  AutoLock lock(lock_);
  if (task_queue_manager_deleted_)
    return;
  queues_.push_back(std::move(task_queue));
}

void GracefulQueueShutdownHelper::OnTaskQueueManagerDeleted() {
  AutoLock lock(lock_);
  task_queue_manager_deleted_ = true;
  queues_.clear();
}

std::vector<std::unique_ptr<internal::TaskQueueImpl>>
GracefulQueueShutdownHelper::TakeQueues() {
  AutoLock lock(lock_);
  std::vector<std::unique_ptr<internal::TaskQueueImpl>> result;
  result.swap(queues_);
  return result;
}

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base
