// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/test/lazy_scheduler_message_loop_delegate_for_tests.h"

#include <memory>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/time/default_tick_clock.h"

namespace blink {
namespace scheduler {

// static
scoped_refptr<LazySchedulerMessageLoopDelegateForTests>
LazySchedulerMessageLoopDelegateForTests::Create() {
  return base::WrapRefCounted(new LazySchedulerMessageLoopDelegateForTests());
}

LazySchedulerMessageLoopDelegateForTests::
    LazySchedulerMessageLoopDelegateForTests()
    : message_loop_(base::MessageLoop::current()),
      thread_id_(base::PlatformThread::CurrentId()),
      time_source_(base::DefaultTickClock::GetInstance()),
      pending_observer_(nullptr) {
  if (message_loop_)
    original_task_runner_ = message_loop_->task_runner();
}

LazySchedulerMessageLoopDelegateForTests::
    ~LazySchedulerMessageLoopDelegateForTests() {
  RestoreDefaultTaskRunner();
}

base::MessageLoop* LazySchedulerMessageLoopDelegateForTests::EnsureMessageLoop()
    const {
  if (message_loop_)
    return message_loop_;
  DCHECK(RunsTasksInCurrentSequence());
  message_loop_ = base::MessageLoop::current();
  DCHECK(message_loop_);
  original_task_runner_ = message_loop_->task_runner();
  if (pending_task_runner_)
    message_loop_->SetTaskRunner(std::move(pending_task_runner_));
  if (pending_observer_)
    base::RunLoop::AddNestingObserverOnCurrentThread(pending_observer_);
  return message_loop_;
}

void LazySchedulerMessageLoopDelegateForTests::SetDefaultTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  if (!HasMessageLoop()) {
    pending_task_runner_ = std::move(task_runner);
    return;
  }
  message_loop_->SetTaskRunner(std::move(task_runner));
}

void LazySchedulerMessageLoopDelegateForTests::RestoreDefaultTaskRunner() {
  if (HasMessageLoop() && base::MessageLoop::current() == message_loop_)
    message_loop_->SetTaskRunner(original_task_runner_);
}

bool LazySchedulerMessageLoopDelegateForTests::HasMessageLoop() const {
  return !!message_loop_;
}

bool LazySchedulerMessageLoopDelegateForTests::PostDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  EnsureMessageLoop();
  return original_task_runner_->PostDelayedTask(from_here, std::move(task),
                                                delay);
}

bool LazySchedulerMessageLoopDelegateForTests::PostNonNestableDelayedTask(
    const base::Location& from_here,
    base::OnceClosure task,
    base::TimeDelta delay) {
  EnsureMessageLoop();
  return original_task_runner_->PostNonNestableDelayedTask(
      from_here, std::move(task), delay);
}

bool LazySchedulerMessageLoopDelegateForTests::RunsTasksInCurrentSequence()
    const {
  return thread_id_ == base::PlatformThread::CurrentId();
}

bool LazySchedulerMessageLoopDelegateForTests::IsNested() const {
  DCHECK(RunsTasksInCurrentSequence());
  EnsureMessageLoop();
  return base::RunLoop::IsNestedOnCurrentThread();
}

void LazySchedulerMessageLoopDelegateForTests::AddNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  // While |observer| _could_ be associated with the current thread regardless
  // of the presence of a MessageLoop, the association is delayed until
  // EnsureMessageLoop() is invoked. This works around a state issue where
  // otherwise many tests fail because of the following sequence:
  //   1) blink::scheduler::CreateRendererSchedulerForTests()
  //       -> TaskQueueManager::TaskQueueManager()
  //       -> LazySchedulerMessageLoopDelegateForTests::AddNestingObserver()
  //   2) Any test framework with a base::MessageLoop member (and not caring
  //      about the blink scheduler) does:
  //        ThreadTaskRunnerHandle::Get()->PostTask(
  //            FROM_HERE, an_init_task_with_a_nested_loop);
  //        RunLoop.RunUntilIdle();
  //   3) |a_task_with_a_nested_loop| triggers
  //          TaskQueueManager::OnBeginNestedLoop() which:
  //            a) flags any_thread().is_nested = true;
  //            b) posts a task to self, which triggers:
  //                 LazySchedulerMessageLoopDelegateForTests::PostDelayedTask()
  //   4) This self-task in turn triggers TaskQueueManager::DoWork()
  //      which expects to be the only one to trigger nested loops (doesn't
  //      support TaskQueueManager::OnBeginNestedLoop() being invoked before
  //      it kicks in), resulting in it hitting:
  //      DCHECK_EQ(any_thread().is_nested, delegate_->IsNested()); (1 vs 0).
  // TODO(skyostil): fix this convulotion as part of http://crbug.com/495659.
  if (!HasMessageLoop()) {
    pending_observer_ = observer;
    return;
  }
  base::RunLoop::AddNestingObserverOnCurrentThread(observer);
}

void LazySchedulerMessageLoopDelegateForTests::RemoveNestingObserver(
    base::RunLoop::NestingObserver* observer) {
  if (!message_loop_ || message_loop_ != base::MessageLoop::current()) {
    DCHECK_EQ(pending_observer_, observer);
    pending_observer_ = nullptr;
    return;
  }
  base::RunLoop::RemoveNestingObserverOnCurrentThread(observer);
}

base::TimeTicks LazySchedulerMessageLoopDelegateForTests::NowTicks() {
  return time_source_->NowTicks();
}

}  // namespace scheduler
}  // namespace blink
