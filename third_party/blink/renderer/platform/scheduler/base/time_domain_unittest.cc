// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/time_domain.h"

#include <memory>
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/test/simple_test_tick_clock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager.h"
#include "third_party/blink/renderer/platform/scheduler/base/work_queue.h"

using testing::_;
using testing::AnyNumber;
using testing::Mock;

namespace base {
namespace sequence_manager {

class TaskQueueImplForTest : public internal::TaskQueueImpl {
 public:
  TaskQueueImplForTest(TaskQueueManagerImpl* task_queue_manager,
                       TimeDomain* time_domain,
                       const TaskQueue::Spec& spec)
      : TaskQueueImpl(task_queue_manager, time_domain, spec) {}
  ~TaskQueueImplForTest() {}

  using TaskQueueImpl::SetDelayedWakeUpForTesting;
};

class MockTimeDomain : public TimeDomain {
 public:
  MockTimeDomain() : now_(TimeTicks() + TimeDelta::FromSeconds(1)) {}

  ~MockTimeDomain() override = default;

  using TimeDomain::NextScheduledRunTime;
  using TimeDomain::NextScheduledTaskQueue;
  using TimeDomain::UnregisterQueue;
  using TimeDomain::WakeUpReadyDelayedQueues;

  // TimeSource implementation:
  LazyNow CreateLazyNow() const override { return LazyNow(now_); }
  TimeTicks Now() const override { return now_; }

  void AsValueIntoInternal(trace_event::TracedValue* state) const override {}

  Optional<TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override {
    return Optional<TimeDelta>();
  }
  const char* GetName() const override { return "Test"; }
  void OnRegisterWithTaskQueueManager(
      TaskQueueManagerImpl* task_queue_manager) override {}

  MOCK_METHOD2(RequestWakeUpAt, void(TimeTicks now, TimeTicks run_time));

  MOCK_METHOD1(CancelWakeUpAt, void(TimeTicks run_time));

  void SetNow(TimeTicks now) { now_ = now; }

 private:
  TimeTicks now_;

  DISALLOW_COPY_AND_ASSIGN(MockTimeDomain);
};

class TimeDomainTest : public testing::Test {
 public:
  void SetUp() final {
    time_domain_ = WrapUnique(CreateMockTimeDomain());
    task_queue_ = std::make_unique<TaskQueueImplForTest>(
        nullptr, time_domain_.get(), TaskQueue::Spec("test"));
  }

  void TearDown() final {
    if (task_queue_)
      task_queue_->UnregisterTaskQueue();
  }

  virtual MockTimeDomain* CreateMockTimeDomain() {
    return new MockTimeDomain();
  }

  std::unique_ptr<MockTimeDomain> time_domain_;
  std::unique_ptr<TaskQueueImplForTest> task_queue_;
};

TEST_F(TimeDomainTest, ScheduleWakeUpForQueue) {
  TimeDelta delay = TimeDelta::FromMilliseconds(10);
  TimeTicks delayed_runtime = time_domain_->Now() + delay;
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, delayed_runtime));
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{now + delay, 0});

  TimeTicks next_scheduled_runtime;
  EXPECT_TRUE(time_domain_->NextScheduledRunTime(&next_scheduled_runtime));
  EXPECT_EQ(delayed_runtime, next_scheduled_runtime);

  internal::TaskQueueImpl* next_task_queue;
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue_.get(), next_task_queue);
  Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(_)).Times(AnyNumber());
}

TEST_F(TimeDomainTest, ScheduleWakeUpForQueueSupersedesPreviousWakeUp) {
  TimeDelta delay1 = TimeDelta::FromMilliseconds(10);
  TimeDelta delay2 = TimeDelta::FromMilliseconds(100);
  TimeTicks delayed_runtime1 = time_domain_->Now() + delay1;
  TimeTicks delayed_runtime2 = time_domain_->Now() + delay2;
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, delayed_runtime1));
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{delayed_runtime1, 0});

  TimeTicks next_scheduled_runtime;
  EXPECT_TRUE(time_domain_->NextScheduledRunTime(&next_scheduled_runtime));
  EXPECT_EQ(delayed_runtime1, next_scheduled_runtime);

  Mock::VerifyAndClearExpectations(time_domain_.get());

  // Now schedule a later wake_up, which should replace the previously
  // requested one.
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, delayed_runtime2));
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{delayed_runtime2, 0});

  EXPECT_TRUE(time_domain_->NextScheduledRunTime(&next_scheduled_runtime));
  EXPECT_EQ(delayed_runtime2, next_scheduled_runtime);
  Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(_)).Times(AnyNumber());
}

TEST_F(TimeDomainTest, RequestWakeUpAt_OnlyCalledForEarlierTasks) {
  std::unique_ptr<TaskQueueImplForTest> task_queue2 =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  std::unique_ptr<TaskQueueImplForTest> task_queue3 =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  std::unique_ptr<TaskQueueImplForTest> task_queue4 =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  TimeDelta delay1 = TimeDelta::FromMilliseconds(10);
  TimeDelta delay2 = TimeDelta::FromMilliseconds(20);
  TimeDelta delay3 = TimeDelta::FromMilliseconds(30);
  TimeDelta delay4 = TimeDelta::FromMilliseconds(1);

  // RequestWakeUpAt should always be called if there are no other wake-ups.
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, now + delay1));
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{now + delay1, 0});

  Mock::VerifyAndClearExpectations(time_domain_.get());

  // RequestWakeUpAt should not be called when scheduling later tasks.
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, _)).Times(0);
  task_queue2->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{now + delay2, 0});
  task_queue3->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{now + delay3, 0});

  // RequestWakeUpAt should be called when scheduling earlier tasks.
  Mock::VerifyAndClearExpectations(time_domain_.get());
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, now + delay4));
  task_queue4->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{now + delay4, 0});

  Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, _));
  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(_)).Times(2);
  task_queue2->UnregisterTaskQueue();
  task_queue3->UnregisterTaskQueue();
  task_queue4->UnregisterTaskQueue();
}

TEST_F(TimeDomainTest, UnregisterQueue) {
  std::unique_ptr<TaskQueueImplForTest> task_queue2_ =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  TimeTicks wake_up1 = now + TimeDelta::FromMilliseconds(10);
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, wake_up1)).Times(1);
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{wake_up1, 0});
  TimeTicks wake_up2 = now + TimeDelta::FromMilliseconds(100);
  task_queue2_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{wake_up2, 0});

  internal::TaskQueueImpl* next_task_queue;
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue_.get(), next_task_queue);

  testing::Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(wake_up1)).Times(1);
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, wake_up2)).Times(1);

  time_domain_->UnregisterQueue(task_queue_.get());
  task_queue_ = std::unique_ptr<TaskQueueImplForTest>();
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue2_.get(), next_task_queue);

  testing::Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(wake_up2)).Times(1);

  time_domain_->UnregisterQueue(task_queue2_.get());
  EXPECT_FALSE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
}

TEST_F(TimeDomainTest, WakeUpReadyDelayedQueues) {
  TimeDelta delay = TimeDelta::FromMilliseconds(50);
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now_1(now);
  TimeTicks delayed_runtime = now + delay;
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, delayed_runtime));
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{delayed_runtime, 0});

  TimeTicks next_run_time;
  ASSERT_TRUE(time_domain_->NextScheduledRunTime(&next_run_time));
  EXPECT_EQ(delayed_runtime, next_run_time);

  time_domain_->WakeUpReadyDelayedQueues(&lazy_now_1);
  ASSERT_TRUE(time_domain_->NextScheduledRunTime(&next_run_time));
  EXPECT_EQ(delayed_runtime, next_run_time);

  time_domain_->SetNow(delayed_runtime);
  LazyNow lazy_now_2(time_domain_->CreateLazyNow());
  time_domain_->WakeUpReadyDelayedQueues(&lazy_now_2);
  ASSERT_FALSE(time_domain_->NextScheduledRunTime(&next_run_time));
}

TEST_F(TimeDomainTest, WakeUpReadyDelayedQueuesWithIdenticalRuntimes) {
  int sequence_num = 0;
  TimeDelta delay = TimeDelta::FromMilliseconds(50);
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  TimeTicks delayed_runtime = now + delay;
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, delayed_runtime));
  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(delayed_runtime));

  std::unique_ptr<TaskQueueImplForTest> task_queue2 =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  task_queue2->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{delayed_runtime, ++sequence_num});
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{delayed_runtime, ++sequence_num});

  time_domain_->WakeUpReadyDelayedQueues(&lazy_now);

  // The second task queue should wake up first since it has a lower sequence
  // number.
  internal::TaskQueueImpl* next_task_queue;
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue2.get(), next_task_queue);

  task_queue2->UnregisterTaskQueue();
}

TEST_F(TimeDomainTest, CancelDelayedWork) {
  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  TimeTicks run_time = now + TimeDelta::FromMilliseconds(20);

  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, run_time));
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{run_time, 0});

  internal::TaskQueueImpl* next_task_queue;
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue_.get(), next_task_queue);

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(run_time));
  task_queue_->SetDelayedWakeUpForTesting(nullopt);
  EXPECT_FALSE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
}

TEST_F(TimeDomainTest, CancelDelayedWork_TwoQueues) {
  std::unique_ptr<TaskQueueImplForTest> task_queue2 =
      std::make_unique<TaskQueueImplForTest>(nullptr, time_domain_.get(),
                                             TaskQueue::Spec("test"));

  TimeTicks now = time_domain_->Now();
  LazyNow lazy_now(now);
  TimeTicks run_time1 = now + TimeDelta::FromMilliseconds(20);
  TimeTicks run_time2 = now + TimeDelta::FromMilliseconds(40);
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, run_time1));
  task_queue_->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{run_time1, 0});
  Mock::VerifyAndClearExpectations(time_domain_.get());

  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, _)).Times(0);
  task_queue2->SetDelayedWakeUpForTesting(
      internal::TaskQueueImpl::DelayedWakeUp{run_time2, 0});
  Mock::VerifyAndClearExpectations(time_domain_.get());

  internal::TaskQueueImpl* next_task_queue;
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue_.get(), next_task_queue);

  TimeTicks next_run_time;
  ASSERT_TRUE(time_domain_->NextScheduledRunTime(&next_run_time));
  EXPECT_EQ(run_time1, next_run_time);

  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(run_time1));
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, run_time2));
  task_queue_->SetDelayedWakeUpForTesting(nullopt);
  EXPECT_TRUE(time_domain_->NextScheduledTaskQueue(&next_task_queue));
  EXPECT_EQ(task_queue2.get(), next_task_queue);

  ASSERT_TRUE(time_domain_->NextScheduledRunTime(&next_run_time));
  EXPECT_EQ(run_time2, next_run_time);

  Mock::VerifyAndClearExpectations(time_domain_.get());
  EXPECT_CALL(*time_domain_.get(), RequestWakeUpAt(_, _)).Times(AnyNumber());
  EXPECT_CALL(*time_domain_.get(), CancelWakeUpAt(_)).Times(AnyNumber());

  // Tidy up.
  task_queue2->UnregisterTaskQueue();
}

}  // namespace sequence_manager
}  // namespace base
