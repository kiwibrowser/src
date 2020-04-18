/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/modules/serviceworkers/service_worker.h"

#include <memory>
#include "third_party/blink/public/mojom/service_worker/service_worker_state.mojom-blink.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/bindings/core/v8/callback_promise_adapter.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/messaging/blink_transferable_message.h"
#include "third_party/blink/renderer/core/messaging/message_port.h"
#include "third_party/blink/renderer/modules/event_target_modules.h"
#include "third_party/blink/renderer/modules/serviceworkers/service_worker_container_client.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

const AtomicString& ServiceWorker::InterfaceName() const {
  return EventTargetNames::ServiceWorker;
}

void ServiceWorker::postMessage(ScriptState* script_state,
                                scoped_refptr<SerializedScriptValue> message,
                                const MessagePortArray& ports,
                                ExceptionState& exception_state) {
  ServiceWorkerContainerClient* client =
      ServiceWorkerContainerClient::From(GetExecutionContext());
  if (!client || !client->Provider()) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "Failed to post a message: No associated provider is available.");
    return;
  }

  BlinkTransferableMessage msg;
  msg.message = message;
  msg.ports = MessagePort::DisentanglePorts(
      ExecutionContext::From(script_state), ports, exception_state);
  if (exception_state.HadException())
    return;

  if (handle_->ServiceWorker()->GetState() ==
      mojom::blink::ServiceWorkerState::kRedundant) {
    exception_state.ThrowDOMException(kInvalidStateError,
                                      "ServiceWorker is in redundant state.");
    return;
  }

  handle_->ServiceWorker()->PostMessageToServiceWorker(
      ToTransferableMessage(std::move(msg)));
}

ScriptPromise ServiceWorker::InternalsTerminate(ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  handle_->ServiceWorker()->TerminateForTesting(
      std::make_unique<CallbackPromiseAdapter<void, void>>(resolver));
  return promise;
}

void ServiceWorker::DispatchStateChangeEvent() {
  this->DispatchEvent(Event::Create(EventTypeNames::statechange));
}

String ServiceWorker::scriptURL() const {
  return handle_->ServiceWorker()->Url().GetString();
}

String ServiceWorker::state() const {
  switch (handle_->ServiceWorker()->GetState()) {
    case mojom::blink::ServiceWorkerState::kUnknown:
      // The web platform should never see this internal state
      NOTREACHED();
      return "unknown";
    case mojom::blink::ServiceWorkerState::kInstalling:
      return "installing";
    case mojom::blink::ServiceWorkerState::kInstalled:
      return "installed";
    case mojom::blink::ServiceWorkerState::kActivating:
      return "activating";
    case mojom::blink::ServiceWorkerState::kActivated:
      return "activated";
    case mojom::blink::ServiceWorkerState::kRedundant:
      return "redundant";
  }
  NOTREACHED();
  return g_null_atom;
}

ServiceWorker* ServiceWorker::From(
    ExecutionContext* execution_context,
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  return GetOrCreate(execution_context, std::move(handle));
}

bool ServiceWorker::HasPendingActivity() const {
  if (was_stopped_)
    return false;
  return handle_->ServiceWorker()->GetState() !=
         mojom::blink::ServiceWorkerState::kRedundant;
}

void ServiceWorker::ContextDestroyed(ExecutionContext*) {
  was_stopped_ = true;
}

ServiceWorker* ServiceWorker::GetOrCreate(
    ExecutionContext* execution_context,
    std::unique_ptr<WebServiceWorker::Handle> handle) {
  if (!handle)
    return nullptr;

  ServiceWorker* existing_worker =
      static_cast<ServiceWorker*>(handle->ServiceWorker()->Proxy());
  if (existing_worker) {
    DCHECK_EQ(existing_worker->GetExecutionContext(), execution_context);
    return existing_worker;
  }

  return new ServiceWorker(execution_context, std::move(handle));
}

ServiceWorker::ServiceWorker(ExecutionContext* execution_context,
                             std::unique_ptr<WebServiceWorker::Handle> handle)
    : AbstractWorker(execution_context),
      handle_(std::move(handle)),
      was_stopped_(false) {
  DCHECK(handle_);
  handle_->ServiceWorker()->SetProxy(this);
}

ServiceWorker::~ServiceWorker() = default;

void ServiceWorker::Trace(blink::Visitor* visitor) {
  AbstractWorker::Trace(visitor);
}

}  // namespace blink
