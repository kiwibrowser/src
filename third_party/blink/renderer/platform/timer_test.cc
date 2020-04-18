// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/timer.h"

#include <memory>
#include <queue>
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_thread.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

using base::sequence_manager::TaskQueue;
using testing::ElementsAre;

namespace blink {
namespace {

class TimerTest : public testing::Test {
 public:
  void SetUp() override {
    run_times_.clear();
    platform_->AdvanceClockSeconds(10.0);
    start_time_ = CurrentTimeTicksInSeconds();
  }

  void CountingTask(TimerBase*) {
    run_times_.push_back(CurrentTimeTicksInSeconds());
  }

  void RecordNextFireTimeTask(TimerBase* timer) {
    next_fire_times_.push_back(CurrentTimeTicksInSeconds() +
                               timer->NextFireInterval());
  }

  void RunUntilDeadline(double deadline) {
    double period = deadline - CurrentTimeTicksInSeconds();
    EXPECT_GE(period, 0.0);
    platform_->RunForPeriodSeconds(period);
  }

  // Returns false if there are no pending delayed tasks, otherwise sets |time|
  // to the delay in seconds till the next pending delayed task is scheduled to
  // fire.
  bool TimeTillNextDelayedTask(double* time) const {
    base::TimeTicks next_run_time;
    if (!platform_->GetMainThreadScheduler()
             ->GetActiveTimeDomain()
             ->NextScheduledRunTime(&next_run_time))
      return false;
    *time = (next_run_time -
             platform_->GetMainThreadScheduler()->GetActiveTimeDomain()->Now())
                .InSecondsF();
    return true;
  }

 protected:
  double start_time_;
  WTF::Vector<double> run_times_;
  WTF::Vector<double> next_fire_times_;
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

  base::MessageLoop message_loop_;
};

class OnHeapTimerOwner final
    : public GarbageCollectedFinalized<OnHeapTimerOwner> {
 public:
  class Record final : public RefCounted<Record> {
   public:
    static scoped_refptr<Record> Create() { return base::AdoptRef(new Record); }

    bool TimerHasFired() const { return timer_has_fired_; }
    bool IsDisposed() const { return is_disposed_; }
    bool OwnerIsDestructed() const { return owner_is_destructed_; }
    void SetTimerHasFired() { timer_has_fired_ = true; }
    void Dispose() { is_disposed_ = true; }
    void SetOwnerIsDestructed() { owner_is_destructed_ = true; }

   private:
    Record() = default;

    bool timer_has_fired_ = false;
    bool is_disposed_ = false;
    bool owner_is_destructed_ = false;
  };

  explicit OnHeapTimerOwner(
      scoped_refptr<Record> record,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : timer_(std::move(task_runner), this, &OnHeapTimerOwner::Fired),
        record_(std::move(record)) {}
  ~OnHeapTimerOwner() { record_->SetOwnerIsDestructed(); }

  void StartOneShot(TimeDelta interval, const base::Location& caller) {
    timer_.StartOneShot(interval, caller);
  }

  void Trace(blink::Visitor* visitor) {}

 private:
  void Fired(TimerBase*) {
    EXPECT_FALSE(record_->IsDisposed());
    record_->SetTimerHasFired();
  }

  TaskRunnerTimer<OnHeapTimerOwner> timer_;
  scoped_refptr<Record> record_;
};

class GCForbiddenScope final {
 public:
  STACK_ALLOCATED();
  GCForbiddenScope() { ThreadState::Current()->EnterGCForbiddenScope(); }
  ~GCForbiddenScope() { ThreadState::Current()->LeaveGCForbiddenScope(); }
};

TEST_F(TimerTest, StartOneShot_Zero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  double run_time;
  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_));
}

TEST_F(TimerTest, StartOneShot_ZeroAndCancel) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  double run_time;
  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  timer.Stop();

  platform_->RunUntilIdle();
  EXPECT_FALSE(run_times_.size());
}

TEST_F(TimerTest, StartOneShot_ZeroAndCancelThenRepost) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  double run_time;
  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  timer.Stop();

  platform_->RunUntilIdle();
  EXPECT_FALSE(run_times_.size());

  timer.StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_));
}

TEST_F(TimerTest, StartOneShot_Zero_RepostingAfterRunning) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  double run_time;
  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_));

  timer.StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_FALSE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_, start_time_));
}

TEST_F(TimerTest, StartOneShot_NonZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 10.0));
}

TEST_F(TimerTest, StartOneShot_NonZeroAndCancel) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  timer.Stop();
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_FALSE(run_times_.size());
}

TEST_F(TimerTest, StartOneShot_NonZeroAndCancelThenRepost) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  timer.Stop();
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));

  platform_->RunUntilIdle();
  EXPECT_FALSE(run_times_.size());

  double second_post_time = CurrentTimeTicksInSeconds();
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(second_post_time + 10.0));
}

TEST_F(TimerTest, StartOneShot_NonZero_RepostingAfterRunning) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 10.0));

  timer.StartOneShot(TimeDelta::FromSeconds(20), FROM_HERE);

  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(20.0, run_time);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 10.0, start_time_ + 30.0));
}

TEST_F(TimerTest, PostingTimerTwiceWithSameRunTimeDoesNothing) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(10.0, run_time);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 10.0));
}

TEST_F(TimerTest, PostingTimerTwiceWithNewerRunTimeCancelsOriginalTask) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 0.0));
}

TEST_F(TimerTest, PostingTimerTwiceWithLaterRunTimeCancelsOriginalTask) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  platform_->RunUntilIdle();
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 10.0));
}

TEST_F(TimerTest, StartRepeatingTask) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(1.0, run_time);

  RunUntilDeadline(start_time_ + 5.5);
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 1.0, start_time_ + 2.0,
                                      start_time_ + 3.0, start_time_ + 4.0,
                                      start_time_ + 5.0));
}

TEST_F(TimerTest, StartRepeatingTask_ThenCancel) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(1.0, run_time);

  RunUntilDeadline(start_time_ + 2.5);
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 1.0, start_time_ + 2.0));

  timer.Stop();
  platform_->RunUntilIdle();

  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 1.0, start_time_ + 2.0));
}

TEST_F(TimerTest, StartRepeatingTask_ThenPostOneShot) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  double run_time;
  EXPECT_TRUE(TimeTillNextDelayedTask(&run_time));
  EXPECT_FLOAT_EQ(1.0, run_time);

  RunUntilDeadline(start_time_ + 2.5);
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 1.0, start_time_ + 2.0));

  timer.StartOneShot(TimeDelta(), FROM_HERE);
  platform_->RunUntilIdle();

  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 1.0, start_time_ + 2.0,
                                      start_time_ + 2.5));
}

TEST_F(TimerTest, IsActive_NeverPosted) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);

  EXPECT_FALSE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterPosting_OneShotZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_TRUE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterPosting_OneShotNonZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  EXPECT_TRUE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterPosting_Repeating) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  EXPECT_TRUE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterRunning_OneShotZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  platform_->RunUntilIdle();
  EXPECT_FALSE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterRunning_OneShotNonZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  platform_->RunUntilIdle();
  EXPECT_FALSE(timer.IsActive());
}

TEST_F(TimerTest, IsActive_AfterRunning_Repeating) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  RunUntilDeadline(start_time_ + 10);
  EXPECT_TRUE(timer.IsActive());  // It should run until cancelled.
}

TEST_F(TimerTest, NextFireInterval_OneShotZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_FLOAT_EQ(0.0, timer.NextFireInterval());
}

TEST_F(TimerTest, NextFireInterval_OneShotNonZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  EXPECT_FLOAT_EQ(10.0, timer.NextFireInterval());
}

TEST_F(TimerTest, NextFireInterval_OneShotNonZero_AfterAFewSeconds) {
  platform_->SetAutoAdvanceNowToPendingTasks(false);

  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  platform_->AdvanceClockSeconds(2.0);
  EXPECT_FLOAT_EQ(8.0, timer.NextFireInterval());
}

TEST_F(TimerTest, NextFireInterval_Repeating) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(20), FROM_HERE);

  EXPECT_FLOAT_EQ(20.0, timer.NextFireInterval());
}

TEST_F(TimerTest, RepeatInterval_NeverStarted) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);

  EXPECT_FLOAT_EQ(0.0, timer.RepeatInterval());
}

TEST_F(TimerTest, RepeatInterval_OneShotZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_FLOAT_EQ(0.0, timer.RepeatInterval());
}

TEST_F(TimerTest, RepeatInterval_OneShotNonZero) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta::FromSeconds(10), FROM_HERE);

  EXPECT_FLOAT_EQ(0.0, timer.RepeatInterval());
}

TEST_F(TimerTest, RepeatInterval_Repeating) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(20), FROM_HERE);

  EXPECT_FLOAT_EQ(20.0, timer.RepeatInterval());
}

TEST_F(TimerTest, AugmentRepeatInterval) {
  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(10), FROM_HERE);
  EXPECT_FLOAT_EQ(10.0, timer.RepeatInterval());
  EXPECT_FLOAT_EQ(10.0, timer.NextFireInterval());

  platform_->AdvanceClockSeconds(2.0);
  timer.AugmentRepeatInterval(10);

  EXPECT_FLOAT_EQ(20.0, timer.RepeatInterval());
  EXPECT_FLOAT_EQ(18.0, timer.NextFireInterval());

  // NOTE setAutoAdvanceNowToPendingTasks(true) (which uses
  // cc::OrderedSimpleTaskRunner) results in somewhat strange behavior of the
  // test clock which breaks this test.  Specifically the test clock advancing
  // logic ignores newly posted delayed tasks and advances too far.
  RunUntilDeadline(start_time_ + 50.0);
  EXPECT_THAT(run_times_, ElementsAre(start_time_ + 20.0, start_time_ + 40.0));
}

TEST_F(TimerTest, AugmentRepeatInterval_TimerFireDelayed) {
  platform_->SetAutoAdvanceNowToPendingTasks(false);

  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::CountingTask);
  timer.StartRepeating(TimeDelta::FromSeconds(10), FROM_HERE);
  EXPECT_FLOAT_EQ(10.0, timer.RepeatInterval());
  EXPECT_FLOAT_EQ(10.0, timer.NextFireInterval());

  platform_->AdvanceClockSeconds(123.0);  // Make the timer long overdue.
  timer.AugmentRepeatInterval(10);

  EXPECT_FLOAT_EQ(20.0, timer.RepeatInterval());
  // The timer is overdue so it should be scheduled to fire immediatly.
  EXPECT_FLOAT_EQ(0.0, timer.NextFireInterval());
}

TEST_F(TimerTest, RepeatingTimerDoesNotDrift) {
  platform_->SetAutoAdvanceNowToPendingTasks(false);

  TaskRunnerTimer<TimerTest> timer(
      platform_->GetMainThreadScheduler()->DefaultTaskRunner(), this,
      &TimerTest::RecordNextFireTimeTask);
  timer.StartRepeating(TimeDelta::FromSeconds(2), FROM_HERE);

  RecordNextFireTimeTask(
      &timer);  // Next scheduled task to run at m_startTime + 2.0

  // Simulate timer firing early. Next scheduled task to run at
  // m_startTime + 4.0
  platform_->AdvanceClockSeconds(1.9);
  RunUntilDeadline(CurrentTimeTicksInSeconds() + 0.2);

  // Next scheduled task to run at m_startTime + 6.0
  platform_->RunForPeriodSeconds(2.0);
  // Next scheduled task to run at m_startTime + 8.0
  platform_->RunForPeriodSeconds(2.1);
  // Next scheduled task to run at m_startTime + 10.0
  platform_->RunForPeriodSeconds(2.9);
  // Next scheduled task to run at m_startTime + 14.0 (skips a beat)
  platform_->AdvanceClockSeconds(3.1);
  platform_->RunUntilIdle();
  // Next scheduled task to run at m_startTime + 18.0 (skips a beat)
  platform_->AdvanceClockSeconds(4.0);
  platform_->RunUntilIdle();
  // Next scheduled task to run at m_startTime + 28.0 (skips 5 beats)
  platform_->AdvanceClockSeconds(10.0);
  platform_->RunUntilIdle();

  EXPECT_THAT(
      next_fire_times_,
      ElementsAre(start_time_ + 2.0, start_time_ + 4.0, start_time_ + 6.0,
                  start_time_ + 8.0, start_time_ + 10.0, start_time_ + 14.0,
                  start_time_ + 18.0, start_time_ + 28.0));
}

template <typename TimerFiredClass>
class TimerForTest : public TaskRunnerTimer<TimerFiredClass> {
 public:
  using TimerFiredFunction =
      typename TaskRunnerTimer<TimerFiredClass>::TimerFiredFunction;

  ~TimerForTest() override = default;

  TimerForTest(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
               TimerFiredClass* timer_fired_class,
               TimerFiredFunction timer_fired_function)
      : TaskRunnerTimer<TimerFiredClass>(std::move(task_runner),
                                         timer_fired_class,
                                         timer_fired_function) {}
};

TEST_F(TimerTest, UserSuppliedTaskRunner) {
  scoped_refptr<TaskQueue> task_runner(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type =
      scheduler::TaskQueueWithTaskType::Create(task_runner,
                                               TaskType::kInternalTest);
  TimerForTest<TimerTest> timer(task_queue_with_task_type, this,
                                &TimerTest::CountingTask);
  timer.StartOneShot(TimeDelta(), FROM_HERE);

  // Make sure the task was posted on taskRunner.
  EXPECT_FALSE(task_runner->IsEmpty());
}

TEST_F(TimerTest, RunOnHeapTimer) {
  scoped_refptr<OnHeapTimerOwner::Record> record =
      OnHeapTimerOwner::Record::Create();
  Persistent<OnHeapTimerOwner> owner = new OnHeapTimerOwner(
      record, platform_->GetMainThreadScheduler()->DefaultTaskRunner());

  owner->StartOneShot(TimeDelta(), FROM_HERE);

  EXPECT_FALSE(record->TimerHasFired());
  platform_->RunUntilIdle();
  EXPECT_TRUE(record->TimerHasFired());
}

TEST_F(TimerTest, DestructOnHeapTimer) {
  scoped_refptr<OnHeapTimerOwner::Record> record =
      OnHeapTimerOwner::Record::Create();
  Persistent<OnHeapTimerOwner> owner = new OnHeapTimerOwner(
      record, platform_->GetMainThreadScheduler()->DefaultTaskRunner());

  record->Dispose();
  owner->StartOneShot(TimeDelta(), FROM_HERE);

  owner = nullptr;
  ThreadState::Current()->CollectGarbage(
      BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kEagerSweeping, BlinkGC::kForcedGC);
  EXPECT_TRUE(record->OwnerIsDestructed());

  EXPECT_FALSE(record->TimerHasFired());
  platform_->RunUntilIdle();
  EXPECT_FALSE(record->TimerHasFired());
}

TEST_F(TimerTest, MarkOnHeapTimerAsUnreachable) {
  scoped_refptr<OnHeapTimerOwner::Record> record =
      OnHeapTimerOwner::Record::Create();
  Persistent<OnHeapTimerOwner> owner = new OnHeapTimerOwner(
      record, platform_->GetMainThreadScheduler()->DefaultTaskRunner());

  record->Dispose();
  owner->StartOneShot(TimeDelta(), FROM_HERE);

  owner = nullptr;
  ThreadState::Current()->CollectGarbage(
      BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kLazySweeping, BlinkGC::kForcedGC);
  EXPECT_FALSE(record->OwnerIsDestructed());

  {
    GCForbiddenScope scope;
    EXPECT_FALSE(record->TimerHasFired());
    platform_->RunUntilIdle();
    EXPECT_FALSE(record->TimerHasFired());
    EXPECT_FALSE(record->OwnerIsDestructed());
  }
}

namespace {

class TaskObserver : public base::MessageLoop::TaskObserver {
 public:
  TaskObserver(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      std::vector<scoped_refptr<base::SingleThreadTaskRunner>>* run_order)
      : task_runner_(std::move(task_runner)), run_order_(run_order) {}

  void WillProcessTask(const base::PendingTask&) override {}

  void DidProcessTask(const base::PendingTask&) override {
    run_order_->push_back(task_runner_);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::vector<scoped_refptr<base::SingleThreadTaskRunner>>* run_order_;
};

}  // namespace

TEST_F(TimerTest, MoveToNewTaskRunnerOneShot) {
  std::vector<scoped_refptr<base::SingleThreadTaskRunner>> run_order;

  scoped_refptr<TaskQueue> task_runner1(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type1 =
      scheduler::TaskQueueWithTaskType::Create(task_runner1,
                                               TaskType::kInternalTest);
  TaskObserver task_observer1(task_queue_with_task_type1, &run_order);
  task_runner1->AddTaskObserver(&task_observer1);

  scoped_refptr<TaskQueue> task_runner2(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type2 =
      scheduler::TaskQueueWithTaskType::Create(task_runner2,
                                               TaskType::kInternalTest);
  TaskObserver task_observer2(task_queue_with_task_type2, &run_order);
  task_runner2->AddTaskObserver(&task_observer2);

  TimerForTest<TimerTest> timer(task_queue_with_task_type1, this,
                                &TimerTest::CountingTask);

  double start_time = CurrentTimeTicksInSeconds();

  timer.StartOneShot(TimeDelta::FromSeconds(1), FROM_HERE);

  platform_->RunForPeriodSeconds(0.5);

  timer.MoveToNewTaskRunner(task_queue_with_task_type2);

  platform_->RunUntilIdle();

  EXPECT_THAT(run_times_, ElementsAre(start_time + 1.0));

  EXPECT_THAT(run_order, ElementsAre(task_queue_with_task_type2));

  EXPECT_TRUE(task_runner1->IsEmpty());
  EXPECT_TRUE(task_runner2->IsEmpty());
}

TEST_F(TimerTest, MoveToNewTaskRunnerRepeating) {
  std::vector<scoped_refptr<base::SingleThreadTaskRunner>> run_order;

  scoped_refptr<TaskQueue> task_runner1(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type1 =
      scheduler::TaskQueueWithTaskType::Create(task_runner1,
                                               TaskType::kInternalTest);
  TaskObserver task_observer1(task_queue_with_task_type1, &run_order);
  task_runner1->AddTaskObserver(&task_observer1);

  scoped_refptr<TaskQueue> task_runner2(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type2 =
      scheduler::TaskQueueWithTaskType::Create(task_runner2,
                                               TaskType::kInternalTest);
  TaskObserver task_observer2(task_queue_with_task_type2, &run_order);
  task_runner2->AddTaskObserver(&task_observer2);

  TimerForTest<TimerTest> timer(task_queue_with_task_type1, this,
                                &TimerTest::CountingTask);

  double start_time = CurrentTimeTicksInSeconds();

  timer.StartRepeating(TimeDelta::FromSeconds(1), FROM_HERE);

  platform_->RunForPeriodSeconds(2.5);

  timer.MoveToNewTaskRunner(task_queue_with_task_type2);

  platform_->RunForPeriodSeconds(2);

  EXPECT_THAT(run_times_, ElementsAre(start_time + 1.0, start_time + 2.0,
                                      start_time + 3.0, start_time + 4.0));

  EXPECT_THAT(
      run_order,
      ElementsAre(task_queue_with_task_type1, task_queue_with_task_type1,
                  task_queue_with_task_type2, task_queue_with_task_type2));

  EXPECT_TRUE(task_runner1->IsEmpty());
  EXPECT_FALSE(task_runner2->IsEmpty());
}

// This test checks that when inactive timer is moved to a different task
// runner it isn't activated.
TEST_F(TimerTest, MoveToNewTaskRunnerWithoutTasks) {
  scoped_refptr<TaskQueue> task_runner1(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type1 =
      scheduler::TaskQueueWithTaskType::Create(task_runner1,
                                               TaskType::kInternalTest);

  scoped_refptr<TaskQueue> task_runner2(
      platform_->GetMainThreadScheduler()->NewTimerTaskQueue(
          scheduler::MainThreadTaskQueue::QueueType::kFrameThrottleable,
          nullptr));
  scoped_refptr<scheduler::TaskQueueWithTaskType> task_queue_with_task_type2 =
      scheduler::TaskQueueWithTaskType::Create(task_runner2,
                                               TaskType::kInternalTest);

  TimerForTest<TimerTest> timer(task_queue_with_task_type1, this,
                                &TimerTest::CountingTask);

  platform_->RunUntilIdle();
  EXPECT_TRUE(!run_times_.size());
  EXPECT_TRUE(task_runner1->IsEmpty());
  EXPECT_TRUE(task_runner2->IsEmpty());
}

}  // namespace
}  // namespace blink
