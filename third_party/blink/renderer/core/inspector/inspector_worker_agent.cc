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

#include "third_party/blink/renderer/core/inspector/inspector_worker_agent.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/workers/execution_context_worker_registry.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

using protocol::Maybe;
using protocol::Response;

namespace WorkerAgentState {
static const char kAutoAttach[] = "autoAttach";
static const char kWaitForDebuggerOnStart[] = "waitForDebuggerOnStart";
static const char kAttachedSessionIds[] = "attachedSessionIds";
};  // namespace WorkerAgentState

int InspectorWorkerAgent::s_last_connection_ = 0;

InspectorWorkerAgent::InspectorWorkerAgent(
    InspectedFrames* inspected_frames,
    WorkerGlobalScope* worker_global_scope)
    : inspected_frames_(inspected_frames),
      worker_global_scope_(worker_global_scope) {}

InspectorWorkerAgent::~InspectorWorkerAgent() = default;

void InspectorWorkerAgent::Restore() {
  if (!AutoAttachEnabled())
    return;
  instrumenting_agents_->addInspectorWorkerAgent(this);
  protocol::DictionaryValue* attached = AttachedSessionIds();
  for (size_t i = 0; i < attached->size(); ++i)
    GetFrontend()->detachedFromTarget(attached->at(i).first);
  state_->remove(WorkerAgentState::kAttachedSessionIds);
  ConnectToAllProxies();
}

Response InspectorWorkerAgent::disable() {
  if (AutoAttachEnabled()) {
    DisconnectFromAllProxies(false);
    instrumenting_agents_->removeInspectorWorkerAgent(this);
  }
  state_->setBoolean(WorkerAgentState::kAutoAttach, false);
  state_->setBoolean(WorkerAgentState::kWaitForDebuggerOnStart, false);
  state_->remove(WorkerAgentState::kAttachedSessionIds);
  return Response::OK();
}

Response InspectorWorkerAgent::setAutoAttach(bool auto_attach,
                                             bool wait_for_debugger_on_start) {
  state_->setBoolean(WorkerAgentState::kWaitForDebuggerOnStart,
                     wait_for_debugger_on_start);

  if (auto_attach == AutoAttachEnabled())
    return Response::OK();
  state_->setBoolean(WorkerAgentState::kAutoAttach, auto_attach);
  if (auto_attach) {
    instrumenting_agents_->addInspectorWorkerAgent(this);
    ConnectToAllProxies();
  } else {
    DisconnectFromAllProxies(true);
    instrumenting_agents_->removeInspectorWorkerAgent(this);
  }
  return Response::OK();
}

bool InspectorWorkerAgent::AutoAttachEnabled() {
  return state_->booleanProperty(WorkerAgentState::kAutoAttach, false);
}

Response InspectorWorkerAgent::sendMessageToTarget(const String& message,
                                                   Maybe<String> session_id,
                                                   Maybe<String> target_id) {
  if (session_id.isJust()) {
    auto it = session_id_to_connection_.find(session_id.fromJust());
    if (it == session_id_to_connection_.end())
      return Response::Error("No session with given id");
    WorkerInspectorProxy* proxy = connected_proxies_.at(it->value);
    proxy->SendMessageToInspector(it->value, message);
    return Response::OK();
  }
  if (target_id.isJust()) {
    int connection = 0;
    for (auto& it : connected_proxies_) {
      if (it.value->InspectorId() == target_id.fromJust()) {
        if (connection)
          return Response::Error("Multiple sessions attached, specify id");
        connection = it.key;
      }
    }
    if (!connection)
      return Response::Error("No target with given id");
    WorkerInspectorProxy* proxy = connected_proxies_.at(connection);
    proxy->SendMessageToInspector(connection, message);
    return Response::OK();
  }
  return Response::Error("Session id must be specified");
}

void InspectorWorkerAgent::ShouldWaitForDebuggerOnWorkerStart(bool* result) {
  if (AutoAttachEnabled() &&
      state_->booleanProperty(WorkerAgentState::kWaitForDebuggerOnStart, false))
    *result = true;
}

void InspectorWorkerAgent::DidStartWorker(WorkerInspectorProxy* proxy,
                                          bool waiting_for_debugger) {
  DCHECK(GetFrontend() && AutoAttachEnabled());
  ConnectToProxy(proxy, waiting_for_debugger);
}

void InspectorWorkerAgent::WorkerTerminated(WorkerInspectorProxy* proxy) {
  DCHECK(GetFrontend() && AutoAttachEnabled());
  Vector<String> session_ids;
  for (auto& it : session_id_to_connection_) {
    if (connected_proxies_.at(it.value) == proxy)
      session_ids.push_back(it.key);
  }
  for (const String& session_id : session_ids) {
    AttachedSessionIds()->remove(session_id);
    GetFrontend()->detachedFromTarget(session_id, proxy->InspectorId());
    int connection = session_id_to_connection_.at(session_id);
    proxy->DisconnectFromInspector(connection, this);
    connected_proxies_.erase(connection);
    connection_to_session_id_.erase(connection);
    session_id_to_connection_.erase(session_id);
  }
}

void InspectorWorkerAgent::ConnectToAllProxies() {
  if (worker_global_scope_) {
    for (WorkerInspectorProxy* proxy :
         ExecutionContextWorkerRegistry::From(*worker_global_scope_)
             ->GetWorkerInspectorProxies()) {
      ConnectToProxy(proxy, false);
    }
    return;
  }

  for (LocalFrame* frame : *inspected_frames_) {
    for (WorkerInspectorProxy* proxy :
         ExecutionContextWorkerRegistry::From(*frame->GetDocument())
             ->GetWorkerInspectorProxies()) {
      ConnectToProxy(proxy, false);
    }
  }
}

void InspectorWorkerAgent::DisconnectFromAllProxies(bool report_to_frontend) {
  for (auto& it : session_id_to_connection_) {
    WorkerInspectorProxy* proxy = connected_proxies_.at(it.value);
    if (report_to_frontend) {
      AttachedSessionIds()->remove(it.key);
      GetFrontend()->detachedFromTarget(it.key, proxy->InspectorId());
    }
    proxy->DisconnectFromInspector(it.value, this);
  }
  connection_to_session_id_.clear();
  session_id_to_connection_.clear();
  connected_proxies_.clear();
}

void InspectorWorkerAgent::DidCommitLoadForLocalFrame(LocalFrame* frame) {
  if (!AutoAttachEnabled() || frame != inspected_frames_->Root())
    return;

  // During navigation workers from old page may die after a while.
  // Usually, it's fine to report them terminated later, but some tests
  // expect strict set of workers, and we reuse renderer between tests.
  DisconnectFromAllProxies(true);
}

protocol::DictionaryValue* InspectorWorkerAgent::AttachedSessionIds() {
  protocol::DictionaryValue* ids =
      state_->getObject(WorkerAgentState::kAttachedSessionIds);
  if (!ids) {
    std::unique_ptr<protocol::DictionaryValue> new_ids =
        protocol::DictionaryValue::create();
    ids = new_ids.get();
    state_->setObject(WorkerAgentState::kAttachedSessionIds,
                      std::move(new_ids));
  }
  return ids;
}

void InspectorWorkerAgent::ConnectToProxy(WorkerInspectorProxy* proxy,
                                          bool waiting_for_debugger) {
  int connection = ++s_last_connection_;
  connected_proxies_.Set(connection, proxy);

  String session_id = proxy->InspectorId() + "-" + String::Number(connection);
  session_id_to_connection_.Set(session_id, connection);
  connection_to_session_id_.Set(connection, session_id);

  proxy->ConnectToInspector(connection, this);
  DCHECK(GetFrontend());
  AttachedSessionIds()->setBoolean(session_id, true);
  GetFrontend()->attachedToTarget(session_id,
                                  protocol::Target::TargetInfo::create()
                                      .setTargetId(proxy->InspectorId())
                                      .setType("worker")
                                      .setTitle(proxy->Url())
                                      .setUrl(proxy->Url())
                                      .setAttached(true)
                                      .build(),
                                  waiting_for_debugger);
}

void InspectorWorkerAgent::DispatchMessageFromWorker(
    WorkerInspectorProxy* proxy,
    int connection,
    const String& message) {
  auto it = connection_to_session_id_.find(connection);
  if (it == connection_to_session_id_.end())
    return;
  GetFrontend()->receivedMessageFromTarget(it->value, message,
                                           proxy->InspectorId());
}

void InspectorWorkerAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(connected_proxies_);
  visitor->Trace(inspected_frames_);
  visitor->Trace(worker_global_scope_);
  InspectorBaseAgent::Trace(visitor);
}

}  // namespace blink
