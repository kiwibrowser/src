// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/base/task_queue_manager.h"

#include <stddef.h>
#include <memory>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/default_tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_impl.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_queue_selector.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/task_queue_manager_for_test.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/test_task_queue.h"
#include "third_party/blink/renderer/platform/scheduler/base/test/test_task_time_observer.h"
#include "third_party/blink/renderer/platform/scheduler/base/virtual_time_domain.h"
#include "third_party/blink/renderer/platform/scheduler/base/work_queue_sets.h"

namespace base {
namespace sequence_manager {

// To reduce noise related to the OS timer, we use a virtual time domain to
// fast forward the timers.
class PerfTestTimeDomain : public VirtualTimeDomain {
 public:
  PerfTestTimeDomain() : VirtualTimeDomain(TimeTicks::Now()) {}
  ~PerfTestTimeDomain() override = default;

  Optional<TimeDelta> DelayTillNextTask(LazyNow* lazy_now) override {
    TimeTicks run_time;
    if (!NextScheduledRunTime(&run_time))
      return Optional<TimeDelta>();

    AdvanceNowTo(run_time);
    return TimeDelta();  // Makes DoWork post an immediate continuation.
  }

  void RequestWakeUpAt(TimeTicks now, TimeTicks run_time) override {
    // De-dupe DoWorks.
    if (NumberOfScheduledWakeUps() == 1u)
      RequestDoWork();
  }

  void CancelWakeUpAt(TimeTicks run_time) override {
    // We didn't post a delayed task in RequestWakeUpAt so there's no need to do
    // anything here.
  }

  const char* GetName() const override { return "PerfTestTimeDomain"; }

  DISALLOW_COPY_AND_ASSIGN(PerfTestTimeDomain);
};

class TaskQueueManagerPerfTest : public testing::Test {
 public:
  TaskQueueManagerPerfTest()
      : num_queues_(0),
        max_tasks_in_flight_(0),
        num_tasks_in_flight_(0),
        num_tasks_to_post_(0),
        num_tasks_to_run_(0) {}

  void SetUp() override {
    if (ThreadTicks::IsSupported())
      ThreadTicks::WaitUntilInitialized();
  }

  void TearDown() override {
    queues_.clear();
    manager_->UnregisterTimeDomain(virtual_time_domain_.get());
    manager_.reset();
  }

  void Initialize(size_t num_queues) {
    num_queues_ = num_queues;
    message_loop_.reset(new MessageLoop());
    manager_ = TaskQueueManagerForTest::Create(message_loop_.get(),
                                               message_loop_->task_runner(),
                                               DefaultTickClock::GetInstance());
    manager_->AddTaskTimeObserver(&test_task_time_observer_);

    virtual_time_domain_.reset(new PerfTestTimeDomain());
    manager_->RegisterTimeDomain(virtual_time_domain_.get());

    for (size_t i = 0; i < num_queues; i++) {
      queues_.push_back(manager_->CreateTaskQueue<TestTaskQueue>(
          TaskQueue::Spec("test").SetTimeDomain(virtual_time_domain_.get())));
    }

    delayed_task_closure_ = BindRepeating(
        &TaskQueueManagerPerfTest::TestDelayedTask, Unretained(this));

    immediate_task_closure_ = BindRepeating(
        &TaskQueueManagerPerfTest::TestImmediateTask, Unretained(this));
  }

  void TestDelayedTask() {
    if (--num_tasks_to_run_ == 0) {
      run_loop_->QuitWhenIdle();
      return;
    }

    num_tasks_in_flight_--;
    // NOTE there are only up to max_tasks_in_flight_ pending delayed tasks at
    // any one time.  Thanks to the lower_num_tasks_to_post going to zero if
    // there are a lot of tasks in flight, the total number of task in flight at
    // any one time is very variable.
    unsigned int lower_num_tasks_to_post =
        num_tasks_in_flight_ < (max_tasks_in_flight_ / 2) ? 1 : 0;
    unsigned int max_tasks_to_post =
        num_tasks_to_post_ % 2 ? lower_num_tasks_to_post : 10;
    for (unsigned int i = 0;
         i < max_tasks_to_post && num_tasks_in_flight_ < max_tasks_in_flight_ &&
         num_tasks_to_post_ > 0;
         i++) {
      // Choose a queue weighted towards queue 0.
      unsigned int queue = num_tasks_to_post_ % (num_queues_ + 1);
      if (queue == num_queues_) {
        queue = 0;
      }
      // Simulate a mix of short and longer delays.
      unsigned int delay =
          num_tasks_to_post_ % 2 ? 1 : (10 + num_tasks_to_post_ % 10);
      queues_[queue]->PostDelayedTask(FROM_HERE, delayed_task_closure_,
                                      TimeDelta::FromMilliseconds(delay));
      num_tasks_in_flight_++;
      num_tasks_to_post_--;
    }
  }

  void TestImmediateTask() {
    if (--num_tasks_to_run_ == 0) {
      run_loop_->QuitWhenIdle();
      return;
    }

    num_tasks_in_flight_--;
    // NOTE there are only up to max_tasks_in_flight_ pending delayed tasks at
    // any one time.  Thanks to the lower_num_tasks_to_post going to zero if
    // there are a lot of tasks in flight, the total number of task in flight at
    // any one time is very variable.
    unsigned int lower_num_tasks_to_post =
        num_tasks_in_flight_ < (max_tasks_in_flight_ / 2) ? 1 : 0;
    unsigned int max_tasks_to_post =
        num_tasks_to_post_ % 2 ? lower_num_tasks_to_post : 10;
    for (unsigned int i = 0;
         i < max_tasks_to_post && num_tasks_in_flight_ < max_tasks_in_flight_ &&
         num_tasks_to_post_ > 0;
         i++) {
      // Choose a queue weighted towards queue 0.
      unsigned int queue = num_tasks_to_post_ % (num_queues_ + 1);
      if (queue == num_queues_) {
        queue = 0;
      }
      queues_[queue]->PostTask(FROM_HERE, immediate_task_closure_);
      num_tasks_in_flight_++;
      num_tasks_to_post_--;
    }
  }

  void ResetAndCallTestDelayedTask(unsigned int num_tasks_to_run) {
    num_tasks_in_flight_ = 1;
    num_tasks_to_post_ = num_tasks_to_run;
    num_tasks_to_run_ = num_tasks_to_run;
    TestDelayedTask();
  }

  void ResetAndCallTestImmediateTask(unsigned int num_tasks_to_run) {
    num_tasks_in_flight_ = 1;
    num_tasks_to_post_ = num_tasks_to_run;
    num_tasks_to_run_ = num_tasks_to_run;
    TestImmediateTask();
  }

  void Benchmark(const std::string& trace, const RepeatingClosure& test_task) {
    ThreadTicks start = ThreadTicks::Now();
    ThreadTicks now;
    unsigned long long num_iterations = 0;
    do {
      test_task.Run();
      run_loop_.reset(new RunLoop());
      run_loop_->Run();
      now = ThreadTicks::Now();
      num_iterations++;
    } while (now - start < TimeDelta::FromSeconds(5));
    perf_test::PrintResult(
        "task", "", trace,
        (now - start).InMicroseconds() / static_cast<double>(num_iterations),
        "us/run", true);
  }

  size_t num_queues_;
  unsigned int max_tasks_in_flight_;
  unsigned int num_tasks_in_flight_;
  unsigned int num_tasks_to_post_;
  unsigned int num_tasks_to_run_;
  std::unique_ptr<MessageLoop> message_loop_;
  std::unique_ptr<TaskQueueManager> manager_;
  std::unique_ptr<RunLoop> run_loop_;
  std::unique_ptr<VirtualTimeDomain> virtual_time_domain_;
  std::vector<scoped_refptr<SingleThreadTaskRunner>> queues_;
  RepeatingClosure delayed_task_closure_;
  RepeatingClosure immediate_task_closure_;
  // TODO(alexclarke): parameterize so we can measure with and without a
  // TaskTimeObserver.
  TestTaskTimeObserver test_task_time_observer_;
};

TEST_F(TaskQueueManagerPerfTest, RunTenThousandDelayedTasks_OneQueue) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(1u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 delayed tasks with one queue",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestDelayedTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandDelayedTasks_FourQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(4u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 delayed tasks with four queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestDelayedTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandDelayedTasks_EightQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(8u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 delayed tasks with eight queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestDelayedTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandDelayedTasks_ThirtyTwoQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(32u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 delayed tasks with eight queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestDelayedTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandImmediateTasks_OneQueue) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(1u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 immediate tasks with one queue",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestImmediateTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandImmediateTasks_FourQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(4u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 immediate tasks with four queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestImmediateTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandImmediateTasks_EightQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(8u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 immediate tasks with eight queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestImmediateTask,
                    Unretained(this), 10000));
}

TEST_F(TaskQueueManagerPerfTest, RunTenThousandImmediateTasks_ThirtyTwoQueues) {
  if (!ThreadTicks::IsSupported())
    return;
  Initialize(32u);

  max_tasks_in_flight_ = 200;
  Benchmark(
      "run 10000 immediate tasks with eight queues",
      BindRepeating(&TaskQueueManagerPerfTest::ResetAndCallTestImmediateTask,
                    Unretained(this), 10000));
}

// TODO(alexclarke): Add additional tests with different mixes of non-delayed vs
// delayed tasks.

}  // namespace sequence_manager
}  // namespace base
