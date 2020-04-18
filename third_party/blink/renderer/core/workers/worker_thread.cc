/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "third_party/blink/renderer/core/workers/worker_thread.h"

#include <limits>
#include <memory>
#include <utility>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/bindings/core/v8/script_source_code.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/inspector/console_message_storage.h"
#include "third_party/blink/renderer/core/inspector/inspector_task_runner.h"
#include "third_party/blink/renderer/core/inspector/worker_inspector_controller.h"
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/threaded_worklet_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"
#include "third_party/blink/renderer/core/workers/worker_clients.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/platform/bindings/microtask.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/heap/safe_point.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/scheduler/child/webthread_impl_for_worker_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/child/worker_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/non_main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/waitable_event.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

using ExitCode = WorkerThread::ExitCode;

namespace {

// TODO(nhiroki): Adjust the delay based on UMA.
constexpr TimeDelta kForcibleTerminationDelay = TimeDelta::FromSeconds(2);

}  // namespace

static Mutex& ThreadSetMutex() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(Mutex, mutex, ());
  return mutex;
}

static std::atomic_int g_unique_worker_thread_id(1);

static int GetNextWorkerThreadId() {
  int next_worker_thread_id =
      g_unique_worker_thread_id.fetch_add(1, std::memory_order_relaxed);
  CHECK_LT(next_worker_thread_id, std::numeric_limits<int>::max());
  return next_worker_thread_id;
}

WorkerThread::~WorkerThread() {
  MutexLocker lock(ThreadSetMutex());
  DCHECK(WorkerThreads().Contains(this));
  WorkerThreads().erase(this);

  DCHECK(child_threads_.IsEmpty());
  DCHECK_NE(ExitCode::kNotTerminated, exit_code_);
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      EnumerationHistogram, exit_code_histogram,
      ("WorkerThread.ExitCode", static_cast<int>(ExitCode::kLastEnum)));
  exit_code_histogram.Count(static_cast<int>(exit_code_));
}

void WorkerThread::Start(
    std::unique_ptr<GlobalScopeCreationParams> global_scope_creation_params,
    const base::Optional<WorkerBackingThreadStartupData>& thread_startup_data,
    WorkerInspectorProxy::PauseOnWorkerStart pause_on_start,
    ParentExecutionContextTaskRunners* parent_execution_context_task_runners) {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  DCHECK(!parent_execution_context_task_runners_);
  parent_execution_context_task_runners_ =
      parent_execution_context_task_runners;

  // Synchronously initialize the per-global-scope scheduler to prevent someone
  // from posting a task to the thread before the scheduler is ready.
  WaitableEvent waitable_event;
  GetWorkerBackingThread().BackingThread().PostTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThread::InitializeSchedulerOnWorkerThread,
                      CrossThreadUnretained(this),
                      CrossThreadUnretained(&waitable_event)));
  waitable_event.Wait();

  inspector_task_runner_ =
      InspectorTaskRunner::Create(GetTaskRunner(TaskType::kInternalInspector));

  GetWorkerBackingThread().BackingThread().PostTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThread::InitializeOnWorkerThread,
                      CrossThreadUnretained(this),
                      WTF::Passed(std::move(global_scope_creation_params)),
                      thread_startup_data, pause_on_start));
}

void WorkerThread::EvaluateClassicScript(
    const KURL& script_url,
    const String& source_code,
    std::unique_ptr<Vector<char>> cached_meta_data,
    const v8_inspector::V8StackTraceId& stack_id) {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  PostCrossThreadTask(
      *GetTaskRunner(TaskType::kInternalWorker), FROM_HERE,
      CrossThreadBind(&WorkerThread::EvaluateClassicScriptOnWorkerThread,
                      CrossThreadUnretained(this), script_url, source_code,
                      WTF::Passed(std::move(cached_meta_data)), stack_id));
}

void WorkerThread::ImportModuleScript(
    const KURL& script_url,
    network::mojom::FetchCredentialsMode credentials_mode) {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  PostCrossThreadTask(
      *GetTaskRunner(TaskType::kInternalWorker), FROM_HERE,
      CrossThreadBind(&WorkerThread::ImportModuleScriptOnWorkerThread,
                      CrossThreadUnretained(this), script_url,
                      credentials_mode));
}

void WorkerThread::TerminateChildThreadsOnWorkerThread() {
  DCHECK(IsCurrentThread());
  PrepareForShutdownOnWorkerThread();
  for (WorkerThread* child : child_threads_)
    child->Terminate();
}

void WorkerThread::Terminate() {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  {
    MutexLocker lock(mutex_);
    if (requested_to_terminate_)
      return;
    requested_to_terminate_ = true;
  }

  // Schedule a task to forcibly terminate the script execution in case that the
  // shutdown sequence does not start on the worker thread in a certain time
  // period.
  ScheduleToTerminateScriptExecution();

  worker_thread_lifecycle_context_->NotifyContextDestroyed();
  inspector_task_runner_->Dispose();

  if (!child_threads_.IsEmpty()) {
    // When child workers are present, wait for them to shutdown before shutting
    // down this thread. ChildThreadTerminatedOnWorkerThread() is responsible
    // for completing shutdown on the worker thread after the last child shuts
    // down.
    GetWorkerBackingThread().BackingThread().PostTask(
        FROM_HERE,
        CrossThreadBind(&WorkerThread::TerminateChildThreadsOnWorkerThread,
                        CrossThreadUnretained(this)));
    return;
  }

  GetWorkerBackingThread().BackingThread().PostTask(
      FROM_HERE,
      CrossThreadBind(&WorkerThread::PrepareForShutdownOnWorkerThread,
                      CrossThreadUnretained(this)));
  GetWorkerBackingThread().BackingThread().PostTask(
      FROM_HERE, CrossThreadBind(&WorkerThread::PerformShutdownOnWorkerThread,
                                 CrossThreadUnretained(this)));
}

void WorkerThread::TerminateForTesting() {
  // Schedule a regular async worker thread termination task, and forcibly
  // terminate the V8 script execution to ensure the task runs.
  Terminate();
  EnsureScriptExecutionTerminates(ExitCode::kSyncForciblyTerminated);
}

void WorkerThread::TerminateAllWorkersForTesting() {
  DCHECK(IsMainThread());

  // Keep this lock to prevent WorkerThread instances from being destroyed.
  MutexLocker lock(ThreadSetMutex());
  HashSet<WorkerThread*> threads = WorkerThreads();

  for (WorkerThread* thread : threads) {
    thread->TerminateForTesting();
  }

  for (WorkerThread* thread : threads)
    thread->shutdown_event_->Wait();

  // Destruct base::Thread and join the underlying system threads.
  for (WorkerThread* thread : threads)
    thread->ClearWorkerBackingThread();
}

void WorkerThread::WillProcessTask() {
  DCHECK(IsCurrentThread());

  // No tasks should get executed after we have closed.
  DCHECK(!GlobalScope()->IsClosing());
}

void WorkerThread::DidProcessTask() {
  DCHECK(IsCurrentThread());
  Microtask::PerformCheckpoint(GetIsolate());
  GlobalScope()->ScriptController()->GetRejectedPromises()->ProcessQueue();
  if (GlobalScope()->IsClosing()) {
    // This WorkerThread will eventually be requested to terminate.
    GetWorkerReportingProxy().DidCloseWorkerGlobalScope();

    // Stop further worker tasks to run after this point.
    PrepareForShutdownOnWorkerThread();
  } else if (IsForciblyTerminated()) {
    // The script has been terminated forcibly, which means we need to
    // ask objects in the thread to stop working as soon as possible.
    PrepareForShutdownOnWorkerThread();
  }
}

v8::Isolate* WorkerThread::GetIsolate() {
  return GetWorkerBackingThread().GetIsolate();
}

bool WorkerThread::IsCurrentThread() {
  return GetWorkerBackingThread().BackingThread().IsCurrentThread();
}

ThreadableLoadingContext* WorkerThread::GetLoadingContext() {
  DCHECK(IsCurrentThread());
  // This should be never called after the termination sequence starts.
  DCHECK(loading_context_);
  return loading_context_;
}

void WorkerThread::AppendDebuggerTask(CrossThreadClosure task) {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  inspector_task_runner_->AppendTask(std::move(task));
}

void WorkerThread::StartRunningDebuggerTasksOnPauseOnWorkerThread() {
  DCHECK(IsCurrentThread());
  if (worker_inspector_controller_)
    worker_inspector_controller_->FlushProtocolNotifications();
  paused_in_debugger_ = true;
  do {
    if (!inspector_task_runner_->WaitForAndRunSingleTask())
      break;
    // Keep waiting until execution is resumed.
  } while (paused_in_debugger_);
}

void WorkerThread::StopRunningDebuggerTasksOnPauseOnWorkerThread() {
  DCHECK(IsCurrentThread());
  paused_in_debugger_ = false;
}

WorkerOrWorkletGlobalScope* WorkerThread::GlobalScope() {
  DCHECK(IsCurrentThread());
  return global_scope_.Get();
}

WorkerInspectorController* WorkerThread::GetWorkerInspectorController() {
  DCHECK(IsCurrentThread());
  return worker_inspector_controller_.Get();
}

unsigned WorkerThread::WorkerThreadCount() {
  MutexLocker lock(ThreadSetMutex());
  return WorkerThreads().size();
}

HashSet<WorkerThread*>& WorkerThread::WorkerThreads() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(HashSet<WorkerThread*>, threads, ());
  return threads;
}

PlatformThreadId WorkerThread::GetPlatformThreadId() {
  return GetWorkerBackingThread().BackingThread().PlatformThread().ThreadId();
}

bool WorkerThread::IsForciblyTerminated() {
  MutexLocker lock(mutex_);
  switch (exit_code_) {
    case ExitCode::kNotTerminated:
    case ExitCode::kGracefullyTerminated:
      return false;
    case ExitCode::kSyncForciblyTerminated:
    case ExitCode::kAsyncForciblyTerminated:
      return true;
    case ExitCode::kLastEnum:
      NOTREACHED() << static_cast<int>(exit_code_);
      return false;
  }
  NOTREACHED() << static_cast<int>(exit_code_);
  return false;
}

ExitCode WorkerThread::GetExitCodeForTesting() {
  MutexLocker lock(mutex_);
  return exit_code_;
}

scheduler::WorkerScheduler* WorkerThread::GetScheduler() {
  DCHECK(IsCurrentThread());
  return worker_scheduler_.get();
}

void WorkerThread::ChildThreadStartedOnWorkerThread(WorkerThread* child) {
  DCHECK(IsCurrentThread());
  child_threads_.insert(child);
}

void WorkerThread::ChildThreadTerminatedOnWorkerThread(WorkerThread* child) {
  DCHECK(IsCurrentThread());
  child_threads_.erase(child);
  if (child_threads_.IsEmpty() && CheckRequestedToTerminate())
    PerformShutdownOnWorkerThread();
}

WorkerThread::WorkerThread(ThreadableLoadingContext* loading_context,
                           WorkerReportingProxy& worker_reporting_proxy)
    : time_origin_(CurrentTimeTicksInSeconds()),
      worker_thread_id_(GetNextWorkerThreadId()),
      forcible_termination_delay_(kForcibleTerminationDelay),
      devtools_worker_token_(base::UnguessableToken::Create()),
      loading_context_(loading_context),
      worker_reporting_proxy_(worker_reporting_proxy),
      shutdown_event_(std::make_unique<WaitableEvent>(
          WaitableEvent::ResetPolicy::kManual,
          WaitableEvent::InitialState::kNonSignaled)),
      worker_thread_lifecycle_context_(new WorkerThreadLifecycleContext) {
  MutexLocker lock(ThreadSetMutex());
  WorkerThreads().insert(this);
}

void WorkerThread::ScheduleToTerminateScriptExecution() {
  DCHECK(!forcible_termination_task_handle_.IsActive());
  forcible_termination_task_handle_ = PostDelayedCancellableTask(
      *parent_execution_context_task_runners_->Get(TaskType::kInternalDefault),
      FROM_HERE,
      WTF::Bind(&WorkerThread::EnsureScriptExecutionTerminates,
                WTF::Unretained(this), ExitCode::kAsyncForciblyTerminated),
      forcible_termination_delay_);
}

bool WorkerThread::ShouldTerminateScriptExecution() {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  switch (thread_state_) {
    case ThreadState::kNotStarted:
      // Shutdown sequence will surely start during initialization sequence
      // on the worker thread. Don't have to schedule a termination task.
      return false;
    case ThreadState::kRunning:
      // Terminating during debugger task may lead to crash due to heavy use
      // of v8 api in debugger. Any debugger task is guaranteed to finish, so
      // we can wait for the completion.
      return !inspector_task_runner_->IsRunningTask();
    case ThreadState::kReadyToShutdown:
      // Shutdown sequence will surely start soon. Don't have to schedule a
      // termination task.
      return false;
  }
  NOTREACHED();
  return false;
}

void WorkerThread::EnsureScriptExecutionTerminates(ExitCode exit_code) {
  DCHECK_CALLED_ON_VALID_THREAD(parent_thread_checker_);
  MutexLocker lock(mutex_);
  if (!ShouldTerminateScriptExecution())
    return;

  DCHECK(exit_code == ExitCode::kSyncForciblyTerminated ||
         exit_code == ExitCode::kAsyncForciblyTerminated);
  SetExitCode(exit_code);

  GetIsolate()->TerminateExecution();
  forcible_termination_task_handle_.Cancel();
}

void WorkerThread::InitializeSchedulerOnWorkerThread(
    WaitableEvent* waitable_event) {
  DCHECK(IsCurrentThread());
  DCHECK(!worker_scheduler_);
  scheduler::WebThreadImplForWorkerScheduler& web_thread_for_worker =
      static_cast<scheduler::WebThreadImplForWorkerScheduler&>(
          GetWorkerBackingThread().BackingThread().PlatformThread());
  worker_scheduler_ = std::make_unique<scheduler::WorkerScheduler>(
      web_thread_for_worker.GetNonMainThreadScheduler());
  waitable_event->Signal();
}

void WorkerThread::InitializeOnWorkerThread(
    std::unique_ptr<GlobalScopeCreationParams> global_scope_creation_params,
    const base::Optional<WorkerBackingThreadStartupData>& thread_startup_data,
    WorkerInspectorProxy::PauseOnWorkerStart pause_on_start) {
  DCHECK(IsCurrentThread());
  {
    MutexLocker lock(mutex_);
    DCHECK_EQ(ThreadState::kNotStarted, thread_state_);

    if (IsOwningBackingThread()) {
      DCHECK(thread_startup_data.has_value());
      GetWorkerBackingThread().InitializeOnBackingThread(*thread_startup_data);
    } else {
      DCHECK(!thread_startup_data.has_value());
    }
    GetWorkerBackingThread().BackingThread().AddTaskObserver(this);

    console_message_storage_ = new ConsoleMessageStorage();
    global_scope_ =
        CreateWorkerGlobalScope(std::move(global_scope_creation_params));
    worker_reporting_proxy_.DidCreateWorkerGlobalScope(GlobalScope());
    worker_inspector_controller_ = WorkerInspectorController::Create(this);

    // Since context initialization below may fail, we should notify debugger
    // about the new worker thread separately, so that it can resolve it by id
    // at any moment.
    if (WorkerThreadDebugger* debugger =
            WorkerThreadDebugger::From(GetIsolate()))
      debugger->WorkerThreadCreated(this);

    // TODO(nhiroki): Handle a case where the script controller fails to
    // initialize the context.
    if (GlobalScope()->ScriptController()->InitializeContextIfNeeded(
            String())) {
      worker_reporting_proxy_.DidInitializeWorkerContext();
      v8::HandleScope handle_scope(GetIsolate());
      Platform::Current()->WorkerContextCreated(
          GlobalScope()->ScriptController()->GetContext());
    }

    inspector_task_runner_->InitIsolate(GetIsolate());
    SetThreadState(ThreadState::kRunning);
  }

  // It is important that no code is run on the Isolate between
  // initializing InspectorTaskRunner and pausing on start.
  // Otherwise, InspectorTaskRunner might interrupt isolate execution
  // from another thread and try to resume "pause on start" before
  // we even paused.
  if (pause_on_start == WorkerInspectorProxy::PauseOnWorkerStart::kPause)
    StartRunningDebuggerTasksOnPauseOnWorkerThread();

  if (CheckRequestedToTerminate()) {
    // Stop further worker tasks from running after this point. WorkerThread
    // was requested to terminate before initialization or during running
    // debugger tasks. PerformShutdownOnWorkerThread() will be called soon.
    PrepareForShutdownOnWorkerThread();
    return;
  }
}

void WorkerThread::EvaluateClassicScriptOnWorkerThread(
    const KURL& script_url,
    String source_code,
    std::unique_ptr<Vector<char>> cached_meta_data,
    const v8_inspector::V8StackTraceId& stack_id) {
  WorkerThreadDebugger* debugger = WorkerThreadDebugger::From(GetIsolate());
  debugger->ExternalAsyncTaskStarted(stack_id);
  ToWorkerGlobalScope(GlobalScope())
      ->EvaluateClassicScript(script_url, std::move(source_code),
                              std::move(cached_meta_data));
  debugger->ExternalAsyncTaskFinished(stack_id);
}

void WorkerThread::ImportModuleScriptOnWorkerThread(
    const KURL& script_url,
    network::mojom::FetchCredentialsMode credentials_mode) {
  // Worklets have a different code path to import module scripts.
  // TODO(nhiroki): Consider excluding this code path from WorkerThread like
  // Worklets.
  ToWorkerGlobalScope(GlobalScope())
      ->ImportModuleScript(script_url, credentials_mode);
}

void WorkerThread::PrepareForShutdownOnWorkerThread() {
  DCHECK(IsCurrentThread());
  {
    MutexLocker lock(mutex_);
    if (thread_state_ == ThreadState::kReadyToShutdown)
      return;
    SetThreadState(ThreadState::kReadyToShutdown);
    if (exit_code_ == ExitCode::kNotTerminated)
      SetExitCode(ExitCode::kGracefullyTerminated);
  }

  inspector_task_runner_->Dispose();
  GetWorkerReportingProxy().WillDestroyWorkerGlobalScope();
  probe::AllAsyncTasksCanceled(GlobalScope());

  GlobalScope()->NotifyContextDestroyed();
  if (worker_inspector_controller_) {
    worker_inspector_controller_->Dispose();
    worker_inspector_controller_.Clear();
  }
  worker_scheduler_->Dispose();
  GlobalScope()->Dispose();
  global_scope_ = nullptr;

  if (WorkerThreadDebugger* debugger = WorkerThreadDebugger::From(GetIsolate()))
    debugger->WorkerThreadDestroyed(this);

  console_message_storage_.Clear();
  loading_context_.Clear();
  GetWorkerBackingThread().BackingThread().RemoveTaskObserver(this);
}

void WorkerThread::PerformShutdownOnWorkerThread() {
  DCHECK(IsCurrentThread());
#if DCHECK_IS_ON()
  {
    MutexLocker lock(mutex_);
    DCHECK(requested_to_terminate_);
    DCHECK_EQ(ThreadState::kReadyToShutdown, thread_state_);
  }
#endif

  if (IsOwningBackingThread())
    GetWorkerBackingThread().ShutdownOnBackingThread();
  // We must not touch workerBackingThread() from now on.

  // Notify the proxy that the WorkerOrWorkletGlobalScope has been disposed
  // of. This can free this thread object, hence it must not be touched
  // afterwards.
  GetWorkerReportingProxy().DidTerminateWorkerThread();

  shutdown_event_->Signal();
}

void WorkerThread::SetThreadState(ThreadState next_thread_state) {
  switch (next_thread_state) {
    case ThreadState::kNotStarted:
      NOTREACHED();
      return;
    case ThreadState::kRunning:
      DCHECK_EQ(ThreadState::kNotStarted, thread_state_);
      thread_state_ = next_thread_state;
      return;
    case ThreadState::kReadyToShutdown:
      DCHECK_EQ(ThreadState::kRunning, thread_state_);
      thread_state_ = next_thread_state;
      return;
  }
}

void WorkerThread::SetExitCode(ExitCode exit_code) {
  DCHECK_EQ(ExitCode::kNotTerminated, exit_code_);
  exit_code_ = exit_code;
}

bool WorkerThread::CheckRequestedToTerminate() {
  MutexLocker lock(mutex_);
  return requested_to_terminate_;
}

}  // namespace blink
