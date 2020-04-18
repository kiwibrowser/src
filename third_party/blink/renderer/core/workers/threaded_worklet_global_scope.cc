// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/threaded_worklet_global_scope.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/inspector/console_message_storage.h"
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

ThreadedWorkletGlobalScope::ThreadedWorkletGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params,
    v8::Isolate* isolate,
    WorkerThread* thread)
    : WorkletGlobalScope(
          std::move(creation_params),
          isolate,
          thread->GetWorkerReportingProxy(),
          // Specify |kInternalLoading| because these task runners are used
          // during module loading and this usage is not explicitly spec'ed.
          thread->GetParentExecutionContextTaskRunners()->Get(
              TaskType::kInternalLoading),
          thread->GetTaskRunner(TaskType::kInternalLoading)),
      thread_(thread) {}

ThreadedWorkletGlobalScope::~ThreadedWorkletGlobalScope() {
  DCHECK(!thread_);
}

void ThreadedWorkletGlobalScope::Dispose() {
  DCHECK(IsContextThread());
  WorkletGlobalScope::Dispose();
  thread_ = nullptr;
}

bool ThreadedWorkletGlobalScope::IsContextThread() const {
  return GetThread()->IsCurrentThread();
}

void ThreadedWorkletGlobalScope::AddConsoleMessage(
    ConsoleMessage* console_message) {
  DCHECK(IsContextThread());
  GetThread()->GetWorkerReportingProxy().ReportConsoleMessage(
      console_message->Source(), console_message->Level(),
      console_message->Message(), console_message->Location());
  GetThread()->GetConsoleMessageStorage()->AddConsoleMessage(this,
                                                             console_message);
}

void ThreadedWorkletGlobalScope::ExceptionThrown(ErrorEvent* error_event) {
  DCHECK(IsContextThread());
  if (WorkerThreadDebugger* debugger =
          WorkerThreadDebugger::From(GetThread()->GetIsolate()))
    debugger->ExceptionThrown(thread_, error_event);
}

}  // namespace blink
