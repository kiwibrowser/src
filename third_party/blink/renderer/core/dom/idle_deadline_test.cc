// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/idle_deadline.h"

#include "base/single_thread_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support_with_mock_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {
namespace {

class MockIdleDeadlineScheduler final : public ThreadScheduler {
 public:
  MockIdleDeadlineScheduler() = default;
  ~MockIdleDeadlineScheduler() override = default;

  // ThreadScheduler implementation:
  scoped_refptr<base::SingleThreadTaskRunner> V8TaskRunner() override {
    return nullptr;
  }
  void Shutdown() override {}
  bool ShouldYieldForHighPriorityWork() override { return true; }
  bool CanExceedIdleDeadlineIfRequired() const override { return false; }
  void PostIdleTask(const base::Location&, WebThread::IdleTask) override {}
  void PostNonNestableIdleTask(const base::Location&,
                               WebThread::IdleTask) override {}
  std::unique_ptr<PageScheduler> CreatePageScheduler(
      PageScheduler::Delegate*) override {
    return nullptr;
  }
  scoped_refptr<base::SingleThreadTaskRunner> CompositorTaskRunner() override {
    return nullptr;
  }
  std::unique_ptr<RendererPauseHandle> PauseScheduler() override {
    return nullptr;
  }

  base::TimeTicks MonotonicallyIncreasingVirtualTime() override {
    return base::TimeTicks();
  }

  void AddTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override {}

  void RemoveTaskObserver(
      base::MessageLoop::TaskObserver* task_observer) override {}

  scheduler::NonMainThreadScheduler* AsNonMainThreadScheduler() override {
    return nullptr;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockIdleDeadlineScheduler);
};

class MockIdleDeadlineThread final : public WebThread {
 public:
  MockIdleDeadlineThread() = default;
  ~MockIdleDeadlineThread() override = default;
  bool IsCurrentThread() const override { return true; }
  ThreadScheduler* Scheduler() const override { return &scheduler_; }

 private:
  mutable MockIdleDeadlineScheduler scheduler_;
  DISALLOW_COPY_AND_ASSIGN(MockIdleDeadlineThread);
};

class MockIdleDeadlinePlatform : public TestingPlatformSupport {
 public:
  MockIdleDeadlinePlatform() = default;
  ~MockIdleDeadlinePlatform() override = default;
  WebThread* CurrentThread() override { return &thread_; }

 private:
  MockIdleDeadlineThread thread_;
  DISALLOW_COPY_AND_ASSIGN(MockIdleDeadlinePlatform);
};

}  // namespace

class IdleDeadlineTest : public testing::Test {
 public:
  void SetUp() override {
    original_time_function_ = SetTimeFunctionsForTesting([] { return 1.0; });
  }

  void TearDown() override {
    SetTimeFunctionsForTesting(original_time_function_);
  }

 private:
  TimeFunction original_time_function_;
};

TEST_F(IdleDeadlineTest, deadlineInFuture) {
  IdleDeadline* deadline =
      IdleDeadline::Create(1.25, IdleDeadline::CallbackType::kCalledWhenIdle);
  // Note: the deadline is computed with reduced resolution.
  EXPECT_FLOAT_EQ(250.0, deadline->timeRemaining());
}

TEST_F(IdleDeadlineTest, deadlineInPast) {
  IdleDeadline* deadline =
      IdleDeadline::Create(0.75, IdleDeadline::CallbackType::kCalledWhenIdle);
  EXPECT_FLOAT_EQ(0, deadline->timeRemaining());
}

TEST_F(IdleDeadlineTest, yieldForHighPriorityWork) {
  ScopedTestingPlatformSupport<MockIdleDeadlinePlatform> platform;

  IdleDeadline* deadline =
      IdleDeadline::Create(1.25, IdleDeadline::CallbackType::kCalledWhenIdle);
  EXPECT_FLOAT_EQ(0, deadline->timeRemaining());
}

}  // namespace blink
