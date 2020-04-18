// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"

#include "base/bind.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "third_party/blink/public/platform/scheduler/child/webthread_base.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/wtf/thread_specific.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

namespace {

struct ThreadLocalStorage {
  WebThread* current_thread = nullptr;
};

ThreadLocalStorage* GetThreadLocalStorage() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<ThreadLocalStorage>, tls, ());
  return tls;
}

void PrepareCurrentThread(WaitableEvent* event, WebThread* thread) {
  GetThreadLocalStorage()->current_thread = thread;
  event->Signal();
}

}  // namespace

TestingPlatformSupportWithMockScheduler::
    TestingPlatformSupportWithMockScheduler()
    : mock_task_runner_(
          base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, true)) {
  DCHECK(IsMainThread());
  std::unique_ptr<base::sequence_manager::TaskQueueManagerForTest>
      task_queue_manager =
          base::sequence_manager::TaskQueueManagerForTest::Create(
              nullptr, mock_task_runner_, &clock_);
  task_queue_manager_ = task_queue_manager.get();
  scheduler_ = std::make_unique<scheduler::MainThreadSchedulerImpl>(
      std::move(task_queue_manager), base::nullopt);
  thread_ = scheduler_->CreateMainThread();
  // Set the work batch size to one so RunPendingTasks behaves as expected.
  scheduler_->GetSchedulerHelperForTesting()->SetWorkBatchSizeForTesting(1);

  WTF::SetTimeFunctionsForTesting(GetTestTime);
}

TestingPlatformSupportWithMockScheduler::
    ~TestingPlatformSupportWithMockScheduler() {
  WTF::SetTimeFunctionsForTesting(nullptr);
  scheduler_->Shutdown();
}

std::unique_ptr<WebThread>
TestingPlatformSupportWithMockScheduler::CreateThread(
    const WebThreadCreationParams& params) {
  std::unique_ptr<scheduler::WebThreadBase> thread =
      scheduler::WebThreadBase::CreateWorkerThread(params);
  thread->Init();
  WaitableEvent event;
  thread->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(PrepareCurrentThread, base::Unretained(&event),
                                base::Unretained(thread.get())));
  event.Wait();
  return std::move(thread);
}

WebThread* TestingPlatformSupportWithMockScheduler::CurrentThread() {
  DCHECK_EQ(thread_->IsCurrentThread(), IsMainThread());

  if (thread_->IsCurrentThread()) {
    return thread_.get();
  }
  ThreadLocalStorage* storage = GetThreadLocalStorage();
  DCHECK(storage->current_thread);
  return storage->current_thread;
}

void TestingPlatformSupportWithMockScheduler::RunSingleTask() {
  mock_task_runner_->SetRunTaskLimit(1);
  mock_task_runner_->RunPendingTasks();
  mock_task_runner_->ClearRunTaskLimit();
}

void TestingPlatformSupportWithMockScheduler::RunUntilIdle() {
  mock_task_runner_->RunUntilIdle();
}

void TestingPlatformSupportWithMockScheduler::RunForPeriodSeconds(
    double seconds) {
  const base::TimeTicks deadline =
      clock_.NowTicks() + base::TimeDelta::FromSecondsD(seconds);

  for (;;) {
    // If we've run out of immediate work then fast forward to the next delayed
    // task, but don't pass |deadline|.
    if (!task_queue_manager_->HasImmediateWork()) {
      base::TimeTicks next_delayed_task;
      if (!task_queue_manager_->GetRealTimeDomain()->NextScheduledRunTime(
              &next_delayed_task) ||
          next_delayed_task > deadline) {
        break;
      }

      clock_.SetNowTicks(next_delayed_task);
    }

    if (clock_.NowTicks() > deadline)
      break;

    mock_task_runner_->RunPendingTasks();
  }

  clock_.SetNowTicks(deadline);
}

void TestingPlatformSupportWithMockScheduler::AdvanceClockSeconds(
    double seconds) {
  clock_.Advance(base::TimeDelta::FromSecondsD(seconds));
}

void TestingPlatformSupportWithMockScheduler::SetAutoAdvanceNowToPendingTasks(
    bool auto_advance) {
  mock_task_runner_->SetAutoAdvanceNowToPendingTasks(auto_advance);
}

scheduler::MainThreadSchedulerImpl*
TestingPlatformSupportWithMockScheduler::GetMainThreadScheduler() const {
  return scheduler_.get();
}

// static
double TestingPlatformSupportWithMockScheduler::GetTestTime() {
  TestingPlatformSupportWithMockScheduler* platform =
      static_cast<TestingPlatformSupportWithMockScheduler*>(
          Platform::Current());
  return (platform->clock_.NowTicks() - base::TimeTicks()).InSecondsF();
}

}  // namespace blink
