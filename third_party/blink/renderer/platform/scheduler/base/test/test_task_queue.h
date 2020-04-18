// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_QUEUE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_QUEUE_H_

#include "base/memory/weak_ptr.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"

namespace base {
namespace sequence_manager {

class TestTaskQueue : public TaskQueue {
 public:
  explicit TestTaskQueue(std::unique_ptr<internal::TaskQueueImpl> impl,
                         const TaskQueue::Spec& spec);
  ~TestTaskQueue() override;

  using TaskQueue::GetTaskQueueImpl;

  WeakPtr<TestTaskQueue> GetWeakPtr();

 private:
  // Used to ensure that task queue is deleted in tests.
  WeakPtrFactory<TestTaskQueue> weak_factory_;
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TEST_TASK_QUEUE_H_
