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

#include "third_party/blink/renderer/core/inspector/worker_inspector_controller.h"

#include "third_party/blink/renderer/core/CoreProbeSink.h"
#include "third_party/blink/renderer/core/inspector/inspector_log_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_network_agent.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/core/inspector/inspector_worker_agent.h"
#include "third_party/blink/renderer/core/inspector/protocol/Protocol.h"
#include "third_party/blink/renderer/core/inspector/worker_thread_debugger.h"
#include "third_party/blink/renderer/core/loader/worker_fetch_context.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/workers/worker_backing_thread.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_reporting_proxy.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/web_thread_supporting_gc.h"

namespace blink {

WorkerInspectorController* WorkerInspectorController::Create(
    WorkerThread* thread) {
  WorkerThreadDebugger* debugger =
      WorkerThreadDebugger::From(thread->GetIsolate());
  return debugger ? new WorkerInspectorController(thread, debugger) : nullptr;
}

WorkerInspectorController::WorkerInspectorController(
    WorkerThread* thread,
    WorkerThreadDebugger* debugger)
    : debugger_(debugger), thread_(thread), probe_sink_(new CoreProbeSink()) {
  probe_sink_->addInspectorTraceEvents(new InspectorTraceEvents());
}

WorkerInspectorController::~WorkerInspectorController() {
  DCHECK(!thread_);
}

void WorkerInspectorController::ConnectFrontend(int session_id) {
  if (sessions_.find(session_id) != sessions_.end())
    return;

  InspectorSession* session = new InspectorSession(
      this, probe_sink_.Get(), session_id, debugger_->GetV8Inspector(),
      debugger_->ContextGroupId(thread_), String());
  session->Append(new InspectorLogAgent(thread_->GetConsoleMessageStorage(),
                                        nullptr, session->V8Session()));
  if (thread_->GlobalScope()->IsWorkerGlobalScope()) {
    InspectedFrames* inspected_frames = new InspectedFrames(nullptr);
    WorkerGlobalScope* worker_global_scope =
        ToWorkerGlobalScope(thread_->GlobalScope());
    DCHECK(worker_global_scope->EnsureFetcher());
    session->Append(new InspectorNetworkAgent(
        inspected_frames, worker_global_scope, session->V8Session()));
    session->Append(
        new InspectorWorkerAgent(inspected_frames, worker_global_scope));
  }
  if (sessions_.IsEmpty())
    thread_->GetWorkerBackingThread().BackingThread().AddTaskObserver(this);
  sessions_.insert(session_id, session);
}

void WorkerInspectorController::DisconnectFrontend(int session_id) {
  auto it = sessions_.find(session_id);
  if (it == sessions_.end())
    return;
  it->value->Dispose();
  sessions_.erase(it);
  if (sessions_.IsEmpty())
    thread_->GetWorkerBackingThread().BackingThread().RemoveTaskObserver(this);
}

void WorkerInspectorController::DispatchMessageFromFrontend(
    int session_id,
    const String& message) {
  auto it = sessions_.find(session_id);
  if (it == sessions_.end())
    return;
  it->value->DispatchProtocolMessage(message);
}

void WorkerInspectorController::Dispose() {
  Vector<int> ids;
  CopyKeysToVector(sessions_, ids);
  for (int session_id : ids)
    DisconnectFrontend(session_id);
  thread_ = nullptr;
}

void WorkerInspectorController::FlushProtocolNotifications() {
  for (auto& it : sessions_)
    it.value->flushProtocolNotifications();
}

void WorkerInspectorController::SendProtocolResponse(int session_id,
                                                     int call_id,
                                                     const String& response,
                                                     const String& state) {
  // Make tests more predictable by flushing all sessions before sending
  // protocol response in any of them.
  if (LayoutTestSupport::IsRunningLayoutTest())
    FlushProtocolNotifications();
  // Worker messages are wrapped, no need to handle callId or state.
  thread_->GetWorkerReportingProxy().PostMessageToPageInspector(session_id,
                                                                response);
}

void WorkerInspectorController::SendProtocolNotification(int session_id,
                                                         const String& message,
                                                         const String& state) {
  thread_->GetWorkerReportingProxy().PostMessageToPageInspector(session_id,
                                                                message);
}

void WorkerInspectorController::WillProcessTask() {}

void WorkerInspectorController::DidProcessTask() {
  for (auto& it : sessions_)
    it.value->flushProtocolNotifications();
}

void WorkerInspectorController::Trace(blink::Visitor* visitor) {
  visitor->Trace(probe_sink_);
  visitor->Trace(sessions_);
}

}  // namespace blink
