// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/thread_controller_impl.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/task/sequence_manager/lazy_now.h"
#include "base/task/sequence_manager/sequenced_task_source.h"
#include "base/trace_event/trace_event.h"

namespace base {
namespace sequence_manager {
namespace internal {

ThreadControllerImpl::ThreadControllerImpl(
    MessageLoop* message_loop,
    scoped_refptr<SingleThreadTaskRunner> task_runner,
    const TickClock* time_source)
    : message_loop_(message_loop),
      task_runner_(task_runner),
      message_loop_task_runner_(message_loop ? message_loop->task_runner()
                                             : nullptr),
      time_source_(time_source),
      weak_factory_(this) {
  immediate_do_work_closure_ =
      BindRepeating(&ThreadControllerImpl::DoWork, weak_factory_.GetWeakPtr(),
                    WorkType::kImmediate);
  delayed_do_work_closure_ =
      BindRepeating(&ThreadControllerImpl::DoWork, weak_factory_.GetWeakPtr(),
                    WorkType::kDelayed);
}

ThreadControllerImpl::~ThreadControllerImpl() = default;

ThreadControllerImpl::AnySequence::AnySequence() = default;

ThreadControllerImpl::AnySequence::~AnySequence() = default;

ThreadControllerImpl::MainSequenceOnly::MainSequenceOnly() = default;

ThreadControllerImpl::MainSequenceOnly::~MainSequenceOnly() = default;

std::unique_ptr<ThreadControllerImpl> ThreadControllerImpl::Create(
    MessageLoop* message_loop,
    const TickClock* time_source) {
  return WrapUnique(new ThreadControllerImpl(
      message_loop, message_loop->task_runner(), time_source));
}

void ThreadControllerImpl::SetSequencedTaskSource(
    SequencedTaskSource* sequence) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence);
  DCHECK(!sequence_);
  sequence_ = sequence;
}

void ThreadControllerImpl::ScheduleWork() {
  DCHECK(sequence_);
  AutoLock lock(any_sequence_lock_);
  // Don't post a DoWork if there's an immediate DoWork in flight or if we're
  // inside a top level DoWork. We can rely on a continuation being posted as
  // needed.
  if (any_sequence().immediate_do_work_posted ||
      (any_sequence().do_work_running_count > any_sequence().nesting_depth)) {
    return;
  }
  any_sequence().immediate_do_work_posted = true;

  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
               "ThreadControllerImpl::ScheduleWork::PostTask");
  task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
}

void ThreadControllerImpl::SetNextDelayedDoWork(LazyNow* lazy_now,
                                                TimeTicks run_time) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence_);

  if (main_sequence_only().next_delayed_do_work == run_time)
    return;

  // Cancel DoWork if it was scheduled and we set an "infinite" delay now.
  if (run_time == TimeTicks::Max()) {
    cancelable_delayed_do_work_closure_.Cancel();
    main_sequence_only().next_delayed_do_work = TimeTicks::Max();
    return;
  }

  // If DoWork is running then we don't need to do anything because it will post
  // a continuation as needed. Bailing out here is by far the most common case.
  if (main_sequence_only().do_work_running_count >
      main_sequence_only().nesting_depth) {
    return;
  }

  // If DoWork is about to run then we also don't need to do anything.
  {
    AutoLock lock(any_sequence_lock_);
    if (any_sequence().immediate_do_work_posted)
      return;
  }

  base::TimeDelta delay = std::max(TimeDelta(), run_time - lazy_now->Now());
  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
               "ThreadControllerImpl::SetNextDelayedDoWork::PostDelayedTask",
               "delay_ms", delay.InMillisecondsF());

  main_sequence_only().next_delayed_do_work = run_time;
  // Reset also causes cancellation of the previous DoWork task.
  cancelable_delayed_do_work_closure_.Reset(delayed_do_work_closure_);
  task_runner_->PostDelayedTask(
      FROM_HERE, cancelable_delayed_do_work_closure_.callback(), delay);
}

bool ThreadControllerImpl::RunsTasksInCurrentSequence() {
  return task_runner_->RunsTasksInCurrentSequence();
}

const TickClock* ThreadControllerImpl::GetClock() {
  return time_source_;
}

void ThreadControllerImpl::SetDefaultTaskRunner(
    scoped_refptr<SingleThreadTaskRunner> task_runner) {
  if (!message_loop_)
    return;
  message_loop_->SetTaskRunner(task_runner);
}

void ThreadControllerImpl::RestoreDefaultTaskRunner() {
  if (!message_loop_)
    return;
  message_loop_->SetTaskRunner(message_loop_task_runner_);
}

void ThreadControllerImpl::DidQueueTask(const PendingTask& pending_task) {
  task_annotator_.DidQueueTask("TaskQueueManager::PostTask", pending_task);
}

void ThreadControllerImpl::DoWork(WorkType work_type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(sequence_);

  {
    AutoLock lock(any_sequence_lock_);
    if (work_type == WorkType::kImmediate)
      any_sequence().immediate_do_work_posted = false;
    any_sequence().do_work_running_count++;
  }

  main_sequence_only().do_work_running_count++;

  WeakPtr<ThreadControllerImpl> weak_ptr = weak_factory_.GetWeakPtr();
  // TODO(scheduler-dev): Consider moving to a time based work batch instead.
  for (int i = 0; i < main_sequence_only().work_batch_size_; i++) {
    Optional<PendingTask> task = sequence_->TakeTask();
    if (!task)
      break;

    TRACE_TASK_EXECUTION("ThreadControllerImpl::DoWork", *task);
    task_annotator_.RunTask("ThreadControllerImpl::DoWork", &*task);

    if (!weak_ptr)
      return;

    sequence_->DidRunTask();

    // TODO(alexclarke): Find out why this is needed.
    if (main_sequence_only().nesting_depth > 0)
      break;
  }

  main_sequence_only().do_work_running_count--;

  {
    AutoLock lock(any_sequence_lock_);
    any_sequence().do_work_running_count--;
    DCHECK_GE(any_sequence().do_work_running_count, 0);
    LazyNow lazy_now(time_source_);
    TimeDelta delay_till_next_task = sequence_->DelayTillNextTask(&lazy_now);
    if (delay_till_next_task <= TimeDelta()) {
      // The next task needs to run immediately, post a continuation if needed.
      if (!any_sequence().immediate_do_work_posted) {
        any_sequence().immediate_do_work_posted = true;
        task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
      }
    } else if (delay_till_next_task < TimeDelta::Max()) {
      // The next task needs to run after a delay, post a continuation if
      // needed.
      TimeTicks next_task_at = lazy_now.Now() + delay_till_next_task;
      if (next_task_at != main_sequence_only().next_delayed_do_work) {
        main_sequence_only().next_delayed_do_work = next_task_at;
        cancelable_delayed_do_work_closure_.Reset(delayed_do_work_closure_);
        task_runner_->PostDelayedTask(
            FROM_HERE, cancelable_delayed_do_work_closure_.callback(),
            delay_till_next_task);
      }
    } else {
      // There is no next task scheduled.
      main_sequence_only().next_delayed_do_work = TimeTicks::Max();
    }
  }
}

void ThreadControllerImpl::AddNestingObserver(
    RunLoop::NestingObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  nesting_observer_ = observer;
  RunLoop::AddNestingObserverOnCurrentThread(this);
}

void ThreadControllerImpl::RemoveNestingObserver(
    RunLoop::NestingObserver* observer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(observer, nesting_observer_);
  nesting_observer_ = nullptr;
  RunLoop::RemoveNestingObserverOnCurrentThread(this);
}

void ThreadControllerImpl::OnBeginNestedRunLoop() {
  main_sequence_only().nesting_depth++;
  {
    AutoLock lock(any_sequence_lock_);
    any_sequence().nesting_depth++;
    if (!any_sequence().immediate_do_work_posted) {
      any_sequence().immediate_do_work_posted = true;
      TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("sequence_manager"),
                   "ThreadControllerImpl::OnBeginNestedRunLoop::PostTask");
      task_runner_->PostTask(FROM_HERE, immediate_do_work_closure_);
    }
  }
  if (nesting_observer_)
    nesting_observer_->OnBeginNestedRunLoop();
}

void ThreadControllerImpl::OnExitNestedRunLoop() {
  main_sequence_only().nesting_depth--;
  {
    AutoLock lock(any_sequence_lock_);
    any_sequence().nesting_depth--;
    DCHECK_GE(any_sequence().nesting_depth, 0);
  }
  if (nesting_observer_)
    nesting_observer_->OnExitNestedRunLoop();
}

void ThreadControllerImpl::SetWorkBatchSize(int work_batch_size) {
  main_sequence_only().work_batch_size_ = work_batch_size;
}

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base
