// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/child/idle_canceled_delayed_task_sweeper.h"

#include "base/task/sequence_manager/lazy_now.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/child/idle_helper.h"
#include "third_party/blink/renderer/platform/scheduler/common/scheduler_helper.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_helper.h"

namespace blink {
namespace scheduler {

class TestClass {
 public:
  TestClass() : weak_factory_(this) {}

  void NopTask() {}

  base::WeakPtrFactory<TestClass> weak_factory_;
};

class IdleCanceledDelayedTaskSweeperTest : public testing::Test,
                                           public IdleHelper::Delegate {
 public:
  IdleCanceledDelayedTaskSweeperTest()
      : mock_task_runner_(new cc::OrderedSimpleTaskRunner(&clock_, true)),
        scheduler_helper_(new MainThreadSchedulerHelper(
            base::sequence_manager::TaskQueueManagerForTest::Create(
                nullptr,
                mock_task_runner_,
                &clock_),
            nullptr)),
        idle_helper_(
            new IdleHelper(scheduler_helper_.get(),
                           this,
                           "test",
                           base::TimeDelta::FromSeconds(30),
                           scheduler_helper_->NewTaskQueue(
                               MainThreadTaskQueue::QueueCreationParams(
                                   MainThreadTaskQueue::QueueType::kTest)))),
        idle_canceled_delayed_taks_sweeper_(
            new IdleCanceledDelayedTaskSweeper(scheduler_helper_.get(),
                                               idle_helper_->IdleTaskRunner())),
        default_task_queue_(scheduler_helper_->DefaultMainThreadTaskQueue()) {
    clock_.Advance(base::TimeDelta::FromMicroseconds(5000));
  }

  ~IdleCanceledDelayedTaskSweeperTest() override = default;

  void TearDown() override {
    // Check that all tests stop posting tasks.
    mock_task_runner_->SetAutoAdvanceNowToPendingTasks(true);
    while (mock_task_runner_->RunUntilIdle()) {
    }
  }

  // IdleHelper::Delegate implementation:
  bool CanEnterLongIdlePeriod(
      base::TimeTicks now,
      base::TimeDelta* next_long_idle_period_delay_out) override {
    return true;
  }
  void IsNotQuiescent() override {}
  void OnIdlePeriodStarted() override {}
  void OnIdlePeriodEnded() override {}
  void OnPendingTasksChanged(bool has_tasks) override {}

 protected:
  base::SimpleTestTickClock clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;

  std::unique_ptr<MainThreadSchedulerHelper> scheduler_helper_;
  std::unique_ptr<IdleHelper> idle_helper_;
  std::unique_ptr<IdleCanceledDelayedTaskSweeper>
      idle_canceled_delayed_taks_sweeper_;
  scoped_refptr<base::sequence_manager::TaskQueue> default_task_queue_;

  DISALLOW_COPY_AND_ASSIGN(IdleCanceledDelayedTaskSweeperTest);
};

TEST_F(IdleCanceledDelayedTaskSweeperTest, TestSweep) {
  TestClass class1;
  TestClass class2;

  // Post one task we won't cancel.
  default_task_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TestClass::NopTask, class1.weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(100));

  // And a bunch we will.
  default_task_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TestClass::NopTask, class2.weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(101));

  default_task_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TestClass::NopTask, class2.weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(102));

  default_task_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TestClass::NopTask, class2.weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(103));

  default_task_queue_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&TestClass::NopTask, class2.weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(104));

  // Cancel the last four tasks.
  class2.weak_factory_.InvalidateWeakPtrs();

  // Give the IdleCanceledDelayedTaskSweeper a chance to run but don't let
  // the first non canceled delayed task run.  This is important because the
  // canceled tasks would get removed by TaskQueueImpl::WakeUpForDelayedWork.
  clock_.Advance(base::TimeDelta::FromSeconds(40));
  idle_helper_->EnableLongIdlePeriod();
  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(40));

  EXPECT_EQ(1u, default_task_queue_->GetNumberOfPendingTasks());
}

}  // namespace scheduler
}  // namespace blink
