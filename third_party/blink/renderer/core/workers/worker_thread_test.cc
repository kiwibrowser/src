// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/worker_thread.h"

#include <memory>
#include <utility>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_cache_options.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/inspector/inspector_task_runner.h"
#include "third_party/blink/renderer/core/script/script.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread_test_helper.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/waitable_event.h"

using testing::_;
using testing::AtMost;

namespace blink {

using ExitCode = WorkerThread::ExitCode;

namespace {

class MockWorkerReportingProxy final : public WorkerReportingProxy {
 public:
  MockWorkerReportingProxy() = default;
  ~MockWorkerReportingProxy() override = default;

  MOCK_METHOD1(DidCreateWorkerGlobalScope, void(WorkerOrWorkletGlobalScope*));
  MOCK_METHOD0(DidInitializeWorkerContext, void());
  MOCK_METHOD2(WillEvaluateClassicScriptMock,
               void(size_t scriptSize, size_t cachedMetadataSize));
  MOCK_METHOD1(DidEvaluateClassicScript, void(bool success));
  MOCK_METHOD0(DidCloseWorkerGlobalScope, void());
  MOCK_METHOD0(WillDestroyWorkerGlobalScope, void());
  MOCK_METHOD0(DidTerminateWorkerThread, void());

  void WillEvaluateClassicScript(size_t script_size,
                                 size_t cached_metadata_size) override {
    script_evaluation_event_.Signal();
    WillEvaluateClassicScriptMock(script_size, cached_metadata_size);
  }

  void WaitUntilScriptEvaluation() { script_evaluation_event_.Wait(); }

 private:
  WaitableEvent script_evaluation_event_;
};

// Used as a debugger task. Waits for a signal from the main thread.
void WaitForSignalTask(WorkerThread* worker_thread,
                       WaitableEvent* waitable_event) {
  EXPECT_TRUE(worker_thread->IsCurrentThread());

  // Notify the main thread that the debugger task is waiting for the signal.
  PostCrossThreadTask(
      *worker_thread->GetParentExecutionContextTaskRunners()->Get(
          TaskType::kInternalTest),
      FROM_HERE, CrossThreadBind(&test::ExitRunLoop));
  waitable_event->Wait();
}

}  // namespace

class WorkerThreadTest : public testing::Test {
 public:
  WorkerThreadTest() = default;

  void SetUp() override {
    reporting_proxy_ = std::make_unique<MockWorkerReportingProxy>();
    security_origin_ = SecurityOrigin::Create(KURL("http://fake.url/"));
    worker_thread_ =
        std::make_unique<WorkerThreadForTest>(nullptr, *reporting_proxy_);
    lifecycle_observer_ = new MockWorkerThreadLifecycleObserver(
        worker_thread_->GetWorkerThreadLifecycleContext());
  }

  void TearDown() override {}

  void Start() {
    worker_thread_->StartWithSourceCode(
        security_origin_.get(), "//fake source code",
        ParentExecutionContextTaskRunners::Create());
  }

  void StartWithSourceCodeNotToFinish() {
    // Use a JavaScript source code that makes an infinite loop so that we
    // can catch some kind of issues as a timeout.
    worker_thread_->StartWithSourceCode(
        security_origin_.get(), "while(true) {}",
        ParentExecutionContextTaskRunners::Create());
  }

  void SetForcibleTerminationDelay(TimeDelta forcible_termination_delay) {
    worker_thread_->forcible_termination_delay_ = forcible_termination_delay;
  }

  bool IsForcibleTerminationTaskScheduled() {
    return worker_thread_->forcible_termination_task_handle_.IsActive();
  }

 protected:
  void ExpectReportingCalls() {
    EXPECT_CALL(*reporting_proxy_, DidCreateWorkerGlobalScope(_)).Times(1);
    EXPECT_CALL(*reporting_proxy_, DidInitializeWorkerContext()).Times(1);
    EXPECT_CALL(*reporting_proxy_, WillEvaluateClassicScriptMock(_, _))
        .Times(1);
    EXPECT_CALL(*reporting_proxy_, DidEvaluateClassicScript(true)).Times(1);
    EXPECT_CALL(*reporting_proxy_, WillDestroyWorkerGlobalScope()).Times(1);
    EXPECT_CALL(*reporting_proxy_, DidTerminateWorkerThread()).Times(1);
    EXPECT_CALL(*lifecycle_observer_, ContextDestroyed(_)).Times(1);
  }

  void ExpectReportingCallsForWorkerPossiblyTerminatedBeforeInitialization() {
    EXPECT_CALL(*reporting_proxy_, DidCreateWorkerGlobalScope(_)).Times(1);
    EXPECT_CALL(*reporting_proxy_, DidInitializeWorkerContext())
        .Times(AtMost(1));
    EXPECT_CALL(*reporting_proxy_, WillEvaluateClassicScriptMock(_, _))
        .Times(AtMost(1));
    EXPECT_CALL(*reporting_proxy_, DidEvaluateClassicScript(_))
        .Times(AtMost(1));
    EXPECT_CALL(*reporting_proxy_, WillDestroyWorkerGlobalScope())
        .Times(AtMost(1));
    EXPECT_CALL(*reporting_proxy_, DidTerminateWorkerThread()).Times(1);
    EXPECT_CALL(*lifecycle_observer_, ContextDestroyed(_)).Times(1);
  }

  void ExpectReportingCallsForWorkerForciblyTerminated() {
    EXPECT_CALL(*reporting_proxy_, DidCreateWorkerGlobalScope(_)).Times(1);
    EXPECT_CALL(*reporting_proxy_, DidInitializeWorkerContext()).Times(1);
    EXPECT_CALL(*reporting_proxy_, WillEvaluateClassicScriptMock(_, _))
        .Times(1);
    EXPECT_CALL(*reporting_proxy_, DidEvaluateClassicScript(false)).Times(1);
    EXPECT_CALL(*reporting_proxy_, WillDestroyWorkerGlobalScope()).Times(1);
    EXPECT_CALL(*reporting_proxy_, DidTerminateWorkerThread()).Times(1);
    EXPECT_CALL(*lifecycle_observer_, ContextDestroyed(_)).Times(1);
  }

  ExitCode GetExitCode() { return worker_thread_->GetExitCodeForTesting(); }

  scoped_refptr<const SecurityOrigin> security_origin_;
  std::unique_ptr<MockWorkerReportingProxy> reporting_proxy_;
  std::unique_ptr<WorkerThreadForTest> worker_thread_;
  Persistent<MockWorkerThreadLifecycleObserver> lifecycle_observer_;
};

TEST_F(WorkerThreadTest, ShouldTerminateScriptExecution) {
  using ThreadState = WorkerThread::ThreadState;

  worker_thread_->inspector_task_runner_ = InspectorTaskRunner::Create(nullptr);

  // SetExitCode() and ShouldTerminateScriptExecution() require the lock.
  MutexLocker dummy_lock(worker_thread_->mutex_);

  EXPECT_EQ(ThreadState::kNotStarted, worker_thread_->thread_state_);
  EXPECT_FALSE(worker_thread_->ShouldTerminateScriptExecution());

  worker_thread_->SetThreadState(ThreadState::kRunning);
  EXPECT_TRUE(worker_thread_->ShouldTerminateScriptExecution());

  worker_thread_->SetThreadState(ThreadState::kReadyToShutdown);
  EXPECT_FALSE(worker_thread_->ShouldTerminateScriptExecution());

  // This is necessary to satisfy DCHECK in the dtor of WorkerThread.
  worker_thread_->SetExitCode(ExitCode::kGracefullyTerminated);
}

TEST_F(WorkerThreadTest, AsyncTerminate_OnIdle) {
  ExpectReportingCalls();
  Start();

  // Wait until the initialization completes and the worker thread becomes
  // idle.
  worker_thread_->WaitForInit();

  // The worker thread is not being blocked, so the worker thread should be
  // gracefully shut down.
  worker_thread_->Terminate();
  EXPECT_TRUE(IsForcibleTerminationTaskScheduled());
  worker_thread_->WaitForShutdownForTesting();
  EXPECT_EQ(ExitCode::kGracefullyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest, SyncTerminate_OnIdle) {
  ExpectReportingCalls();
  Start();

  // Wait until the initialization completes and the worker thread becomes
  // idle.
  worker_thread_->WaitForInit();

  WorkerThread::TerminateAllWorkersForTesting();

  // The worker thread may gracefully shut down before forcible termination
  // runs.
  ExitCode exit_code = GetExitCode();
  EXPECT_TRUE(ExitCode::kGracefullyTerminated == exit_code ||
              ExitCode::kSyncForciblyTerminated == exit_code);
}

TEST_F(WorkerThreadTest, AsyncTerminate_ImmediatelyAfterStart) {
  ExpectReportingCallsForWorkerPossiblyTerminatedBeforeInitialization();
  Start();

  // The worker thread is not being blocked, so the worker thread should be
  // gracefully shut down.
  worker_thread_->Terminate();
  worker_thread_->WaitForShutdownForTesting();
  EXPECT_EQ(ExitCode::kGracefullyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest, SyncTerminate_ImmediatelyAfterStart) {
  ExpectReportingCallsForWorkerPossiblyTerminatedBeforeInitialization();
  Start();

  // There are two possible cases depending on timing:
  // (1) If the thread hasn't been initialized on the worker thread yet,
  // TerminateAllWorkersForTesting() should wait for initialization and shut
  // down the thread immediately after that.
  // (2) If the thread has already been initialized on the worker thread,
  // TerminateAllWorkersForTesting() should synchronously forcibly terminates
  // the worker execution.
  WorkerThread::TerminateAllWorkersForTesting();
  ExitCode exit_code = GetExitCode();
  EXPECT_TRUE(ExitCode::kGracefullyTerminated == exit_code ||
              ExitCode::kSyncForciblyTerminated == exit_code);
}

TEST_F(WorkerThreadTest, AsyncTerminate_WhileTaskIsRunning) {
  constexpr TimeDelta kDelay = TimeDelta::FromMilliseconds(10);
  SetForcibleTerminationDelay(kDelay);

  ExpectReportingCallsForWorkerForciblyTerminated();
  StartWithSourceCodeNotToFinish();
  reporting_proxy_->WaitUntilScriptEvaluation();

  // Terminate() schedules a forcible termination task.
  worker_thread_->Terminate();
  EXPECT_TRUE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Multiple Terminate() calls should not take effect.
  worker_thread_->Terminate();
  worker_thread_->Terminate();
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Wait until the forcible termination task runs.
  test::RunDelayedTasks(kDelay);
  worker_thread_->WaitForShutdownForTesting();
  EXPECT_EQ(ExitCode::kAsyncForciblyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest, SyncTerminate_WhileTaskIsRunning) {
  ExpectReportingCallsForWorkerForciblyTerminated();
  StartWithSourceCodeNotToFinish();
  reporting_proxy_->WaitUntilScriptEvaluation();

  // TerminateAllWorkersForTesting() synchronously terminates the worker
  // execution.
  WorkerThread::TerminateAllWorkersForTesting();
  EXPECT_EQ(ExitCode::kSyncForciblyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest,
       AsyncTerminateAndThenSyncTerminate_WhileTaskIsRunning) {
  SetForcibleTerminationDelay(TimeDelta::FromMilliseconds(10));

  ExpectReportingCallsForWorkerForciblyTerminated();
  StartWithSourceCodeNotToFinish();
  reporting_proxy_->WaitUntilScriptEvaluation();

  // Terminate() schedules a forcible termination task.
  worker_thread_->Terminate();
  EXPECT_TRUE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // TerminateAllWorkersForTesting() should overtake the scheduled forcible
  // termination task.
  WorkerThread::TerminateAllWorkersForTesting();
  EXPECT_FALSE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kSyncForciblyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest, Terminate_WhileDebuggerTaskIsRunningOnInitialization) {
  constexpr TimeDelta kDelay = TimeDelta::FromMilliseconds(10);
  SetForcibleTerminationDelay(kDelay);

  EXPECT_CALL(*reporting_proxy_, DidCreateWorkerGlobalScope(_)).Times(1);
  EXPECT_CALL(*reporting_proxy_, DidInitializeWorkerContext()).Times(1);
  EXPECT_CALL(*reporting_proxy_, WillDestroyWorkerGlobalScope()).Times(1);
  EXPECT_CALL(*reporting_proxy_, DidTerminateWorkerThread()).Times(1);
  EXPECT_CALL(*lifecycle_observer_, ContextDestroyed(_)).Times(1);

  auto headers = std::make_unique<Vector<CSPHeaderAndType>>();
  CSPHeaderAndType header_and_type("contentSecurityPolicy",
                                   kContentSecurityPolicyHeaderTypeReport);
  headers->push_back(header_and_type);

  auto global_scope_creation_params =
      std::make_unique<GlobalScopeCreationParams>(
          KURL("http://fake.url/"), ScriptType::kClassic, "fake user agent",
          headers.get(), kReferrerPolicyDefault, security_origin_.get(),
          false /* starter_secure_context */, nullptr /* workerClients */,
          mojom::IPAddressSpace::kLocal, nullptr /* originTrialToken */,
          base::UnguessableToken::Create(),
          std::make_unique<WorkerSettings>(Settings::Create().get()),
          kV8CacheOptionsDefault, nullptr /* worklet_module_responses_map */);

  // Specify PauseOnWorkerStart::kPause so that the worker thread can pause
  // on initialization to run debugger tasks.
  worker_thread_->Start(std::move(global_scope_creation_params),
                        WorkerBackingThreadStartupData::CreateDefault(),
                        WorkerInspectorProxy::PauseOnWorkerStart::kPause,
                        ParentExecutionContextTaskRunners::Create());

  // Used to wait for worker thread termination in a debugger task on the
  // worker thread.
  WaitableEvent waitable_event;
  worker_thread_->AppendDebuggerTask(CrossThreadBind(
      &WaitForSignalTask, CrossThreadUnretained(worker_thread_.get()),
      CrossThreadUnretained(&waitable_event)));

  // Wait for the debugger task.
  test::EnterRunLoop();
  EXPECT_TRUE(worker_thread_->inspector_task_runner_->IsRunningTask());

  // Terminate() schedules a forcible termination task.
  worker_thread_->Terminate();
  EXPECT_TRUE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Wait until the task runs. It shouldn't terminate the script execution
  // because of the running debugger task.
  test::RunDelayedTasks(kDelay);
  EXPECT_FALSE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Forcible script termination request should also respect the running
  // debugger task.
  worker_thread_->EnsureScriptExecutionTerminates(
      ExitCode::kSyncForciblyTerminated);
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Resume the debugger task. Shutdown starts after that.
  waitable_event.Signal();
  worker_thread_->WaitForShutdownForTesting();
  EXPECT_EQ(ExitCode::kGracefullyTerminated, GetExitCode());
}

TEST_F(WorkerThreadTest, Terminate_WhileDebuggerTaskIsRunning) {
  constexpr TimeDelta kDelay = TimeDelta::FromMilliseconds(10);
  SetForcibleTerminationDelay(kDelay);

  ExpectReportingCalls();
  Start();
  worker_thread_->WaitForInit();

  // Used to wait for worker thread termination in a debugger task on the
  // worker thread.
  WaitableEvent waitable_event;
  worker_thread_->AppendDebuggerTask(CrossThreadBind(
      &WaitForSignalTask, CrossThreadUnretained(worker_thread_.get()),
      CrossThreadUnretained(&waitable_event)));

  // Wait for the debugger task.
  test::EnterRunLoop();
  EXPECT_TRUE(worker_thread_->inspector_task_runner_->IsRunningTask());

  // Terminate() schedules a forcible termination task.
  worker_thread_->Terminate();
  EXPECT_TRUE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Wait until the task runs. It shouldn't terminate the script execution
  // because of the running debugger task.
  test::RunDelayedTasks(kDelay);
  EXPECT_FALSE(IsForcibleTerminationTaskScheduled());
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Forcible script termination request should also respect the running
  // debugger task.
  worker_thread_->EnsureScriptExecutionTerminates(
      ExitCode::kSyncForciblyTerminated);
  EXPECT_EQ(ExitCode::kNotTerminated, GetExitCode());

  // Resume the debugger task. Shutdown starts after that.
  waitable_event.Signal();
  worker_thread_->WaitForShutdownForTesting();
  EXPECT_EQ(ExitCode::kGracefullyTerminated, GetExitCode());
}

}  // namespace blink
