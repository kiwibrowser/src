// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/scripted_idle_task_controller.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_idle_request_callback.h"
#include "third_party/blink/renderer/core/dom/idle_request_options.h"
#include "third_party/blink/renderer/core/testing/null_execution_context.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {
namespace {

class MockScriptedIdleTaskControllerScheduler final : public ThreadScheduler {
 public:
  MockScriptedIdleTaskControllerScheduler(bool should_yield)
      : should_yield_(should_yield) {}
  ~MockScriptedIdleTaskControllerScheduler() override = default;

  // ThreadScheduler implementation:
  scoped_refptr<base::SingleThreadTaskRunner> CompositorTaskRunner() override {
    return nullptr;
  }
  scoped_refptr<base::SingleThreadTaskRunner> V8TaskRunner() override {
    return nullptr;
  }
  void Shutdown() override {}
  bool ShouldYieldForHighPriorityWork() override { return should_yield_; }
  bool CanExceedIdleDeadlineIfRequired() const override { return false; }
  void PostIdleTask(const base::Location&,
                    WebThread::IdleTask idle_task) override {
    idle_task_ = std::move(idle_task);
  }
  void PostNonNestableIdleTask(const base::Location&,
                               WebThread::IdleTask) override {}
  std::unique_ptr<PageScheduler> CreatePageScheduler(
      PageScheduler::Delegate*) override {
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

  void RunIdleTask() { std::move(idle_task_).Run(0); }
  bool HasIdleTask() const { return !!idle_task_; }

 private:
  bool should_yield_;
  WebThread::IdleTask idle_task_;

  DISALLOW_COPY_AND_ASSIGN(MockScriptedIdleTaskControllerScheduler);
};

class MockScriptedIdleTaskControllerThread final : public WebThread {
 public:
  MockScriptedIdleTaskControllerThread(bool should_yield)
      : scheduler_(should_yield) {}
  ~MockScriptedIdleTaskControllerThread() override = default;
  bool IsCurrentThread() const override { return true; }
  ThreadScheduler* Scheduler() const override { return &scheduler_; }

  void RunIdleTask() { scheduler_.RunIdleTask(); }
  bool HasIdleTask() const { return scheduler_.HasIdleTask(); }

 private:
  mutable MockScriptedIdleTaskControllerScheduler scheduler_;
  DISALLOW_COPY_AND_ASSIGN(MockScriptedIdleTaskControllerThread);
};

class MockScriptedIdleTaskControllerPlatform : public TestingPlatformSupport {
 public:
  MockScriptedIdleTaskControllerPlatform(bool should_yield)
      : thread_(should_yield) {}
  ~MockScriptedIdleTaskControllerPlatform() override = default;
  WebThread* CurrentThread() override { return &thread_; }

  void RunIdleTask() { thread_.RunIdleTask(); }
  bool HasIdleTask() const { return thread_.HasIdleTask(); }

 private:
  MockScriptedIdleTaskControllerThread thread_;
  DISALLOW_COPY_AND_ASSIGN(MockScriptedIdleTaskControllerPlatform);
};

class MockIdleTask : public ScriptedIdleTaskController::IdleTask {
 public:
  MOCK_METHOD1(invoke, void(IdleDeadline*));
};
}  // namespace

class ScriptedIdleTaskControllerTest : public testing::Test {
 public:
  void SetUp() override { execution_context_ = new NullExecutionContext(); }

 protected:
  Persistent<ExecutionContext> execution_context_;
};

TEST_F(ScriptedIdleTaskControllerTest, RunCallback) {
  ScopedTestingPlatformSupport<MockScriptedIdleTaskControllerPlatform, bool>
      platform(false);
  NullExecutionContext execution_context;
  ScriptedIdleTaskController* controller =
      ScriptedIdleTaskController::Create(execution_context_);

  Persistent<MockIdleTask> idle_task(new MockIdleTask());
  IdleRequestOptions options;
  EXPECT_FALSE(platform->HasIdleTask());
  int id = controller->RegisterCallback(idle_task, options);
  EXPECT_TRUE(platform->HasIdleTask());
  EXPECT_NE(0, id);

  EXPECT_CALL(*idle_task, invoke(testing::_));
  platform->RunIdleTask();
  testing::Mock::VerifyAndClearExpectations(idle_task);
  EXPECT_FALSE(platform->HasIdleTask());
}

TEST_F(ScriptedIdleTaskControllerTest, DontRunCallbackWhenAskedToYield) {
  ScopedTestingPlatformSupport<MockScriptedIdleTaskControllerPlatform, bool>
      platform(true);
  NullExecutionContext execution_context;
  ScriptedIdleTaskController* controller =
      ScriptedIdleTaskController::Create(execution_context_);

  Persistent<MockIdleTask> idle_task(new MockIdleTask());
  IdleRequestOptions options;
  int id = controller->RegisterCallback(idle_task, options);
  EXPECT_NE(0, id);

  EXPECT_CALL(*idle_task, invoke(testing::_)).Times(0);
  platform->RunIdleTask();
  testing::Mock::VerifyAndClearExpectations(idle_task);

  // The idle task should have been reposted.
  EXPECT_TRUE(platform->HasIdleTask());
}

}  // namespace blink
