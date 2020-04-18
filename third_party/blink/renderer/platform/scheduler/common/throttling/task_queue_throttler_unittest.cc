// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/common/throttling/task_queue_throttler.h"

#include <stddef.h>

#include <deque>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/scheduler/base/real_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/common/throttling/budget_pool.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/auto_advancing_virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"

using base::sequence_manager::LazyNow;
using base::sequence_manager::TaskQueue;
using testing::ElementsAre;

namespace blink {
namespace scheduler {
// To avoid symbol collisions in jumbo builds.
namespace task_queue_throttler_unittest {

class MainThreadSchedulerImplForTest : public MainThreadSchedulerImpl {
 public:
  using MainThreadSchedulerImpl::ControlTaskQueue;

  MainThreadSchedulerImplForTest(
      std::unique_ptr<base::sequence_manager::TaskQueueManager> manager,
      base::Optional<base::Time> initial_virtual_time)
      : MainThreadSchedulerImpl(std::move(manager), initial_virtual_time) {}
};

bool MessageLoopTaskCounter(size_t* count) {
  *count = *count + 1;
  return true;
}

void NopTask() {}

void AddOneTask(size_t* count) {
  (*count)++;
}

void RunTenTimesTask(size_t* count, scoped_refptr<TaskQueue> timer_queue) {
  if (++(*count) < 10) {
    timer_queue->PostTask(FROM_HERE,
                          base::BindOnce(&RunTenTimesTask, count, timer_queue));
  }
}

// Test clock which simulates passage of time by automatically
// advancing time with each call to Now().
class AutoAdvancingTestClock : public base::SimpleTestTickClock {
 public:
  AutoAdvancingTestClock(base::TimeDelta interval)
      : advancing_interval_(interval) {}
  ~AutoAdvancingTestClock() override = default;

  base::TimeTicks NowTicks() const override {
    const_cast<AutoAdvancingTestClock*>(this)->Advance(advancing_interval_);
    return SimpleTestTickClock::NowTicks();
  }

  base::TimeTicks GetNowTicksWithoutAdvancing() {
    return SimpleTestTickClock::NowTicks();
  }

 private:
  base::TimeDelta advancing_interval_;
};

class TaskQueueThrottlerTest : public testing::Test {
 public:
  TaskQueueThrottlerTest() = default;
  ~TaskQueueThrottlerTest() override = default;

  void SetUp() override {
    clock_ = CreateClock();
    clock_->Advance(base::TimeDelta::FromMicroseconds(5000));
    mock_task_runner_ =
        base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(clock_.get(), true);
    scheduler_.reset(new MainThreadSchedulerImplForTest(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            nullptr, mock_task_runner_, clock_.get()),
        base::nullopt));
    scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
        base::TimeDelta());
    task_queue_throttler_ = scheduler_->task_queue_throttler();
    timer_queue_ = scheduler_->NewTimerTaskQueue(
        MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);
  }

  void TearDown() override {
    scheduler_->Shutdown();
    scheduler_.reset();
  }

  void ExpectThrottled(scoped_refptr<TaskQueue> timer_queue) {
    size_t count = 0;
    timer_queue->PostTask(
        FROM_HERE, base::BindOnce(&RunTenTimesTask, &count, timer_queue));

    mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
    EXPECT_LE(count, 1u);

    // Make sure the rest of the tasks run or we risk a UAF on |count|.
    mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(10));
    EXPECT_EQ(10u, count);
  }

  void ExpectUnthrottled(scoped_refptr<TaskQueue> timer_queue) {
    size_t count = 0;
    timer_queue->PostTask(
        FROM_HERE, base::BindOnce(&RunTenTimesTask, &count, timer_queue));

    mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
    EXPECT_EQ(10u, count);
    mock_task_runner_->RunUntilIdle();
  }

  bool IsQueueBlocked(TaskQueue* task_queue) {
    base::sequence_manager::internal::TaskQueueImpl* task_queue_impl =
        task_queue->GetTaskQueueImpl();
    if (!task_queue_impl->IsQueueEnabled())
      return true;
    return task_queue_impl->GetFenceForTest() ==
           static_cast<base::sequence_manager::internal::EnqueueOrder>(
               base::sequence_manager::internal::EnqueueOrderValues::
                   kBlockingFence);
  }

 protected:
  virtual std::unique_ptr<AutoAdvancingTestClock> CreateClock() {
    return std::make_unique<AutoAdvancingTestClock>(base::TimeDelta());
  }

  std::unique_ptr<AutoAdvancingTestClock> clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  std::unique_ptr<MainThreadSchedulerImplForTest> scheduler_;
  scoped_refptr<TaskQueue> timer_queue_;
  TaskQueueThrottler* task_queue_throttler_;  // NOT OWNED

  DISALLOW_COPY_AND_ASSIGN(TaskQueueThrottlerTest);
};

class TaskQueueThrottlerWithAutoAdvancingTimeTest
    : public TaskQueueThrottlerTest,
      public testing::WithParamInterface<bool> {
 public:
  TaskQueueThrottlerWithAutoAdvancingTimeTest()
      : auto_advance_time_interval_(GetParam()
                                        ? base::TimeDelta::FromMicroseconds(1)
                                        : base::TimeDelta()) {}
  ~TaskQueueThrottlerWithAutoAdvancingTimeTest() override = default;

 protected:
  std::unique_ptr<AutoAdvancingTestClock> CreateClock() override {
    return std::make_unique<AutoAdvancingTestClock>(
        auto_advance_time_interval_);
  }

  base::TimeDelta auto_advance_time_interval_;

  DISALLOW_COPY_AND_ASSIGN(TaskQueueThrottlerWithAutoAdvancingTimeTest);
};

INSTANTIATE_TEST_CASE_P(All,
                        TaskQueueThrottlerWithAutoAdvancingTimeTest,
                        testing::Bool());

TEST_F(TaskQueueThrottlerTest, ThrottledTasksReportRealTime) {
  EXPECT_EQ(timer_queue_->GetTimeDomain()->Now(),
            clock_->GetNowTicksWithoutAdvancing());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_EQ(timer_queue_->GetTimeDomain()->Now(),
            clock_->GetNowTicksWithoutAdvancing());

  clock_->Advance(base::TimeDelta::FromMilliseconds(250));
  // Make sure the throttled time domain's Now() reports the same as the
  // underlying clock.
  EXPECT_EQ(timer_queue_->GetTimeDomain()->Now(),
            clock_->GetNowTicksWithoutAdvancing());
}

TEST_F(TaskQueueThrottlerTest, AlignedThrottledRunTime) {
  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.0)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.1)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.2)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.5)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.8)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(0.9)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(2.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(1.0)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(2.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(1.1)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(9.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(8.0)));

  EXPECT_EQ(base::TimeTicks() + base::TimeDelta::FromSecondsD(9.0),
            TaskQueueThrottler::AlignedThrottledRunTime(
                base::TimeTicks() + base::TimeDelta::FromSecondsD(8.1)));
}

namespace {

// Round up time to milliseconds to deal with autoadvancing time.
// TODO(altimin): round time only when autoadvancing time is enabled.
base::TimeDelta RoundTimeToMilliseconds(base::TimeDelta time) {
  return time - time % base::TimeDelta::FromMilliseconds(1);
}

base::TimeTicks RoundTimeToMilliseconds(base::TimeTicks time) {
  return base::TimeTicks() + RoundTimeToMilliseconds(time - base::TimeTicks());
}

void TestTask(std::vector<base::TimeTicks>* run_times,
              AutoAdvancingTestClock* clock) {
  run_times->push_back(
      RoundTimeToMilliseconds(clock->GetNowTicksWithoutAdvancing()));
}

void ExpensiveTestTask(std::vector<base::TimeTicks>* run_times,
                       AutoAdvancingTestClock* clock) {
  run_times->push_back(
      RoundTimeToMilliseconds(clock->GetNowTicksWithoutAdvancing()));
  clock->Advance(base::TimeDelta::FromMilliseconds(250));
}

void RecordThrottling(std::vector<base::TimeDelta>* reported_throttling_times,
                      base::TimeDelta throttling_duration) {
  reported_throttling_times->push_back(
      RoundTimeToMilliseconds(throttling_duration));
}
}  // namespace

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, TimerAlignment) {
  std::vector<base::TimeTicks> run_times;
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(800.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(1200.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(8300.0));

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  mock_task_runner_->RunUntilIdle();

  // Times are aligned to a multiple of 1000 milliseconds.
  EXPECT_THAT(
      run_times,
      ElementsAre(
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000.0),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000.0),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(2000.0),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(9000.0)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       TimerAlignment_Unthrottled) {
  std::vector<base::TimeTicks> run_times;
  base::TimeTicks start_time = clock_->GetNowTicksWithoutAdvancing();
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(800.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(1200.0));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(8300.0));

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());

  mock_task_runner_->RunUntilIdle();

  // Times are not aligned.
  EXPECT_THAT(
      run_times,
      ElementsAre(RoundTimeToMilliseconds(
                      start_time + base::TimeDelta::FromMilliseconds(200.0)),
                  RoundTimeToMilliseconds(
                      start_time + base::TimeDelta::FromMilliseconds(800.0)),
                  RoundTimeToMilliseconds(
                      start_time + base::TimeDelta::FromMilliseconds(1200.0)),
                  RoundTimeToMilliseconds(
                      start_time + base::TimeDelta::FromMilliseconds(8300.0))));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, Refcount) {
  ExpectUnthrottled(timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  ExpectThrottled(timer_queue_);

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  ExpectThrottled(timer_queue_);

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  ExpectThrottled(timer_queue_);

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  ExpectUnthrottled(timer_queue_);

  // Should be a NOP.
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  ExpectUnthrottled(timer_queue_);

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  ExpectThrottled(timer_queue_);
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       ThrotlingAnEmptyQueueDoesNotPostPumpThrottledTasksLocked) {
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  EXPECT_TRUE(scheduler_->ControlTaskQueue()->IsEmpty());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       OnTimeDomainHasImmediateWork_EnabledQueue) {
  task_queue_throttler_->OnQueueNextWakeUpChanged(timer_queue_.get(),
                                                  base::TimeTicks());
  // Check PostPumpThrottledTasksLocked was called.
  EXPECT_FALSE(scheduler_->ControlTaskQueue()->IsEmpty());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       OnTimeDomainHasImmediateWork_DisabledQueue) {
  std::unique_ptr<TaskQueue::QueueEnabledVoter> voter =
      timer_queue_->CreateQueueEnabledVoter();
  voter->SetQueueEnabled(false);

  task_queue_throttler_->OnQueueNextWakeUpChanged(timer_queue_.get(),
                                                  base::TimeTicks());
  // Check PostPumpThrottledTasksLocked was not called.
  EXPECT_TRUE(scheduler_->ControlTaskQueue()->IsEmpty());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       ThrottlingADisabledQueueDoesNotPostPumpThrottledTasks) {
  timer_queue_->PostTask(FROM_HERE, base::BindOnce(&NopTask));

  std::unique_ptr<TaskQueue::QueueEnabledVoter> voter =
      timer_queue_->CreateQueueEnabledVoter();
  voter->SetQueueEnabled(false);

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_TRUE(scheduler_->ControlTaskQueue()->IsEmpty());

  // Enabling it should trigger a call to PostPumpThrottledTasksLocked.
  voter->SetQueueEnabled(true);
  EXPECT_FALSE(scheduler_->ControlTaskQueue()->IsEmpty());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       ThrottlingADisabledQueueDoesNotPostPumpThrottledTasks_DelayedTask) {
  timer_queue_->PostDelayedTask(FROM_HERE, base::BindOnce(&NopTask),
                                base::TimeDelta::FromMilliseconds(1));

  std::unique_ptr<TaskQueue::QueueEnabledVoter> voter =
      timer_queue_->CreateQueueEnabledVoter();
  voter->SetQueueEnabled(false);

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_TRUE(scheduler_->ControlTaskQueue()->IsEmpty());

  // Enabling it should trigger a call to PostPumpThrottledTasksLocked.
  voter->SetQueueEnabled(true);
  EXPECT_FALSE(scheduler_->ControlTaskQueue()->IsEmpty());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, WakeUpForNonDelayedTask) {
  std::vector<base::TimeTicks> run_times;

  // Nothing is posted on timer_queue_ so PumpThrottledTasks will not tick.
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Posting a task should trigger the pump.
  timer_queue_->PostTask(FROM_HERE,
                         base::BindOnce(&TestTask, &run_times, clock_.get()));

  mock_task_runner_->RunUntilIdle();
  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() +
                          base::TimeDelta::FromMilliseconds(1000.0)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, WakeUpForDelayedTask) {
  std::vector<base::TimeTicks> run_times;

  // Nothing is posted on timer_queue_ so PumpThrottledTasks will not tick.
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Posting a task should trigger the pump.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(1200.0));

  mock_task_runner_->RunUntilIdle();
  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() +
                          base::TimeDelta::FromMilliseconds(2000.0)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       SingleThrottledTaskPumpedAndRunWithNoExtraneousMessageLoopTasks) {
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  base::TimeDelta delay(base::TimeDelta::FromMilliseconds(10));
  timer_queue_->PostDelayedTask(FROM_HERE, base::BindOnce(&NopTask), delay);

  size_t task_count = 0;
  mock_task_runner_->RunTasksWhile(
      base::BindRepeating(&MessageLoopTaskCounter, &task_count));

  // Run the task.
  EXPECT_EQ(1u, task_count);
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       SingleFutureThrottledTaskPumpedAndRunWithNoExtraneousMessageLoopTasks) {
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  base::TimeDelta delay(base::TimeDelta::FromSecondsD(15.5));
  timer_queue_->PostDelayedTask(FROM_HERE, base::BindOnce(&NopTask), delay);

  size_t task_count = 0;
  mock_task_runner_->RunTasksWhile(
      base::BindRepeating(&MessageLoopTaskCounter, &task_count));

  // Run the delayed task.
  EXPECT_EQ(1u, task_count);
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       TwoFutureThrottledTaskPumpedAndRunWithNoExtraneousMessageLoopTasks) {
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  std::vector<base::TimeTicks> run_times;

  base::TimeDelta delay(base::TimeDelta::FromSecondsD(15.5));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()), delay);

  base::TimeDelta delay2(base::TimeDelta::FromSecondsD(5.5));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()), delay2);

  size_t task_count = 0;
  mock_task_runner_->RunTasksWhile(
      base::BindRepeating(&MessageLoopTaskCounter, &task_count));

  // Run both delayed tasks.
  EXPECT_EQ(2u, task_count);

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(6),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(16)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       TaskDelayIsBasedOnRealTime) {
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Post an initial task that should run at the first aligned time period.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(900.0));

  mock_task_runner_->RunUntilIdle();

  // Advance realtime.
  clock_->Advance(base::TimeDelta::FromMilliseconds(250));

  // Post a task that due to real time + delay must run in the third aligned
  // time period.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(900.0));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000.0),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000.0)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, TaskQueueDisabledTillPump) {
  size_t count = 0;
  timer_queue_->PostTask(FROM_HERE, base::BindOnce(&AddOneTask, &count));

  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_TRUE(IsQueueBlocked(timer_queue_.get()));

  mock_task_runner_->RunUntilIdle();  // Wait until the pump.
  EXPECT_EQ(1u, count);               // The task got run
  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       DoubleIncrementDoubleDecrement) {
  timer_queue_->PostTask(FROM_HERE, base::BindOnce(&NopTask));

  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_TRUE(IsQueueBlocked(timer_queue_.get()));
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       EnableVirtualTimeThenIncrement) {
  timer_queue_->PostTask(FROM_HERE, base::BindOnce(&NopTask));

  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  EXPECT_EQ(timer_queue_->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());

  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  EXPECT_EQ(timer_queue_->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       IncrementThenEnableVirtualTime) {
  timer_queue_->PostTask(FROM_HERE, base::BindOnce(&NopTask));

  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  EXPECT_TRUE(IsQueueBlocked(timer_queue_.get()));

  scheduler_->EnableVirtualTime(
      MainThreadSchedulerImpl::BaseTimeOverridePolicy::DO_NOT_OVERRIDE);
  EXPECT_FALSE(IsQueueBlocked(timer_queue_.get()));
  EXPECT_EQ(timer_queue_->GetTimeDomain(), scheduler_->GetVirtualTimeDomain());
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, TimeBasedThrottling) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Submit two tasks. They should be aligned, and second one should be
  // throttled.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(3)));

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  run_times.clear();

  // Queue was removed from CPUTimeBudgetPool, only timer alignment should be
  // active now.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(4000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(4250)));

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       EnableAndDisableCPUTimeBudgetPool) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  EXPECT_TRUE(pool->IsThrottlingEnabled());

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Post an expensive task. Pool is now throttled.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(1000)));
  run_times.clear();

  LazyNow lazy_now_1(clock_.get());
  pool->DisableThrottling(&lazy_now_1);
  EXPECT_FALSE(pool->IsThrottlingEnabled());

  // Pool should not be throttled now.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(2000)));
  run_times.clear();

  LazyNow lazy_now_2(clock_.get());
  pool->EnableThrottling(&lazy_now_2);
  EXPECT_TRUE(pool->IsThrottlingEnabled());

  // Because time pool was disabled, time budget level did not replenish
  // and queue is throttled.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(4000)));
  run_times.clear();

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       ImmediateTasksTimeBudgetThrottling) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Submit two tasks. They should be aligned, and second one should be
  // throttled.
  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(3)));

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  run_times.clear();

  // Queue was removed from CPUTimeBudgetPool, only timer alignment should be
  // active now.
  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(4000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(4250)));

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       TwoQueuesTimeBudgetThrottling) {
  std::vector<base::TimeTicks> run_times;

  scoped_refptr<TaskQueue> second_queue = scheduler_->NewTimerTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());
  pool->AddQueue(base::TimeTicks(), second_queue.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->IncreaseThrottleRefCount(second_queue.get());

  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  second_queue->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(3)));

  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->DecreaseThrottleRefCount(second_queue.get());

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), second_queue.get());

  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       DisabledTimeBudgetDoesNotAffectThrottledQueues) {
  std::vector<base::TimeTicks> run_times;
  LazyNow lazy_now(clock_.get());

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  pool->SetTimeBudgetRecoveryRate(lazy_now.Now(), 0.1);
  pool->DisableThrottling(&lazy_now);

  pool->AddQueue(lazy_now.Now(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(100));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1250)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       TimeBudgetThrottlingDoesNotAffectUnthrottledQueues) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);

  LazyNow lazy_now(clock_.get());
  pool->DisableThrottling(&lazy_now);

  pool->AddQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(100));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(105),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(355)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, MaxThrottlingDelay) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetMaxThrottlingDelay(base::TimeTicks(),
                              base::TimeDelta::FromMinutes(1));

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.001);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  for (int i = 0; i < 5; ++i) {
    timer_queue_->PostDelayedTask(
        FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
        base::TimeDelta::FromMilliseconds(200));
  }

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(62),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(123),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(184),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(245)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       EnableAndDisableThrottling) {
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(300));

  // Disable throttling - task should run immediately.
  task_queue_throttler_->DisableThrottling();

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(500));

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(300)));
  run_times.clear();

  // Schedule a task at 900ms. It should proceed as normal.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(400));

  // Schedule a task at 1200ms. It should proceed as normal.
  // PumpThrottledTasks was scheduled at 1000ms, so it needs to be checked
  // that it was cancelled and it does not interfere with tasks posted before
  // 1s mark and scheduled to run after 1s mark.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(700));

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(1300));

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(900),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1200)));
  run_times.clear();

  // Schedule a task at 1500ms. It should be throttled because of enabled
  // throttling.
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(1400));

  // Throttling is enabled and new task should be aligned.
  task_queue_throttler_->EnableThrottling();

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(2000)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, ReportThrottling) {
  std::vector<base::TimeTicks> run_times;
  std::vector<base::TimeDelta> reported_throttling_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  pool->SetReportingCallback(
      base::BindRepeating(&RecordThrottling, &reported_throttling_times));

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));
  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(3)));

  EXPECT_THAT(reported_throttling_times,
              ElementsAre(base::TimeDelta::FromMilliseconds(1255),
                          base::TimeDelta::FromMilliseconds(1755)));

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, GrantAdditionalBudget) {
  std::vector<base::TimeTicks> run_times;

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());
  pool->GrantAdditionalBudget(base::TimeTicks(),
                              base::TimeDelta::FromMilliseconds(500));

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  // Submit five tasks. First three will not be throttled because they have
  // budget to run.
  for (int i = 0; i < 5; ++i) {
    timer_queue_->PostDelayedTask(
        FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
        base::TimeDelta::FromMilliseconds(200));
  }

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1250),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1500),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(3),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(6)));

  pool->RemoveQueue(clock_->GetNowTicksWithoutAdvancing(), timer_queue_.get());
  task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
  pool->Close();
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       EnableAndDisableThrottlingAndTimeBudgets) {
  // This test checks that if time budget pool is enabled when throttling
  // is disabled, it does not throttle the queue.
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->DisableThrottling();

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  LazyNow lazy_now_1(clock_.get());
  pool->DisableThrottling(&lazy_now_1);

  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(100));

  LazyNow lazy_now_2(clock_.get());
  pool->EnableThrottling(&lazy_now_2);

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(300)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       AddQueueToBudgetPoolWhenThrottlingDisabled) {
  // This test checks that a task queue is added to time budget pool
  // when throttling is disabled, is does not throttle queue.
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->DisableThrottling();

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(100));

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(300)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       DisabledQueueThenEnabledQueue) {
  std::vector<base::TimeTicks> run_times;

  scoped_refptr<MainThreadTaskQueue> second_queue =
      scheduler_->NewTimerTaskQueue(
          MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->IncreaseThrottleRefCount(second_queue.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(100));
  second_queue->PostDelayedTask(
      FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
      base::TimeDelta::FromMilliseconds(200));

  std::unique_ptr<TaskQueue::QueueEnabledVoter> voter =
      timer_queue_->CreateQueueEnabledVoter();
  voter->SetQueueEnabled(false);

  clock_->Advance(base::TimeDelta::FromMilliseconds(250));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times, ElementsAre(base::TimeTicks() +
                                     base::TimeDelta::FromMilliseconds(1000)));

  voter->SetQueueEnabled(true);
  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2000)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest, TwoBudgetPools) {
  std::vector<base::TimeTicks> run_times;

  scoped_refptr<TaskQueue> second_queue = scheduler_->NewTimerTaskQueue(
      MainThreadTaskQueue::QueueType::kFrameThrottleable, nullptr);

  CPUTimeBudgetPool* pool1 =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  pool1->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool1->AddQueue(base::TimeTicks(), timer_queue_.get());
  pool1->AddQueue(base::TimeTicks(), second_queue.get());

  CPUTimeBudgetPool* pool2 =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");
  pool2->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.01);
  pool2->AddQueue(base::TimeTicks(), timer_queue_.get());

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
  task_queue_throttler_->IncreaseThrottleRefCount(second_queue.get());

  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  second_queue->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  timer_queue_->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));
  second_queue->PostTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(6000),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(26000)));
}

namespace {

void RunChainedTask(std::deque<base::TimeDelta> task_durations,
                    scoped_refptr<TaskQueue> queue,
                    AutoAdvancingTestClock* clock,
                    std::vector<base::TimeTicks>* run_times,
                    base::TimeDelta delay) {
  if (task_durations.empty())
    return;

  run_times->push_back(
      RoundTimeToMilliseconds(clock->GetNowTicksWithoutAdvancing()));
  clock->Advance(task_durations.front());
  task_durations.pop_front();

  queue->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&RunChainedTask, std::move(task_durations), queue, clock,
                     run_times, delay),
      delay);
}
}  // namespace

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       WakeUpBasedThrottling_ChainedTasks_Instantaneous) {
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&RunChainedTask,
                     std::deque<base::TimeDelta>(10, base::TimeDelta()),
                     timer_queue_, clock_.get(), &run_times, base::TimeDelta()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1),
                          base::TimeTicks() + base::TimeDelta::FromSeconds(1)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       WakeUpBasedThrottling_ImmediateTasks_Fast) {
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &RunChainedTask,
          std::deque<base::TimeDelta>(10, base::TimeDelta::FromMilliseconds(3)),
          timer_queue_, clock_.get(), &run_times, base::TimeDelta()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  // TODO(altimin): Add fence mechanism to block immediate tasks.
  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1003),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1006),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1009),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2003),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2006),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2009),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3003)));
}

TEST_P(TaskQueueThrottlerWithAutoAdvancingTimeTest,
       WakeUpBasedThrottling_DelayedTasks) {
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));
  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&RunChainedTask,
                     std::deque<base::TimeDelta>(10, base::TimeDelta()),
                     timer_queue_, clock_.get(), &run_times,
                     base::TimeDelta::FromMilliseconds(3)),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1003),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1006),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1009),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2003),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2006),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2009),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3003)));
}

TEST_F(TaskQueueThrottlerTest, WakeUpBasedThrottlingWithCPUBudgetThrottling) {
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &RunChainedTask,
          std::deque<base::TimeDelta>{base::TimeDelta::FromMilliseconds(250),
                                      base::TimeDelta(), base::TimeDelta(),
                                      base::TimeDelta::FromMilliseconds(250),
                                      base::TimeDelta(), base::TimeDelta(),
                                      base::TimeDelta::FromMilliseconds(250),
                                      base::TimeDelta(), base::TimeDelta()},
          timer_queue_, clock_.get(), &run_times, base::TimeDelta()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(8000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(8000)));
}

TEST_F(TaskQueueThrottlerTest,
       WakeUpBasedThrottlingWithCPUBudgetThrottling_OnAndOff) {
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.1);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  std::vector<base::TimeTicks> run_times;

  bool is_throttled = false;

  for (int i = 0; i < 5; ++i) {
    timer_queue_->PostDelayedTask(
        FROM_HERE, base::BindOnce(&ExpensiveTestTask, &run_times, clock_.get()),
        base::TimeDelta::FromMilliseconds(200));
    timer_queue_->PostDelayedTask(
        FROM_HERE, base::BindOnce(&TestTask, &run_times, clock_.get()),
        base::TimeDelta::FromMilliseconds(300));

    if (is_throttled) {
      task_queue_throttler_->DecreaseThrottleRefCount(timer_queue_.get());
      is_throttled = false;
    } else {
      task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());
      is_throttled = true;
    }

    mock_task_runner_->RunUntilIdle();
  }

  EXPECT_THAT(run_times,
              ElementsAre(
                  // Throttled due to cpu budget.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  // Unthrottled.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3200),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3450),
                  // Throttled due to wake-up budget. Old tasks still run.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(5000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(5250),
                  // Unthrottled.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6200),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6450),
                  // Throttled due to wake-up budget. Old tasks still run.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(8000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(8250)));
}

TEST_F(TaskQueueThrottlerTest,
       WakeUpBasedThrottlingWithCPUBudgetThrottling_ChainedFastTasks) {
  // This test checks that a new task should run during the wake-up window
  // when time budget allows that and should be blocked when time budget is
  // exhausted.
  scheduler_->GetWakeUpBudgetPoolForTesting()->SetWakeUpDuration(
      base::TimeDelta::FromMilliseconds(10));

  CPUTimeBudgetPool* pool =
      task_queue_throttler_->CreateCPUTimeBudgetPool("test");

  pool->SetTimeBudgetRecoveryRate(base::TimeTicks(), 0.01);
  pool->AddQueue(base::TimeTicks(), timer_queue_.get());

  std::vector<base::TimeTicks> run_times;

  task_queue_throttler_->IncreaseThrottleRefCount(timer_queue_.get());

  timer_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(
          &RunChainedTask,
          std::deque<base::TimeDelta>(10, base::TimeDelta::FromMilliseconds(7)),
          timer_queue_, clock_.get(), &run_times, base::TimeDelta()),
      base::TimeDelta::FromMilliseconds(100));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_times,
              ElementsAre(
                  // Time budget is ~10ms and we can run two 7ms tasks.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(1007),
                  // Time budget is ~6ms and we can run one 7ms task.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2000),
                  // Time budget is ~8ms and we can run two 7ms tasks.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(3007),
                  // Time budget is ~5ms and we can run one 7ms task.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(4000),
                  // Time budget is ~8ms and we can run two 7ms tasks.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(5000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(5007),
                  // Time budget is ~4ms and we can run one 7ms task.
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(6000),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(7000)));
}

}  // namespace task_queue_throttler_unittest
}  // namespace scheduler
}  // namespace blink
