// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace base {
namespace sequence_manager {
namespace internal {

class TaskQueueImpl;

// Thread-safe helper to shutdown queues from any thread.
class GracefulQueueShutdownHelper
    : public RefCountedThreadSafe<GracefulQueueShutdownHelper> {
 public:
  GracefulQueueShutdownHelper();
  ~GracefulQueueShutdownHelper();

  void GracefullyShutdownTaskQueue(
      std::unique_ptr<internal::TaskQueueImpl> queue);

  void OnTaskQueueManagerDeleted();

  std::vector<std::unique_ptr<internal::TaskQueueImpl>> TakeQueues();

 private:
  Lock lock_;
  bool task_queue_manager_deleted_;
  std::vector<std::unique_ptr<internal::TaskQueueImpl>> queues_;

  DISALLOW_COPY_AND_ASSIGN(GracefulQueueShutdownHelper);
};

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_
