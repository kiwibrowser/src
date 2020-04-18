// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/workers/main_thread_worklet_global_scope.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/worker_or_worklet_script_controller.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/deprecation.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/global_scope_creation_params.h"

namespace blink {

MainThreadWorkletGlobalScope::MainThreadWorkletGlobalScope(
    LocalFrame* frame,
    std::unique_ptr<GlobalScopeCreationParams> creation_params,
    WorkerReportingProxy& reporting_proxy)
    : WorkletGlobalScope(
          std::move(creation_params),
          ToIsolate(frame),
          reporting_proxy,
          // Specify |kInternalLoading| because these task runners are used
          // during module loading and this usage is not explicitly spec'ed.
          frame->GetFrameScheduler()->GetTaskRunner(TaskType::kInternalLoading),
          frame->GetFrameScheduler()->GetTaskRunner(
              TaskType::kInternalLoading)),
      ContextClient(frame) {}

MainThreadWorkletGlobalScope::~MainThreadWorkletGlobalScope() = default;

WorkerThread* MainThreadWorkletGlobalScope::GetThread() const {
  NOTREACHED();
  return nullptr;
}

scoped_refptr<base::SingleThreadTaskRunner>
MainThreadWorkletGlobalScope::GetTaskRunner(TaskType type) {
  DCHECK(IsContextThread());
  // MainThreadWorkletGlobalScope lives on the main thread and its GetThread()
  // doesn't return a valid worker thread. Instead, retrieve a task runner
  // from the frame.
  return GetFrame()->GetFrameScheduler()->GetTaskRunner(type);
}

// TODO(nhiroki): Add tests for termination.
void MainThreadWorkletGlobalScope::Terminate() {
  Dispose();
}

void MainThreadWorkletGlobalScope::AddConsoleMessage(
    ConsoleMessage* console_message) {
  GetFrame()->Console().AddMessage(console_message);
}

void MainThreadWorkletGlobalScope::ExceptionThrown(ErrorEvent* event) {
  MainThreadDebugger::Instance()->ExceptionThrown(this, event);
}

CoreProbeSink* MainThreadWorkletGlobalScope::GetProbeSink() {
  return probe::ToCoreProbeSink(GetFrame());
}

void MainThreadWorkletGlobalScope::Trace(blink::Visitor* visitor) {
  WorkletGlobalScope::Trace(visitor);
  ContextClient::Trace(visitor);
}

}  // namespace blink
