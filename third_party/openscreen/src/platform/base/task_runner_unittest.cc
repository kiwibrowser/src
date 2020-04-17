// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <thread>  // NOLINT

#include "platform/test/fake_clock.h"
#include "platform/api/task_runner_factory.h"
#include "platform/api/time.h"
#include "platform/base/task_runner_impl.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace platform {
namespace {

using std::chrono::milliseconds;

const auto kTaskRunnerSleepTime = milliseconds(1);
constexpr Clock::duration kWaitTimeout = milliseconds(1000);

void WaitUntilCondition(std::function<bool()> predicate) {
  while (!predicate()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
}

class FakeTaskWaiter final : public TaskRunnerImpl::TaskWaiter {
 public:
  explicit FakeTaskWaiter(platform::ClockNowFunctionPtr now_function)
      : now_function_(now_function) {}
  ~FakeTaskWaiter() override = default;

  Error WaitForTaskToBePosted(Clock::duration timeout) override {
    Clock::time_point start = now_function_();
    waiting_.store(true);
    while (!has_event_.load() && (now_function_() - start) < timeout) {
      ;
    }
    waiting_.store(false);
    has_event_.store(false);
    return Error::None();
  }

  void OnTaskPosted() override { has_event_.store(true); }

  void WakeUpAndStop() {
    OnTaskPosted();
    task_runner_->PostTask([this]() { task_runner_->RequestStopSoon(); });
  }

  bool IsWaiting() const { return waiting_.load(); }

  void SetTaskRunner(TaskRunnerImpl* task_runner) {
    task_runner_ = task_runner;
  }

 private:
  const platform::ClockNowFunctionPtr now_function_;
  TaskRunnerImpl* task_runner_;
  std::atomic<bool> has_event_{false};
  std::atomic<bool> waiting_{false};
};

class TaskRunnerWithWaiterFactory {
 public:
  static std::unique_ptr<TaskRunner> Create(
      platform::ClockNowFunctionPtr now_function) {
    fake_waiter = std::make_unique<FakeTaskWaiter>(now_function);
    auto runner = std::make_unique<TaskRunnerImpl>(
        now_function, fake_waiter.get(), std::chrono::hours(1));
    fake_waiter->SetTaskRunner(runner.get());
    return runner;
  }

  static std::unique_ptr<FakeTaskWaiter> fake_waiter;
};

// static
std::unique_ptr<FakeTaskWaiter> TaskRunnerWithWaiterFactory::fake_waiter;

}  // anonymous namespace

TEST(TaskRunnerTest, TaskRunnerFromFactoryExecutesTask) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  auto runner = TaskRunnerFactory::Create(fake_clock.now);

  std::thread t([&runner] {
    static_cast<TaskRunnerImpl*>(runner.get())->RunUntilStopped();
  });

  std::string ran_tasks = "";
  const auto task = [&ran_tasks] { ran_tasks += "1"; };
  EXPECT_EQ(ran_tasks, "");

  runner->PostTask(task);

  WaitUntilCondition([&ran_tasks] { return ran_tasks == "1"; });
  EXPECT_EQ(ran_tasks, "1");

  static_cast<TaskRunnerImpl*>(runner.get())->RequestStopSoon();
  t.join();
}

TEST(TaskRunnerTest, TaskRunnerRunsDelayedTasksInOrder) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  auto runner = TaskRunnerFactory::Create(fake_clock.now);

  std::thread t([&runner] {
    static_cast<TaskRunnerImpl*>(runner.get())->RunUntilStopped();
  });

  std::string ran_tasks = "";

  const auto kDelayTime = milliseconds(5);
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };
  runner->PostTaskWithDelay(task_one, kDelayTime);

  const auto task_two = [&ran_tasks] { ran_tasks += "2"; };
  runner->PostTaskWithDelay(task_two, kDelayTime * 2);

  EXPECT_EQ(ran_tasks, "");
  fake_clock.Advance(kDelayTime);
  WaitUntilCondition([&ran_tasks] { return ran_tasks == "1"; });
  EXPECT_EQ(ran_tasks, "1");

  fake_clock.Advance(kDelayTime);
  WaitUntilCondition([&ran_tasks] { return ran_tasks == "12"; });
  EXPECT_EQ(ran_tasks, "12");

  static_cast<TaskRunnerImpl*>(runner.get())->RequestStopSoon();
  t.join();
}

TEST(TaskRunnerTest, SingleThreadedTaskRunnerRunsSequentially) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(fake_clock.now);

  std::string ran_tasks;
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };
  const auto task_two = [&ran_tasks] { ran_tasks += "2"; };
  const auto task_three = [&ran_tasks] { ran_tasks += "3"; };
  const auto task_four = [&ran_tasks] { ran_tasks += "4"; };
  const auto task_five = [&ran_tasks] { ran_tasks += "5"; };

  runner.PostTask(task_one);
  runner.PostTask(task_two);
  runner.PostTask(task_three);
  runner.PostTask(task_four);
  runner.PostTask(task_five);
  EXPECT_EQ(ran_tasks, "");

  runner.RunUntilIdleForTesting();
  EXPECT_EQ(ran_tasks, "12345");
}

TEST(TaskRunnerTest, TaskRunnerCanStopRunning) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(fake_clock.now);

  std::string ran_tasks;
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };
  const auto task_two = [&ran_tasks] { ran_tasks += "2"; };

  runner.PostTask(task_one);
  EXPECT_EQ(ran_tasks, "");

  std::thread start_thread([&runner] { runner.RunUntilStopped(); });

  WaitUntilCondition([&ran_tasks] { return !ran_tasks.empty(); });
  EXPECT_EQ(ran_tasks, "1");

  // Since Stop is called first, and the single threaded task
  // runner should honor the queue, we know the task runner is not running
  // since task two doesn't get ran.
  runner.RequestStopSoon();
  runner.PostTask(task_two);
  EXPECT_EQ(ran_tasks, "1");

  start_thread.join();
}

TEST(TaskRunnerTest, StoppingDoesNotDeleteTasks) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(fake_clock.now);

  std::string ran_tasks;
  const auto task_one = [&ran_tasks] { ran_tasks += "1"; };

  runner.PostTask(task_one);
  runner.RequestStopSoon();

  EXPECT_EQ(ran_tasks, "");
  runner.RunUntilIdleForTesting();

  EXPECT_EQ(ran_tasks, "1");
}

TEST(TaskRunnerTest, TaskRunnerIsStableWithLotsOfTasks) {
  FakeClock fake_clock{platform::Clock::time_point(milliseconds(1337))};
  TaskRunnerImpl runner(fake_clock.now);

  const int kNumberOfTasks = 500;
  std::string expected_ran_tasks;
  expected_ran_tasks.append(kNumberOfTasks, '1');

  std::string ran_tasks;
  for (int i = 0; i < kNumberOfTasks; ++i) {
    const auto task = [&ran_tasks] { ran_tasks += "1"; };
    runner.PostTask(task);
  }

  runner.RunUntilIdleForTesting();
  EXPECT_EQ(ran_tasks, expected_ran_tasks);
}

TEST(TaskRunnerTest, TaskRunnerDelayedTasksDontBlockImmediateTasks) {
  TaskRunnerImpl runner(platform::Clock::now);

  std::string ran_tasks;
  const auto task = [&ran_tasks] { ran_tasks += "1"; };
  const auto delayed_task = [&ran_tasks] { ran_tasks += "A"; };

  runner.PostTaskWithDelay(delayed_task, milliseconds(10000));
  runner.PostTask(task);

  runner.RunUntilIdleForTesting();
  // The immediate task should have run, even though the delayed task
  // was added first.

  EXPECT_EQ(ran_tasks, "1");
}

TEST(TaskRunnerTest, TaskRunnerUsesEventWaiter) {
  std::unique_ptr<TaskRunner> runner =
      TaskRunnerWithWaiterFactory::Create(Clock::now);

  int x = 0;
  std::thread t([&runner, &x] {
    static_cast<TaskRunnerImpl*>(runner.get())->RunUntilStopped();
    x = 1;
  });

  const Clock::time_point start1 = Clock::now();
  FakeTaskWaiter* fake_waiter = TaskRunnerWithWaiterFactory::fake_waiter.get();
  while ((Clock::now() - start1) < kWaitTimeout && !fake_waiter->IsWaiting()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_TRUE(fake_waiter->IsWaiting());

  fake_waiter->WakeUpAndStop();
  const Clock::time_point start2 = Clock::now();
  while ((Clock::now() - start2) < kWaitTimeout && x == 0) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_EQ(x, 1);
  ASSERT_FALSE(fake_waiter->IsWaiting());
  t.join();
}

TEST(TaskRunnerTest, WakesEventWaiterOnPostTask) {
  std::unique_ptr<TaskRunner> runner =
      TaskRunnerWithWaiterFactory::Create(Clock::now);

  int x = 0;
  std::thread t([&runner] {
    static_cast<TaskRunnerImpl*>(runner.get())->RunUntilStopped();
  });

  const Clock::time_point start1 = Clock::now();
  FakeTaskWaiter* fake_waiter = TaskRunnerWithWaiterFactory::fake_waiter.get();
  while ((Clock::now() - start1) < kWaitTimeout && !fake_waiter->IsWaiting()) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_TRUE(fake_waiter->IsWaiting());

  runner->PostTask([&x]() { x = 1; });
  const Clock::time_point start2 = Clock::now();
  while ((Clock::now() - start2) < kWaitTimeout && x == 0) {
    std::this_thread::sleep_for(kTaskRunnerSleepTime);
  }
  ASSERT_EQ(x, 1);

  fake_waiter->WakeUpAndStop();
  t.join();
}

}  // namespace platform
}  // namespace openscreen
