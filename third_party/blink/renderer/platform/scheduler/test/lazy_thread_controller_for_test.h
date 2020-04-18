// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_TEST_LAZY_THREAD_CONTROLLER_FOR_TEST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_TEST_LAZY_THREAD_CONTROLLER_FOR_TEST_H_

#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/platform_thread.h"
#include "third_party/blink/renderer/platform/scheduler/base/thread_controller_impl.h"

namespace blink {
namespace scheduler {

// This class connects the scheduler to a MessageLoop, but unlike
// ThreadControllerImpl it allows the message loop to be created lazily
// after the scheduler has been brought up. This is needed in testing scenarios
// where Blink is initialized before a MessageLoop has been created.
//
// TODO(skyostil): Fix the relevant test suites and remove this class
// (crbug.com/495659).
class LazyThreadControllerForTest
    : public base::sequence_manager::internal::ThreadControllerImpl {
 public:
  LazyThreadControllerForTest();
  ~LazyThreadControllerForTest() override;

  // internal::ThreadControllerImpl:
  void AddNestingObserver(base::RunLoop::NestingObserver* observer) override;
  void RemoveNestingObserver(base::RunLoop::NestingObserver* observer) override;
  bool RunsTasksInCurrentSequence() override;
  void ScheduleWork() override;
  void SetNextDelayedDoWork(base::sequence_manager::LazyNow* lazy_now,
                            base::TimeTicks run_time) override;
  void SetDefaultTaskRunner(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner) override;
  void RestoreDefaultTaskRunner() override;

 private:
  bool HasMessageLoop();
  void EnsureMessageLoop();

  base::PlatformThreadRef thread_ref_;

  bool pending_observer_ = false;
  scoped_refptr<base::SingleThreadTaskRunner> pending_default_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(LazyThreadControllerForTest);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_TEST_LAZY_THREAD_CONTROLLER_FOR_TEST_H_
