/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/probe/core_probes.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/CoreProbeSink.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/inspector/inspector_css_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_dom_debugger_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_network_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_page_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_session.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/inspector/main_thread_debugger.h"
#include "third_party/blink/renderer/core/inspector/thread_debugger.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_info.h"

namespace blink {
namespace probe {

namespace {
void* AsyncId(void* task) {
  // Blink uses odd ids for network requests and even ids for everything else.
  // We should make all of them even before reporting to V8 to avoid collisions
  // with internal V8 async events.
  return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(task) << 1);
}
}  // namespace

AsyncTask::AsyncTask(ExecutionContext* context,
                     void* task,
                     const char* step,
                     bool enabled)
    : debugger_(enabled ? ThreadDebugger::From(ToIsolate(context)) : nullptr),
      task_(AsyncId(task)),
      recurring_(step) {
  if (recurring_) {
    TRACE_EVENT_FLOW_STEP0("devtools.timeline.async", "AsyncTask",
                           TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)),
                           step ? step : "");
  } else {
    TRACE_EVENT_FLOW_END0("devtools.timeline.async", "AsyncTask",
                          TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)));
  }
  if (debugger_)
    debugger_->AsyncTaskStarted(task_);
}

AsyncTask::~AsyncTask() {
  if (debugger_) {
    debugger_->AsyncTaskFinished(task_);
    if (!recurring_)
      debugger_->AsyncTaskCanceled(task_);
  }
}

void AsyncTaskScheduled(ExecutionContext* context,
                        const StringView& name,
                        void* task) {
  TRACE_EVENT_FLOW_BEGIN1("devtools.timeline.async", "AsyncTask",
                          TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)),
                          "data", InspectorAsyncTask::Data(name));
  if (ThreadDebugger* debugger = ThreadDebugger::From(ToIsolate(context)))
    debugger->AsyncTaskScheduled(name, AsyncId(task), true);
}

void AsyncTaskScheduledBreakable(ExecutionContext* context,
                                 const char* name,
                                 void* task) {
  AsyncTaskScheduled(context, name, task);
  breakableLocation(context, name);
}

void AsyncTaskCanceled(ExecutionContext* context, void* task) {
  AsyncTaskCanceled(ToIsolate(context), task);
}

void AsyncTaskCanceled(v8::Isolate* isolate, void* task) {
  if (ThreadDebugger* debugger = ThreadDebugger::From(isolate))
    debugger->AsyncTaskCanceled(AsyncId(task));
  TRACE_EVENT_FLOW_END0("devtools.timeline.async", "AsyncTask",
                        TRACE_ID_LOCAL(reinterpret_cast<uintptr_t>(task)));
}

void AsyncTaskCanceledBreakable(ExecutionContext* context,
                                const char* name,
                                void* task) {
  AsyncTaskCanceled(context, task);
  breakableLocation(context, name);
}

void AllAsyncTasksCanceled(ExecutionContext* context) {
  if (ThreadDebugger* debugger = ThreadDebugger::From(ToIsolate(context)))
    debugger->AllAsyncTasksCanceled();
}

void DidReceiveResourceResponseButCanceled(LocalFrame* frame,
                                           DocumentLoader* loader,
                                           unsigned long identifier,
                                           const ResourceResponse& r,
                                           Resource* resource) {
  didReceiveResourceResponse(frame->GetDocument(), identifier, loader, r,
                             resource);
}

void CanceledAfterReceivedResourceResponse(LocalFrame* frame,
                                           DocumentLoader* loader,
                                           unsigned long identifier,
                                           const ResourceResponse& r,
                                           Resource* resource) {
  DidReceiveResourceResponseButCanceled(frame, loader, identifier, r, resource);
}

void ContinueWithPolicyIgnore(LocalFrame* frame,
                              DocumentLoader* loader,
                              unsigned long identifier,
                              const ResourceResponse& r,
                              Resource* resource) {
  DidReceiveResourceResponseButCanceled(frame, loader, identifier, r, resource);
}

}  // namespace probe
}  // namespace blink
