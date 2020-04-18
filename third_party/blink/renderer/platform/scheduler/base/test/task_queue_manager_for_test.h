// Copyright 2018 The Chromium Authors. All rights reserved.  // Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TASK_QUEUE_MANAGER_FOR_TEST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TASK_QUEUE_MANAGER_FOR_TEST_H_

#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager_impl.h"

namespace base {
class MessageLoop;
}

namespace base {
namespace sequence_manager {

class TaskQueueManagerForTest : public TaskQueueManagerImpl {
 public:
  explicit TaskQueueManagerForTest(
      std::unique_ptr<internal::ThreadController> thread_controller);

  ~TaskQueueManagerForTest() override = default;

  // Creates TaskQueueManagerImpl using ThreadControllerImpl constructed with
  // the given arguments. ThreadControllerImpl is slightly overridden to skip
  // nesting observers registration if message loop is absent.
  static std::unique_ptr<TaskQueueManagerForTest> Create(
      MessageLoop* message_loop,
      scoped_refptr<SingleThreadTaskRunner> task_runner,
      const TickClock* clock);

  size_t ActiveQueuesCount() const;
  bool HasImmediateWork() const;
  size_t PendingTasksCount() const;
  size_t QueuesToDeleteCount() const;
  size_t QueuesToShutdownCount();

  using TaskQueueManagerImpl::GetNextSequenceNumber;
  using TaskQueueManagerImpl::WakeUpReadyDelayedQueues;
};

}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_TEST_TASK_QUEUE_MANAGER_FOR_TEST_H_
