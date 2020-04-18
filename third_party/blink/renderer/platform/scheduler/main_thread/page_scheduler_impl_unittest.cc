// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/main_thread/page_scheduler_impl.h"

#include <memory>

#include "base/callback.h"
#include "base/location.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/child/task_queue_with_task_type.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/frame_scheduler_impl.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/main_thread_scheduler_impl.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"

using base::sequence_manager::TaskQueue;
using testing::ElementsAre;
using VirtualTimePolicy = blink::PageScheduler::VirtualTimePolicy;

namespace blink {
namespace scheduler {
// To avoid symbol collisions in jumbo builds.
namespace page_scheduler_impl_unittest {

class PageSchedulerImplTest : public testing::Test {
 public:
  PageSchedulerImplTest() = default;
  ~PageSchedulerImplTest() override = default;

  void SetUp() override {
    clock_.Advance(base::TimeDelta::FromMicroseconds(5000));
    mock_task_runner_ =
        base::MakeRefCounted<cc::OrderedSimpleTaskRunner>(&clock_, true);
    scheduler_.reset(new MainThreadSchedulerImpl(
        base::sequence_manager::TaskQueueManagerForTest::Create(
            nullptr, mock_task_runner_, &clock_),
        base::nullopt));
    page_scheduler_.reset(new PageSchedulerImpl(
        nullptr, scheduler_.get(), DisableBackgroundTimerThrottling()));
    frame_scheduler_ = page_scheduler_->CreateFrameSchedulerImpl(
        nullptr, FrameScheduler::FrameType::kSubframe);
  }

  void TearDown() override {
    frame_scheduler_.reset();
    page_scheduler_.reset();
    scheduler_->Shutdown();
    scheduler_.reset();
  }

  virtual bool DisableBackgroundTimerThrottling() const { return false; }

 protected:
  static scoped_refptr<TaskQueue> ThrottleableTaskQueueForScheduler(
      FrameSchedulerImpl* scheduler) {
    return scheduler->ThrottleableTaskQueue();
  }

  scoped_refptr<base::SingleThreadTaskRunner> ThrottleableTaskRunner() {
    return TaskQueueWithTaskType::Create(ThrottleableTaskQueue(),
                                         TaskType::kInternalTest);
  }

  scoped_refptr<base::SingleThreadTaskRunner> LoadingTaskRunner() {
    return TaskQueueWithTaskType::Create(LoadingTaskQueue(),
                                         TaskType::kInternalTest);
  }

  scoped_refptr<TaskQueue> ThrottleableTaskQueue() {
    return frame_scheduler_->ThrottleableTaskQueue();
  }

  scoped_refptr<TaskQueue> LoadingTaskQueue() {
    return frame_scheduler_->LoadingTaskQueue();
  }

  scoped_refptr<TaskQueue> DeferrableTaskQueue() {
    return frame_scheduler_->DeferrableTaskQueue();
  }

  scoped_refptr<TaskQueue> PausableTaskQueue() {
    return frame_scheduler_->PausableTaskQueue();
  }

  scoped_refptr<TaskQueue> UnpausableTaskQueue() {
    return frame_scheduler_->UnpausableTaskQueue();
  }

  base::SimpleTestTickClock clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  std::unique_ptr<MainThreadSchedulerImpl> scheduler_;
  std::unique_ptr<PageSchedulerImpl> page_scheduler_;
  std::unique_ptr<FrameSchedulerImpl> frame_scheduler_;
};

TEST_F(PageSchedulerImplTest, TestDestructionOfFrameSchedulersBefore) {
  std::unique_ptr<blink::FrameScheduler> frame1(
      page_scheduler_->CreateFrameScheduler(
          nullptr, FrameScheduler::FrameType::kSubframe));
  std::unique_ptr<blink::FrameScheduler> frame2(
      page_scheduler_->CreateFrameScheduler(
          nullptr, FrameScheduler::FrameType::kSubframe));
}

TEST_F(PageSchedulerImplTest, TestDestructionOfFrameSchedulersAfter) {
  std::unique_ptr<blink::FrameScheduler> frame1(
      page_scheduler_->CreateFrameScheduler(
          nullptr, FrameScheduler::FrameType::kSubframe));
  std::unique_ptr<blink::FrameScheduler> frame2(
      page_scheduler_->CreateFrameScheduler(
          nullptr, FrameScheduler::FrameType::kSubframe));
  page_scheduler_.reset();
}

namespace {

void RunRepeatingTask(scoped_refptr<TaskQueue>,
                      int* run_count,
                      base::TimeDelta delay);

base::OnceClosure MakeRepeatingTask(scoped_refptr<TaskQueue> task_queue,
                                    int* run_count,
                                    base::TimeDelta delay) {
  return base::BindOnce(&RunRepeatingTask, std::move(task_queue),
                        base::Unretained(run_count), delay);
}

void RunRepeatingTask(scoped_refptr<TaskQueue> task_queue,
                      int* run_count,
                      base::TimeDelta delay) {
  ++*run_count;
  TaskQueue* task_queue_ptr = task_queue.get();
  task_queue_ptr->PostDelayedTask(
      FROM_HERE, MakeRepeatingTask(std::move(task_queue_ptr), run_count, delay),
      delay);
}

}  // namespace

TEST_F(PageSchedulerImplTest, RepeatingTimer_PageInForeground) {
  page_scheduler_->SetPageVisible(true);

  int run_count = 0;
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(ThrottleableTaskQueue(), &run_count,
                        base::TimeDelta::FromMilliseconds(1)),
      base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);
}

TEST_F(PageSchedulerImplTest, RepeatingTimer_PageInBackgroundThenForeground) {
  page_scheduler_->SetPageVisible(false);

  int run_count = 0;
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(ThrottleableTaskQueue(), &run_count,
                        base::TimeDelta::FromMilliseconds(20)),
      base::TimeDelta::FromMilliseconds(20));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1, run_count);

  // Make sure there's no delay in throttling being removed for pages that have
  // become visible.
  page_scheduler_->SetPageVisible(true);

  run_count = 0;
  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(50, run_count);
}

TEST_F(PageSchedulerImplTest, RepeatingLoadingTask_PageInBackground) {
  page_scheduler_->SetPageVisible(false);

  int run_count = 0;
  LoadingTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(LoadingTaskQueue(), &run_count,
                        base::TimeDelta::FromMilliseconds(1)),
      base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);  // Loading tasks should not be throttled
}

TEST_F(PageSchedulerImplTest, RepeatingTimers_OneBackgroundOneForeground) {
  std::unique_ptr<PageSchedulerImpl> page_scheduler2(
      new PageSchedulerImpl(nullptr, scheduler_.get(), false));
  std::unique_ptr<FrameSchedulerImpl> frame_scheduler2 =
      page_scheduler2->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  page_scheduler_->SetPageVisible(true);
  page_scheduler2->SetPageVisible(false);

  int run_count1 = 0;
  int run_count2 = 0;
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(ThrottleableTaskQueue(), &run_count1,
                        base::TimeDelta::FromMilliseconds(20)),
      base::TimeDelta::FromMilliseconds(20));
  ThrottleableTaskQueueForScheduler(frame_scheduler2.get())
      ->PostDelayedTask(
          FROM_HERE,
          MakeRepeatingTask(
              ThrottleableTaskQueueForScheduler(frame_scheduler2.get()),
              &run_count2, base::TimeDelta::FromMilliseconds(20)),
          base::TimeDelta::FromMilliseconds(20));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(50, run_count1);
  EXPECT_EQ(1, run_count2);
}

namespace {

void RunVirtualTimeRecorderTask(
    base::SimpleTestTickClock* clock,
    MainThreadSchedulerImpl* scheduler,
    std::vector<base::TimeTicks>* out_real_times,
    std::vector<base::TimeTicks>* out_virtual_times) {
  out_real_times->push_back(clock->NowTicks());
  out_virtual_times->push_back(scheduler->GetVirtualTimeDomain()->Now());
}

base::OnceClosure MakeVirtualTimeRecorderTask(
    base::SimpleTestTickClock* clock,
    MainThreadSchedulerImpl* scheduler,
    std::vector<base::TimeTicks>* out_real_times,
    std::vector<base::TimeTicks>* out_virtual_times) {
  return WTF::Bind(&RunVirtualTimeRecorderTask, WTF::Unretained(clock),
                   WTF::Unretained(scheduler), WTF::Unretained(out_real_times),
                   WTF::Unretained(out_virtual_times));
}
}  // namespace

TEST_F(PageSchedulerImplTest, VirtualTime_TimerFastForwarding) {
  std::vector<base::TimeTicks> real_times;
  std::vector<base::TimeTicks> virtual_times;

  page_scheduler_->EnableVirtualTime();

  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();
  base::TimeTicks initial_virtual_time =
      scheduler_->GetVirtualTimeDomain()->Now();

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(2));

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(20));

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(real_times, ElementsAre(initial_real_time, initial_real_time,
                                      initial_real_time));
  EXPECT_THAT(
      virtual_times,
      ElementsAre(
          initial_virtual_time + base::TimeDelta::FromMilliseconds(2),
          initial_virtual_time + base::TimeDelta::FromMilliseconds(20),
          initial_virtual_time + base::TimeDelta::FromMilliseconds(200)));
}

TEST_F(PageSchedulerImplTest, VirtualTime_LoadingTaskFastForwarding) {
  std::vector<base::TimeTicks> real_times;
  std::vector<base::TimeTicks> virtual_times;

  page_scheduler_->EnableVirtualTime();

  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();
  base::TimeTicks initial_virtual_time =
      scheduler_->GetVirtualTimeDomain()->Now();

  LoadingTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(2));

  LoadingTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(20));

  LoadingTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(200));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(real_times, ElementsAre(initial_real_time, initial_real_time,
                                      initial_real_time));
  EXPECT_THAT(
      virtual_times,
      ElementsAre(
          initial_virtual_time + base::TimeDelta::FromMilliseconds(2),
          initial_virtual_time + base::TimeDelta::FromMilliseconds(20),
          initial_virtual_time + base::TimeDelta::FromMilliseconds(200)));
}

TEST_F(PageSchedulerImplTest,
       RepeatingTimer_PageInBackground_MeansNothingForVirtualTime) {
  page_scheduler_->EnableVirtualTime();
  page_scheduler_->SetPageVisible(false);
  scheduler_->GetSchedulerHelperForTesting()->SetWorkBatchSizeForTesting(1);
  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();

  int run_count = 0;
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(ThrottleableTaskQueue(), &run_count,
                        base::TimeDelta::FromMilliseconds(1)),
      base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunTasksWhile(mock_task_runner_->TaskRunCountBelow(2000));
  // Virtual time means page visibility is ignored.
  EXPECT_EQ(1999, run_count);

  // The global tick clock has not moved, yet we ran a large number of "delayed"
  // tasks despite calling setPageVisible(false).
  EXPECT_EQ(initial_real_time, scheduler_->tick_clock()->NowTicks());
}

namespace {

void RunOrderTask(int index, std::vector<int>* out_run_order) {
  out_run_order->push_back(index);
}

void DelayedRunOrderTask(int index,
                         scoped_refptr<TaskQueue> task_queue,
                         std::vector<int>* out_run_order) {
  out_run_order->push_back(index);
  task_queue->PostTask(FROM_HERE,
                       base::BindOnce(&RunOrderTask, index + 1,
                                      base::Unretained(out_run_order)));
}
}  // namespace

TEST_F(PageSchedulerImplTest, VirtualTime_NotAllowedToAdvance) {
  std::vector<int> run_order;

  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
  page_scheduler_->EnableVirtualTime();

  ThrottleableTaskQueue()->PostTask(
      FROM_HERE,
      base::BindOnce(&RunOrderTask, 0, base::Unretained(&run_order)));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DelayedRunOrderTask, 1, ThrottleableTaskQueue(),
                     base::Unretained(&run_order)),
      base::TimeDelta::FromMilliseconds(2));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DelayedRunOrderTask, 3, ThrottleableTaskQueue(),
                     base::Unretained(&run_order)),
      base::TimeDelta::FromMilliseconds(4));

  mock_task_runner_->RunUntilIdle();

  // No timer tasks are allowed to run.
  EXPECT_THAT(run_order, ElementsAre());
}

TEST_F(PageSchedulerImplTest, VirtualTime_AllowedToAdvance) {
  std::vector<int> run_order;

  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);
  page_scheduler_->EnableVirtualTime();

  ThrottleableTaskQueue()->PostTask(
      FROM_HERE,
      base::BindOnce(&RunOrderTask, 0, base::Unretained(&run_order)));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DelayedRunOrderTask, 1, ThrottleableTaskQueue(),
                     base::Unretained(&run_order)),
      base::TimeDelta::FromMilliseconds(2));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&DelayedRunOrderTask, 3, ThrottleableTaskQueue(),
                     base::Unretained(&run_order)),
      base::TimeDelta::FromMilliseconds(4));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_order, ElementsAre(0, 1, 2, 3, 4));
}

class PageSchedulerImplTestWithDisabledBackgroundTimerThrottling
    : public PageSchedulerImplTest {
 public:
  PageSchedulerImplTestWithDisabledBackgroundTimerThrottling() = default;
  ~PageSchedulerImplTestWithDisabledBackgroundTimerThrottling() override =
      default;

  bool DisableBackgroundTimerThrottling() const override { return true; }
};

TEST_F(PageSchedulerImplTestWithDisabledBackgroundTimerThrottling,
       RepeatingTimer_PageInBackground) {
  page_scheduler_->SetPageVisible(false);

  int run_count = 0;
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(ThrottleableTaskQueue(), &run_count,
                        base::TimeDelta::FromMilliseconds(1)),
      base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);
}

TEST_F(PageSchedulerImplTest, VirtualTimeSettings_NewFrameScheduler) {
  std::vector<int> run_order;

  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
  page_scheduler_->EnableVirtualTime();

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler_->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  ThrottleableTaskQueueForScheduler(frame_scheduler.get())
      ->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&RunOrderTask, 1, base::Unretained(&run_order)),
          base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunUntilIdle();
  EXPECT_TRUE(run_order.empty());

  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);
  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_order, ElementsAre(1));
}

namespace {

template <typename T>
base::OnceClosure MakeDeletionTask(T* obj) {
  return base::BindOnce([](T* obj) { delete obj; }, base::Unretained(obj));
}

}  // namespace

TEST_F(PageSchedulerImplTest, DeleteFrameSchedulers_InTask) {
  for (int i = 0; i < 10; i++) {
    FrameSchedulerImpl* frame_scheduler =
        page_scheduler_
            ->CreateFrameSchedulerImpl(nullptr,
                                       FrameScheduler::FrameType::kSubframe)
            .release();
    ThrottleableTaskQueueForScheduler(frame_scheduler)
        ->PostDelayedTask(FROM_HERE, MakeDeletionTask(frame_scheduler),
                          base::TimeDelta::FromMilliseconds(1));
  }
  mock_task_runner_->RunUntilIdle();
}

TEST_F(PageSchedulerImplTest, DeletePageScheduler_InTask) {
  ThrottleableTaskQueue()->PostTask(
      FROM_HERE, MakeDeletionTask(page_scheduler_.release()));
  mock_task_runner_->RunUntilIdle();
}

TEST_F(PageSchedulerImplTest, DeleteThrottledQueue_InTask) {
  page_scheduler_->SetPageVisible(false);

  FrameSchedulerImpl* frame_scheduler =
      page_scheduler_
          ->CreateFrameSchedulerImpl(nullptr,
                                     FrameScheduler::FrameType::kSubframe)
          .release();
  scoped_refptr<TaskQueue> timer_task_queue =
      ThrottleableTaskQueueForScheduler(frame_scheduler);

  int run_count = 0;
  timer_task_queue->PostDelayedTask(
      FROM_HERE,
      MakeRepeatingTask(timer_task_queue, &run_count,
                        base::TimeDelta::FromMilliseconds(100)),
      base::TimeDelta::FromMilliseconds(100));

  // Note this will run at time t = 10s since we start at time t = 5000us.
  // However, we still should run all tasks after frame scheduler deletion.
  timer_task_queue->PostDelayedTask(FROM_HERE,
                                    MakeDeletionTask(frame_scheduler),
                                    base::TimeDelta::FromMilliseconds(9990));

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(20));
  EXPECT_EQ(110, run_count);
}

TEST_F(PageSchedulerImplTest, VirtualTimePauseCount_DETERMINISTIC_LOADING) {
  page_scheduler_->SetVirtualTimePolicy(
      VirtualTimePolicy::kDeterministicLoading);
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->IncrementVirtualTimePauseCount();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->IncrementVirtualTimePauseCount();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->DecrementVirtualTimePauseCount();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->DecrementVirtualTimePauseCount();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->IncrementVirtualTimePauseCount();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->DecrementVirtualTimePauseCount();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
}

TEST_F(PageSchedulerImplTest,
       WebScopedVirtualTimePauser_DETERMINISTIC_LOADING) {
  page_scheduler_->SetVirtualTimePolicy(
      VirtualTimePolicy::kDeterministicLoading);

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler_->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  {
    WebScopedVirtualTimePauser virtual_time_pauser =
        frame_scheduler->CreateWebScopedVirtualTimePauser(
            "test",
            WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);
    EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

    virtual_time_pauser.PauseVirtualTime();
    EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

    virtual_time_pauser.UnpauseVirtualTime();
    EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

    virtual_time_pauser.PauseVirtualTime();
    EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());
  }

  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
}

namespace {

void RecordVirtualTime(MainThreadSchedulerImpl* scheduler,
                       base::TimeTicks* out) {
  *out = scheduler->GetVirtualTimeDomain()->Now();
}

void PauseAndUnpauseVirtualTime(MainThreadSchedulerImpl* scheduler,
                                FrameSchedulerImpl* frame_scheduler,
                                base::TimeTicks* paused,
                                base::TimeTicks* unpaused) {
  *paused = scheduler->GetVirtualTimeDomain()->Now();

  {
    WebScopedVirtualTimePauser virtual_time_pauser =
        frame_scheduler->CreateWebScopedVirtualTimePauser(
            "test",
            WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);
    virtual_time_pauser.PauseVirtualTime();
  }

  *unpaused = scheduler->GetVirtualTimeDomain()->Now();
}

}  // namespace

TEST_F(PageSchedulerImplTest,
       WebScopedVirtualTimePauserWithInterleavedTasks_DETERMINISTIC_LOADING) {
  // Make task queue manager ask the virtual time domain for the next task delay
  // after each task.
  scheduler_->GetSchedulerHelperForTesting()->SetWorkBatchSizeForTesting(1);

  page_scheduler_->EnableVirtualTime();
  page_scheduler_->SetVirtualTimePolicy(
      VirtualTimePolicy::kDeterministicLoading);

  base::TimeTicks initial_virtual_time =
      scheduler_->GetVirtualTimeDomain()->Now();

  base::TimeTicks time_paused;
  base::TimeTicks time_unpaused;
  base::TimeTicks time_second_task;

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler_->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  // Pauses and unpauses virtual time, thereby advancing virtual time by an
  // additional 10ms due to WebScopedVirtualTimePauser's delay.
  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      WTF::Bind(&PauseAndUnpauseVirtualTime, WTF::Unretained(scheduler_.get()),
                WTF::Unretained(frame_scheduler.get()),
                WTF::Unretained(&time_paused), WTF::Unretained(&time_unpaused)),
      base::TimeDelta::FromMilliseconds(3));

  // Will run after the first task has advanced virtual time past 5ms.
  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      WTF::Bind(&RecordVirtualTime, WTF::Unretained(scheduler_.get()),
                WTF::Unretained(&time_second_task)),
      base::TimeDelta::FromMilliseconds(5));

  mock_task_runner_->RunUntilIdle();

  EXPECT_EQ(time_paused,
            initial_virtual_time + base::TimeDelta::FromMilliseconds(3));
  EXPECT_EQ(time_unpaused,
            initial_virtual_time + base::TimeDelta::FromMilliseconds(13));
  EXPECT_EQ(time_second_task,
            initial_virtual_time + base::TimeDelta::FromMilliseconds(13));
}

TEST_F(PageSchedulerImplTest,
       MultipleWebScopedVirtualTimePausers_DETERMINISTIC_LOADING) {
  page_scheduler_->SetVirtualTimePolicy(
      VirtualTimePolicy::kDeterministicLoading);

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler_->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  WebScopedVirtualTimePauser virtual_time_pauser1 =
      frame_scheduler->CreateWebScopedVirtualTimePauser(
          "test", WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);
  WebScopedVirtualTimePauser virtual_time_pauser2 =
      frame_scheduler->CreateWebScopedVirtualTimePauser(
          "test", WebScopedVirtualTimePauser::VirtualTaskDuration::kNonInstant);

  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

  virtual_time_pauser1.PauseVirtualTime();
  virtual_time_pauser2.PauseVirtualTime();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  virtual_time_pauser2.UnpauseVirtualTime();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  virtual_time_pauser1.UnpauseVirtualTime();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
}

TEST_F(PageSchedulerImplTest, NestedMessageLoop_DETERMINISTIC_LOADING) {
  page_scheduler_->SetVirtualTimePolicy(
      VirtualTimePolicy::kDeterministicLoading);
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->OnBeginNestedRunLoop();
  EXPECT_FALSE(scheduler_->VirtualTimeAllowedToAdvance());

  scheduler_->OnExitNestedRunLoop();
  EXPECT_TRUE(scheduler_->VirtualTimeAllowedToAdvance());
}

TEST_F(PageSchedulerImplTest, PauseTimersWhileVirtualTimeIsPaused) {
  std::vector<int> run_order;

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler =
      page_scheduler_->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);
  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
  page_scheduler_->EnableVirtualTime();

  ThrottleableTaskQueueForScheduler(frame_scheduler.get())
      ->PostTask(FROM_HERE, base::BindOnce(&RunOrderTask, 1,
                                           base::Unretained(&run_order)));

  mock_task_runner_->RunUntilIdle();
  EXPECT_TRUE(run_order.empty());

  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);
  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_order, ElementsAre(1));
}

TEST_F(PageSchedulerImplTest, VirtualTimeBudgetExhaustedCallback) {
  std::vector<base::TimeTicks> real_times;
  std::vector<base::TimeTicks> virtual_times;

  page_scheduler_->EnableVirtualTime();

  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();
  base::TimeTicks initial_virtual_time =
      scheduler_->GetVirtualTimeDomain()->Now();

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(1));

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(2));

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(5));

  ThrottleableTaskRunner()->PostDelayedTask(
      FROM_HERE,
      MakeVirtualTimeRecorderTask(&clock_, scheduler_.get(), &real_times,
                                  &virtual_times),
      base::TimeDelta::FromMilliseconds(7));

  page_scheduler_->GrantVirtualTimeBudget(
      base::TimeDelta::FromMilliseconds(5),
      WTF::Bind(
          [](PageScheduler* scheduler) {
            scheduler->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
          },
          WTF::Unretained(page_scheduler_.get())));

  mock_task_runner_->RunUntilIdle();

  // The timer that is scheduled for the exact point in time when virtual time
  // expires will not run.
  EXPECT_THAT(real_times, ElementsAre(initial_real_time, initial_real_time,
                                      initial_real_time));
  EXPECT_THAT(
      virtual_times,
      ElementsAre(initial_virtual_time + base::TimeDelta::FromMilliseconds(1),
                  initial_virtual_time + base::TimeDelta::FromMilliseconds(2),
                  initial_virtual_time + base::TimeDelta::FromMilliseconds(5)));
}

namespace {
class MockObserver : public PageScheduler::VirtualTimeObserver {
 public:
  ~MockObserver() override = default;

  void OnVirtualTimeAdvanced(base::TimeDelta virtual_time_offset) override {
    virtual_time_log_.push_back(base::StringPrintf(
        "Advanced to %dms",
        static_cast<int>(virtual_time_offset.InMilliseconds())));
  }

  void OnVirtualTimePaused(base::TimeDelta virtual_time_offset) override {
    virtual_time_log_.push_back(base::StringPrintf(
        "Paused at %dms",
        static_cast<int>(virtual_time_offset.InMilliseconds())));
  }

  const std::vector<std::string>& virtual_time_log() const {
    return virtual_time_log_;
  }

 private:
  std::vector<std::string> virtual_time_log_;
};

void NopTask() {}
}  // namespace

TEST_F(PageSchedulerImplTest, VirtualTimeObserver) {
  MockObserver mock_observer;
  page_scheduler_->AddVirtualTimeObserver(&mock_observer);
  page_scheduler_->EnableVirtualTime();

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&NopTask),
      base::TimeDelta::FromMilliseconds(200));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&NopTask),
      base::TimeDelta::FromMilliseconds(20));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&NopTask),
      base::TimeDelta::FromMilliseconds(2));

  page_scheduler_->GrantVirtualTimeBudget(
      base::TimeDelta::FromMilliseconds(1000),
      WTF::Bind(
          [](PageScheduler* scheduler) {
            scheduler->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
          },
          WTF::Unretained(page_scheduler_.get())));

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(
      mock_observer.virtual_time_log(),
      ElementsAre("Advanced to 2ms", "Advanced to 20ms", "Advanced to 200ms",
                  "Advanced to 1000ms", "Paused at 1000ms"));
  page_scheduler_->RemoveVirtualTimeObserver(&mock_observer);
}

namespace {
void RepostingTask(scoped_refptr<TaskQueue> task_queue,
                   int max_count,
                   int* count) {
  if (++(*count) >= max_count)
    return;

  task_queue->PostTask(FROM_HERE,
                       base::BindOnce(&RepostingTask, task_queue, max_count,
                                      base::Unretained(count)));
}

void DelayedTask(int* count_in, int* count_out) {
  *count_out = *count_in;
}

}  // namespace

TEST_F(PageSchedulerImplTest, MaxVirtualTimeTaskStarvationCountOneHundred) {
  page_scheduler_->EnableVirtualTime();
  page_scheduler_->SetMaxVirtualTimeTaskStarvationCount(100);
  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);

  int count = 0;
  int delayed_task_run_at_count = 0;
  RepostingTask(ThrottleableTaskQueue(), 1000, &count);
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(DelayedTask, base::Unretained(&count),
                     base::Unretained(&delayed_task_run_at_count)),
      base::TimeDelta::FromMilliseconds(10));

  page_scheduler_->GrantVirtualTimeBudget(
      base::TimeDelta::FromMilliseconds(1000),
      WTF::Bind(
          [](PageScheduler* scheduler) {
            scheduler->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
          },
          WTF::Unretained(page_scheduler_.get())));

  mock_task_runner_->RunUntilIdle();

  // Two delayed tasks with a run of 100 tasks, plus initial call.
  EXPECT_EQ(201, count);
  EXPECT_EQ(102, delayed_task_run_at_count);
}

TEST_F(PageSchedulerImplTest,
       MaxVirtualTimeTaskStarvationCountOneHundredNestedMessageLoop) {
  page_scheduler_->EnableVirtualTime();
  page_scheduler_->SetMaxVirtualTimeTaskStarvationCount(100);
  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);
  scheduler_->OnBeginNestedRunLoop();

  int count = 0;
  int delayed_task_run_at_count = 0;
  RepostingTask(ThrottleableTaskQueue(), 1000, &count);
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(DelayedTask, WTF::Unretained(&count),
                     WTF::Unretained(&delayed_task_run_at_count)),
      base::TimeDelta::FromMilliseconds(10));

  page_scheduler_->GrantVirtualTimeBudget(
      base::TimeDelta::FromMilliseconds(1000),
      WTF::Bind(
          [](PageScheduler* scheduler) {
            scheduler->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
          },
          WTF::Unretained(page_scheduler_.get())));

  mock_task_runner_->RunUntilIdle();

  EXPECT_EQ(1000, count);
  EXPECT_EQ(1000, delayed_task_run_at_count);
}

TEST_F(PageSchedulerImplTest, MaxVirtualTimeTaskStarvationCountZero) {
  page_scheduler_->EnableVirtualTime();
  page_scheduler_->SetMaxVirtualTimeTaskStarvationCount(0);
  page_scheduler_->SetVirtualTimePolicy(VirtualTimePolicy::kAdvance);

  int count = 0;
  int delayed_task_run_at_count = 0;
  RepostingTask(ThrottleableTaskQueue(), 1000, &count);
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(DelayedTask, WTF::Unretained(&count),
                     WTF::Unretained(&delayed_task_run_at_count)),
      base::TimeDelta::FromMilliseconds(10));

  page_scheduler_->GrantVirtualTimeBudget(
      base::TimeDelta::FromMilliseconds(1000),
      WTF::Bind(
          [](PageScheduler* scheduler) {
            scheduler->SetVirtualTimePolicy(VirtualTimePolicy::kPause);
          },
          WTF::Unretained(page_scheduler_.get())));

  mock_task_runner_->RunUntilIdle();

  EXPECT_EQ(1000, count);
  // If the initial count had been higher, the delayed task could have been
  // arbitrarily delayed.
  EXPECT_EQ(1000, delayed_task_run_at_count);
}

namespace {

void ExpensiveTestTask(base::SimpleTestTickClock* clock,
                       std::vector<base::TimeTicks>* run_times) {
  run_times->push_back(clock->NowTicks());
  clock->Advance(base::TimeDelta::FromMilliseconds(250));
}

void InitializeTrialParams() {
  std::map<std::string, std::string> params = {{"cpu_budget", "0.01"},
                                               {"max_budget", "0.0"},
                                               {"initial_budget", "0.0"},
                                               {"max_delay", "0.0"}};
  const char kParamName[] = "ExpensiveBackgroundTimerThrottling";
  const char kGroupName[] = "Enabled";
  EXPECT_TRUE(base::AssociateFieldTrialParams(kParamName, kGroupName, params));
  EXPECT_TRUE(base::FieldTrialList::CreateFieldTrial(kParamName, kGroupName));

  std::map<std::string, std::string> actual_params;
  base::GetFieldTrialParams(kParamName, &actual_params);
  EXPECT_EQ(actual_params, params);
}

}  // namespace

TEST_F(PageSchedulerImplTest, BackgroundTimerThrottling) {
  ScopedExpensiveBackgroundTimerThrottlingForTest
      budget_background_throttling_enabler(true);

  std::unique_ptr<base::FieldTrialList> field_trial_list =
      std::make_unique<base::FieldTrialList>(nullptr);
  InitializeTrialParams();
  page_scheduler_.reset(
      new PageSchedulerImpl(nullptr, scheduler_.get(), false));
  EXPECT_FALSE(page_scheduler_->IsThrottled());

  std::vector<base::TimeTicks> run_times;
  frame_scheduler_ = page_scheduler_->CreateFrameSchedulerImpl(
      nullptr, FrameScheduler::FrameType::kSubframe);
  page_scheduler_->SetPageVisible(true);
  EXPECT_FALSE(page_scheduler_->IsThrottled());

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(2500));

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
      base::TimeDelta::FromMilliseconds(1));
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
      base::TimeDelta::FromMilliseconds(1));

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(3500));

  // Check that these tasks are aligned, but are not subject to budget-based
  // throttling.
  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromMilliseconds(2501),
                  base::TimeTicks() + base::TimeDelta::FromMilliseconds(2751)));
  run_times.clear();

  page_scheduler_->SetPageVisible(false);
  EXPECT_FALSE(page_scheduler_->IsThrottled());

  // Ensure that the page is fully throttled.
  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromSeconds(15));
  EXPECT_TRUE(page_scheduler_->IsThrottled());

  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
      base::TimeDelta::FromMicroseconds(1));
  ThrottleableTaskQueue()->PostDelayedTask(
      FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
      base::TimeDelta::FromMicroseconds(1));

  mock_task_runner_->RunUntilIdle();

  // Check that tasks are aligned and throttled.
  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(16),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(26)));

  base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
}

TEST_F(PageSchedulerImplTest, OpenWebSocketExemptsFromBudgetThrottling) {
  ScopedExpensiveBackgroundTimerThrottlingForTest
      budget_background_throttling_enabler(true);

  std::unique_ptr<base::FieldTrialList> field_trial_list =
      std::make_unique<base::FieldTrialList>(nullptr);
  InitializeTrialParams();
  std::unique_ptr<PageSchedulerImpl> page_scheduler(
      new PageSchedulerImpl(nullptr, scheduler_.get(), false));

  std::vector<base::TimeTicks> run_times;

  std::unique_ptr<FrameSchedulerImpl> frame_scheduler1 =
      page_scheduler->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);
  std::unique_ptr<FrameSchedulerImpl> frame_scheduler2 =
      page_scheduler->CreateFrameSchedulerImpl(
          nullptr, FrameScheduler::FrameType::kSubframe);

  page_scheduler->SetPageVisible(false);

  // Wait for 20s to avoid initial throttling delay.
  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(20500));

  for (size_t i = 0; i < 3; ++i) {
    ThrottleableTaskQueueForScheduler(frame_scheduler1.get())
        ->PostDelayedTask(
            FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
            base::TimeDelta::FromMilliseconds(1));
  }

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(55500));

  // Check that tasks are throttled.
  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(21),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(26),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(51)));
  run_times.clear();

  std::unique_ptr<FrameScheduler::ActiveConnectionHandle> websocket_connection =
      frame_scheduler1->OnActiveConnectionCreated();

  for (size_t i = 0; i < 3; ++i) {
    ThrottleableTaskQueueForScheduler(frame_scheduler1.get())
        ->PostDelayedTask(
            FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
            base::TimeDelta::FromMilliseconds(1));
  }

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(58500));

  // Check that the timer task queue from the first frame is aligned,
  // but not throttled.
  EXPECT_THAT(
      run_times,
      ElementsAre(
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(56000),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(56250),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(56500)));
  run_times.clear();

  for (size_t i = 0; i < 3; ++i) {
    ThrottleableTaskQueueForScheduler(frame_scheduler2.get())
        ->PostDelayedTask(
            FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
            base::TimeDelta::FromMilliseconds(1));
  }

  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(59500));

  // Check that the second frame scheduler becomes unthrottled.
  EXPECT_THAT(
      run_times,
      ElementsAre(
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(59000),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(59250),
          base::TimeTicks() + base::TimeDelta::FromMilliseconds(59500)));
  run_times.clear();

  websocket_connection.reset();

  // Wait for 10s to enable throttling back.
  mock_task_runner_->RunUntilTime(base::TimeTicks() +
                                  base::TimeDelta::FromMilliseconds(70500));

  for (size_t i = 0; i < 3; ++i) {
    ThrottleableTaskQueueForScheduler(frame_scheduler1.get())
        ->PostDelayedTask(
            FROM_HERE, base::BindOnce(&ExpensiveTestTask, &clock_, &run_times),
            base::TimeDelta::FromMilliseconds(1));
  }

  mock_task_runner_->RunUntilIdle();

  // WebSocket is closed, budget-based throttling now applies.
  EXPECT_THAT(
      run_times,
      ElementsAre(base::TimeTicks() + base::TimeDelta::FromSeconds(84),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(109),
                  base::TimeTicks() + base::TimeDelta::FromSeconds(134)));

  base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
}

namespace {
void IncrementCounter(int* counter) {
  ++*counter;
}
}  // namespace

TEST_F(PageSchedulerImplTest, PageFreeze) {
  ScopedStopLoadingInBackgroundForTest stop_loading_enabler(true);
  ScopedStopNonTimersInBackgroundForTest stop_non_timers_enabler(true);

  int counter = 0;
  LoadingTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  ThrottleableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  DeferrableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  PausableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  UnpausableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));

  page_scheduler_->SetPageVisible(false);
  EXPECT_EQ(false, page_scheduler_->IsFrozen());

  // In a backgrounded active page, all queues should run.
  mock_task_runner_->RunUntilIdle();
  EXPECT_EQ(5, counter);

  LoadingTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  ThrottleableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  DeferrableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  PausableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  UnpausableTaskQueue()->PostTask(
      FROM_HERE, base::BindOnce(&IncrementCounter, base::Unretained(&counter)));
  counter = 0;

  page_scheduler_->SetPageFrozen(true);
  EXPECT_EQ(true, page_scheduler_->IsFrozen());

  // In a backgrounded frozen page, only Unpausable queue should run.
  mock_task_runner_->RunUntilIdle();
  EXPECT_EQ(1, counter);

  // A visible page should not be frozen.
  page_scheduler_->SetPageVisible(true);
  EXPECT_EQ(false, page_scheduler_->IsFrozen());

  // Once the page is unfrozen, the rest of the queues should run.
  mock_task_runner_->RunUntilIdle();
  EXPECT_EQ(5, counter);
}

TEST_F(PageSchedulerImplTest, AudioState) {
  page_scheduler_->AudioStateChanged(true);
  EXPECT_TRUE(page_scheduler_->IsAudioPlaying());

  page_scheduler_->AudioStateChanged(false);
  // We are audible for a certain period after raw signal disappearing.
  EXPECT_TRUE(page_scheduler_->IsAudioPlaying());

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(3));

  page_scheduler_->AudioStateChanged(false);
  // We are still audible. A new call to AudioStateChanged shouldn't change
  // anything.
  EXPECT_TRUE(page_scheduler_->IsAudioPlaying());

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(3));

  // Audio is finally silent.
  EXPECT_FALSE(page_scheduler_->IsAudioPlaying());
}

TEST_F(PageSchedulerImplTest, PageSchedulerDestroyedWhileAudioChangePending) {
  page_scheduler_->AudioStateChanged(true);
  EXPECT_TRUE(page_scheduler_->IsAudioPlaying());
  page_scheduler_->AudioStateChanged(false);

  page_scheduler_.reset();

  mock_task_runner_->RunUntilIdle();
}

TEST_F(PageSchedulerImplTest, AudiblePagesAreNotThrottled) {
  page_scheduler_->SetPageVisible(false);
  EXPECT_TRUE(scheduler_->task_queue_throttler()->IsThrottled(
      ThrottleableTaskQueue().get()));

  // No throttling when the page is audible.
  page_scheduler_->AudioStateChanged(true);
  EXPECT_FALSE(scheduler_->task_queue_throttler()->IsThrottled(
      ThrottleableTaskQueue().get()));

  // No throttling for some time after audio signal disappears.
  page_scheduler_->AudioStateChanged(false);
  EXPECT_FALSE(scheduler_->task_queue_throttler()->IsThrottled(
      ThrottleableTaskQueue().get()));

  // Eventually throttling is reenabled again.
  mock_task_runner_->RunUntilIdle();
  EXPECT_TRUE(scheduler_->task_queue_throttler()->IsThrottled(
      ThrottleableTaskQueue().get()));
}

TEST_F(PageSchedulerImplTest, BudgetBasedThrottlingForPageScheduler) {
  page_scheduler_->SetPageVisible(false);
}

TEST_F(PageSchedulerImplTest, KeepActiveSetForNewPages) {
  scheduler_->SetSchedulerKeepActive(true);

  std::unique_ptr<PageSchedulerImpl> page_scheduler2 =
      std::make_unique<PageSchedulerImpl>(nullptr, scheduler_.get(),
                                          DisableBackgroundTimerThrottling());

  EXPECT_TRUE(page_scheduler_->KeepActive());
  EXPECT_TRUE(page_scheduler2->KeepActive());
}

}  // namespace page_scheduler_impl_unittest
}  // namespace scheduler
}  // namespace blink
