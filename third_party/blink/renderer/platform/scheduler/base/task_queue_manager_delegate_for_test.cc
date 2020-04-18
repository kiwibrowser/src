// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager_delegate_for_test.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace base {
namespace sequence_manager {

// static
scoped_refptr<TaskQueueManagerDelegateForTest>
TaskQueueManagerDelegateForTest::Create(
    scoped_refptr<SingleThreadTaskRunner> task_runner,
    const TickClock* time_source) {
  return WrapRefCounted(
      new TaskQueueManagerDelegateForTest(task_runner, time_source));
}

TaskQueueManagerDelegateForTest::TaskQueueManagerDelegateForTest(
    scoped_refptr<SingleThreadTaskRunner> task_runner,
    const TickClock* time_source)
    : task_runner_(task_runner), time_source_(time_source) {}

TaskQueueManagerDelegateForTest::~TaskQueueManagerDelegateForTest() {}

bool TaskQueueManagerDelegateForTest::PostDelayedTask(const Location& from_here,
                                                      OnceClosure task,
                                                      TimeDelta delay) {
  return task_runner_->PostDelayedTask(from_here, std::move(task), delay);
}

bool TaskQueueManagerDelegateForTest::PostNonNestableDelayedTask(
    const Location& from_here,
    OnceClosure task,
    TimeDelta delay) {
  return task_runner_->PostNonNestableDelayedTask(from_here, std::move(task),
                                                  delay);
}

bool TaskQueueManagerDelegateForTest::RunsTasksInCurrentSequence() const {
  return task_runner_->RunsTasksInCurrentSequence();
}

bool TaskQueueManagerDelegateForTest::IsNested() const {
  return false;
}

void TaskQueueManagerDelegateForTest::AddNestingObserver(
    RunLoop::NestingObserver* observer) {}

void TaskQueueManagerDelegateForTest::RemoveNestingObserver(
    RunLoop::NestingObserver* observer) {}

TimeTicks TaskQueueManagerDelegateForTest::NowTicks() const {
  return time_source_->NowTicks();
}

}  // namespace sequence_manager
}  // namespace base
