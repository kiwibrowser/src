// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"

#include "third_party/blink/renderer/platform/scheduler/base/thread_controller_impl.h"

namespace base {
namespace sequence_manager {

namespace {

class ThreadControllerForTest : public internal::ThreadControllerImpl {
 public:
  ThreadControllerForTest(MessageLoop* message_loop,
                          scoped_refptr<SingleThreadTaskRunner> task_runner,
                          const TickClock* time_source)
      : ThreadControllerImpl(message_loop,
                             std::move(task_runner),
                             time_source) {}

  void AddNestingObserver(RunLoop::NestingObserver* observer) override {
    if (!message_loop_)
      return;
    ThreadControllerImpl::AddNestingObserver(observer);
  }

  void RemoveNestingObserver(RunLoop::NestingObserver* observer) override {
    if (!message_loop_)
      return;
    ThreadControllerImpl::RemoveNestingObserver(observer);
  }

  ~ThreadControllerForTest() override = default;
};

}  // namespace

TaskQueueManagerForTest::TaskQueueManagerForTest(
    std::unique_ptr<internal::ThreadController> thread_controller)
    : TaskQueueManagerImpl(std::move(thread_controller)) {}

// static
std::unique_ptr<TaskQueueManagerForTest> TaskQueueManagerForTest::Create(
    MessageLoop* message_loop,
    scoped_refptr<SingleThreadTaskRunner> task_runner,
    const TickClock* clock) {
  return std::make_unique<TaskQueueManagerForTest>(
      std::make_unique<ThreadControllerForTest>(message_loop,
                                                std::move(task_runner), clock));
}

size_t TaskQueueManagerForTest::ActiveQueuesCount() const {
  return main_thread_only().active_queues.size();
}

bool TaskQueueManagerForTest::HasImmediateWork() const {
  return !main_thread_only().selector.AllEnabledWorkQueuesAreEmpty();
}

size_t TaskQueueManagerForTest::PendingTasksCount() const {
  size_t task_count = 0;
  for (auto* const queue : main_thread_only().active_queues)
    task_count += queue->GetNumberOfPendingTasks();
  return task_count;
}

size_t TaskQueueManagerForTest::QueuesToDeleteCount() const {
  return main_thread_only().queues_to_delete.size();
}

size_t TaskQueueManagerForTest::QueuesToShutdownCount() {
  TakeQueuesToGracefullyShutdownFromHelper();
  return main_thread_only().queues_to_gracefully_shutdown.size();
}

}  // namespace sequence_manager
}  // namespace base
