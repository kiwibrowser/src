// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_THREAD_CONTROLLER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_THREAD_CONTROLLER_IMPL_H_

#include "base/cancelable_callback.h"
#include "base/debug/task_annotator.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/run_loop.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/task/sequence_manager/thread_controller.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace base {

class MessageLoop;

namespace sequence_manager {
namespace internal {

class PLATFORM_EXPORT ThreadControllerImpl : public ThreadController,
                                             public RunLoop::NestingObserver {
 public:
  ~ThreadControllerImpl() override;

  static std::unique_ptr<ThreadControllerImpl> Create(
      MessageLoop* message_loop,
      const TickClock* time_source);

  // ThreadController:
  void SetWorkBatchSize(int work_batch_size) override;
  void DidQueueTask(const PendingTask& pending_task) override;
  void ScheduleWork() override;
  void SetNextDelayedDoWork(LazyNow* lazy_now, TimeTicks run_time) override;
  void SetSequencedTaskSource(SequencedTaskSource* sequence) override;
  bool RunsTasksInCurrentSequence() override;
  const TickClock* GetClock() override;
  void SetDefaultTaskRunner(scoped_refptr<SingleThreadTaskRunner>) override;
  void RestoreDefaultTaskRunner() override;
  void AddNestingObserver(RunLoop::NestingObserver* observer) override;
  void RemoveNestingObserver(RunLoop::NestingObserver* observer) override;

  // RunLoop::NestingObserver:
  void OnBeginNestedRunLoop() override;
  void OnExitNestedRunLoop() override;

 protected:
  ThreadControllerImpl(MessageLoop* message_loop,
                       scoped_refptr<SingleThreadTaskRunner> task_runner,
                       const TickClock* time_source);

  // TODO(altimin): Make these const. Blocked on removing
  // lazy initialisation support.
  MessageLoop* message_loop_;
  scoped_refptr<SingleThreadTaskRunner> task_runner_;

  RunLoop::NestingObserver* nesting_observer_ = nullptr;

 private:
  enum class WorkType { kImmediate, kDelayed };

  void DoWork(WorkType work_type);

  struct AnySequence {
    AnySequence();
    ~AnySequence();

    int do_work_running_count = 0;
    int nesting_depth = 0;
    bool immediate_do_work_posted = false;
  };

  mutable Lock any_sequence_lock_;
  AnySequence any_sequence_;

  struct AnySequence& any_sequence() {
    any_sequence_lock_.AssertAcquired();
    return any_sequence_;
  }
  const struct AnySequence& any_sequence() const {
    any_sequence_lock_.AssertAcquired();
    return any_sequence_;
  }

  struct MainSequenceOnly {
    MainSequenceOnly();
    ~MainSequenceOnly();

    int do_work_running_count = 0;
    int nesting_depth = 0;
    int work_batch_size_ = 1;

    TimeTicks next_delayed_do_work = TimeTicks::Max();
  };

  SEQUENCE_CHECKER(sequence_checker_);
  MainSequenceOnly main_sequence_only_;
  MainSequenceOnly& main_sequence_only() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return main_sequence_only_;
  }
  const MainSequenceOnly& main_sequence_only() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return main_sequence_only_;
  }

  scoped_refptr<SingleThreadTaskRunner> message_loop_task_runner_;
  const TickClock* time_source_;
  RepeatingClosure immediate_do_work_closure_;
  RepeatingClosure delayed_do_work_closure_;
  CancelableClosure cancelable_delayed_do_work_closure_;
  SequencedTaskSource* sequence_ = nullptr;  // NOT OWNED
  debug::TaskAnnotator task_annotator_;

  WeakPtrFactory<ThreadControllerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ThreadControllerImpl);
};

}  // namespace internal
}  // namespace sequence_manager
}  // namespace base

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_BASE_THREAD_CONTROLLER_IMPL_H_
