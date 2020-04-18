// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_

#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"

namespace blink {
namespace scheduler {

class NonMainThreadScheduler;

class PLATFORM_EXPORT WorkerTaskQueue
    : public base::sequence_manager::TaskQueue {
 public:
  WorkerTaskQueue(
      std::unique_ptr<base::sequence_manager::internal::TaskQueueImpl> impl,
      const Spec& spec,
      NonMainThreadScheduler* non_main_thread_scheduler);
  ~WorkerTaskQueue() override;

  void OnTaskCompleted(const base::sequence_manager::TaskQueue::Task& task,
                       base::TimeTicks start,
                       base::TimeTicks end,
                       base::Optional<base::TimeDelta> thread_time);

 private:
  // Not owned.
  NonMainThreadScheduler* non_main_thread_scheduler_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_CHILD_WORKER_TASK_QUEUE_H_
