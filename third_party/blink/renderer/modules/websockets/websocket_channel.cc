/*
 * Copyright (C) 2009, 2012 Google Inc.  All rights reserved.
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

#include "third_party/blink/renderer/modules/websockets/websocket_channel.h"

#include <memory>
#include "third_party/blink/renderer/bindings/core/v8/source_location.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/core/workers/worker_thread.h"
#include "third_party/blink/renderer/modules/websockets/websocket_channel_client.h"
#include "third_party/blink/renderer/modules/websockets/websocket_channel_impl.h"
#include "third_party/blink/renderer/modules/websockets/worker_websocket_channel.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

WebSocketChannel* WebSocketChannel::Create(ExecutionContext* context,
                                           WebSocketChannelClient* client) {
  DCHECK(context);
  DCHECK(client);

  std::unique_ptr<SourceLocation> location = SourceLocation::Capture(context);

  // When the off-main-thread WebSocket flag is enabled, web workers use
  // WebSocketChannelImpl as opposed to WorkerWebSocketChannel. See
  // https://crbug.com/825740
  if (context->IsDocument() ||
      RuntimeEnabledFeatures::OffMainThreadWebSocketEnabled()) {
    return WebSocketChannelImpl::Create(context, client, std::move(location));
  }

  WorkerGlobalScope* worker_global_scope = ToWorkerGlobalScope(context);
  return WorkerWebSocketChannel::Create(*worker_global_scope, client,
                                        std::move(location));
}

}  // namespace blink
