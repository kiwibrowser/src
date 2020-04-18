// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/websockets/inspector_websocket_events.h"

#include <memory>
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/identifiers_factory.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

std::unique_ptr<TracedValue> InspectorWebSocketCreateEvent::Data(
    ExecutionContext* execution_context,
    unsigned long identifier,
    const KURL& url,
    const String& protocol) {
  DCHECK(execution_context->IsContextThread());
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("identifier", identifier);
  value->SetString("url", url.GetString());
  if (execution_context->IsDocument()) {
    value->SetString("frame", IdentifiersFactory::FrameId(
                                  ToDocument(execution_context)->GetFrame()));
  } else if (execution_context->IsWorkerGlobalScope()) {
    value->SetString("workerId", IdentifiersFactory::IdFromToken(
                                     ToWorkerGlobalScope(execution_context)
                                         ->GetThread()
                                         ->GetDevToolsWorkerToken()));
  } else {
    NOTREACHED()
        << "WebSocket is available only in Document and WorkerGlobalScope";
  }
  if (!protocol.IsNull())
    value->SetString("webSocketProtocol", protocol);
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorWebSocketEvent::Data(
    ExecutionContext* execution_context,
    unsigned long identifier) {
  DCHECK(execution_context->IsContextThread());
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("identifier", identifier);
  if (execution_context->IsDocument()) {
    value->SetString("frame", IdentifiersFactory::FrameId(
                                  ToDocument(execution_context)->GetFrame()));
  } else if (execution_context->IsWorkerGlobalScope()) {
    value->SetString("workerId", IdentifiersFactory::IdFromToken(
                                     ToWorkerGlobalScope(execution_context)
                                         ->GetThread()
                                         ->GetDevToolsWorkerToken()));
  } else {
    NOTREACHED()
        << "WebSocket is available only in Document and WorkerGlobalScope";
  }
  SetCallStack(value.get());
  return value;
}

}  // namespace blink
