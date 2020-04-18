// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/deadline_task_runner.h"

#include <memory>

#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace scheduler {

class DeadlineTaskRunnerTest : public testing::Test {
 public:
  DeadlineTaskRunnerTest() = default;
  ~DeadlineTaskRunnerTest() override = default;

  void SetUp() override {
    clock_.reset(new base::SimpleTestTickClock());
    clock_->Advance(base::TimeDelta::FromMicroseconds(5000));
    mock_task_runner_ = new cc::OrderedSimpleTaskRunner(clock_.get(), true);
    deadline_task_runner_.reset(new DeadlineTaskRunner(
        base::BindRepeating(&DeadlineTaskRunnerTest::TestTask,
                            base::Unretained(this)),
        mock_task_runner_));
    run_times_.clear();
  }

  bool RunUntilIdle() { return mock_task_runner_->RunUntilIdle(); }

  void TestTask() { run_times_.push_back(clock_->NowTicks()); }

  std::unique_ptr<base::SimpleTestTickClock> clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  std::unique_ptr<DeadlineTaskRunner> deadline_task_runner_;
  std::vector<base::TimeTicks> run_times_;
};

TEST_F(DeadlineTaskRunnerTest, RunOnce) {
  base::TimeTicks start_time = clock_->NowTicks();
  base::TimeDelta delay = base::TimeDelta::FromMilliseconds(10);
  deadline_task_runner_->SetDeadline(FROM_HERE, delay, clock_->NowTicks());
  RunUntilIdle();

  EXPECT_THAT(run_times_, testing::ElementsAre(start_time + delay));
};

TEST_F(DeadlineTaskRunnerTest, RunTwice) {
  base::TimeDelta delay1 = base::TimeDelta::FromMilliseconds(10);
  base::TimeTicks deadline1 = clock_->NowTicks() + delay1;
  deadline_task_runner_->SetDeadline(FROM_HERE, delay1, clock_->NowTicks());
  RunUntilIdle();

  base::TimeDelta delay2 = base::TimeDelta::FromMilliseconds(100);
  base::TimeTicks deadline2 = clock_->NowTicks() + delay2;
  deadline_task_runner_->SetDeadline(FROM_HERE, delay2, clock_->NowTicks());
  RunUntilIdle();

  EXPECT_THAT(run_times_, testing::ElementsAre(deadline1, deadline2));
};

TEST_F(DeadlineTaskRunnerTest, EarlierDeadlinesTakePrecidence) {
  base::TimeTicks start_time = clock_->NowTicks();
  base::TimeDelta delay1 = base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta delay10 = base::TimeDelta::FromMilliseconds(10);
  base::TimeDelta delay100 = base::TimeDelta::FromMilliseconds(100);
  deadline_task_runner_->SetDeadline(FROM_HERE, delay100, clock_->NowTicks());
  deadline_task_runner_->SetDeadline(FROM_HERE, delay10, clock_->NowTicks());
  deadline_task_runner_->SetDeadline(FROM_HERE, delay1, clock_->NowTicks());

  RunUntilIdle();

  EXPECT_THAT(run_times_, testing::ElementsAre(start_time + delay1));
};

TEST_F(DeadlineTaskRunnerTest, LaterDeadlinesIgnored) {
  base::TimeTicks start_time = clock_->NowTicks();
  base::TimeDelta delay100 = base::TimeDelta::FromMilliseconds(100);
  base::TimeDelta delay10000 = base::TimeDelta::FromMilliseconds(10000);
  deadline_task_runner_->SetDeadline(FROM_HERE, delay100, clock_->NowTicks());
  deadline_task_runner_->SetDeadline(FROM_HERE, delay10000, clock_->NowTicks());

  RunUntilIdle();

  EXPECT_THAT(run_times_, testing::ElementsAre(start_time + delay100));
};

TEST_F(DeadlineTaskRunnerTest, DeleteDeadlineTaskRunnerAfterPosting) {
  deadline_task_runner_->SetDeadline(
      FROM_HERE, base::TimeDelta::FromMilliseconds(10), clock_->NowTicks());

  // Deleting the pending task should cancel it.
  deadline_task_runner_.reset(nullptr);
  RunUntilIdle();

  EXPECT_TRUE(run_times_.empty());
};

}  // namespace scheduler
}  // namespace blink
