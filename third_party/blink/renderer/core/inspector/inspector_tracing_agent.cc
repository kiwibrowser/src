// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/inspector_tracing_agent.h"

#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/inspector/inspected_frames.h"
#include "third_party/blink/renderer/core/inspector/inspector_trace_events.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

using protocol::Maybe;
using protocol::Response;

namespace TracingAgentState {
const char kSessionId[] = "sessionId";
}

namespace {
const char kDevtoolsMetadataEventCategory[] =
    TRACE_DISABLED_BY_DEFAULT("devtools.timeline");
}

InspectorTracingAgent::InspectorTracingAgent(InspectedFrames* inspected_frames)
    : inspected_frames_(inspected_frames) {}

InspectorTracingAgent::~InspectorTracingAgent() {}

void InspectorTracingAgent::Trace(blink::Visitor* visitor) {
  visitor->Trace(inspected_frames_);
  InspectorBaseAgent::Trace(visitor);
}

void InspectorTracingAgent::Restore() {
  state_->getString(TracingAgentState::kSessionId, &session_id_);
  if (IsStarted())
    EmitMetadataEvents();
}

void InspectorTracingAgent::start(Maybe<String> categories,
                                  Maybe<String> options,
                                  Maybe<double> buffer_usage_reporting_interval,
                                  Maybe<String> transfer_mode,
                                  Maybe<String> transfer_compression,
                                  Maybe<protocol::Tracing::TraceConfig> config,
                                  std::unique_ptr<StartCallback> callback) {
  DCHECK(!IsStarted());
  if (config.isJust()) {
    callback->sendFailure(Response::Error(
        "Using trace config on renderer targets is not supported yet."));
    return;
  }

  session_id_ = IdentifiersFactory::CreateIdentifier();
  state_->setString(TracingAgentState::kSessionId, session_id_);

  // Tracing is already started by DevTools TracingHandler::Start for the
  // renderer target in the browser process. It will eventually start tracing
  // in the renderer process via IPC. But we still need a redundant enable here
  // for EmitMetadataEvents, at which point we are not sure if tracing
  // is already started in the renderer process.
  TraceEvent::EnableTracing(categories.fromMaybe(String()));

  EmitMetadataEvents();
  callback->sendSuccess();
}

void InspectorTracingAgent::end(std::unique_ptr<EndCallback> callback) {
  TraceEvent::DisableTracing();
  InnerDisable();
  callback->sendSuccess();
}

bool InspectorTracingAgent::IsStarted() const {
  return !session_id_.IsEmpty();
}

void InspectorTracingAgent::EmitMetadataEvents() {
  TRACE_EVENT_INSTANT1(kDevtoolsMetadataEventCategory, "TracingStartedInPage",
                       TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorTracingStartedInFrame::Data(
                           session_id_, inspected_frames_->Root()));
}

Response InspectorTracingAgent::disable() {
  InnerDisable();
  return Response::OK();
}

void InspectorTracingAgent::InnerDisable() {
  state_->remove(TracingAgentState::kSessionId);
  session_id_ = String();
}

}  // namespace blink
