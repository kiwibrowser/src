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

#include "third_party/blink/renderer/core/messaging/message_port.h"

#include <memory>

#include "mojo/public/cpp/base/big_buffer_mojom_traits.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value_factory.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/inspector/thread_debugger.h"
#include "third_party/blink/renderer/core/messaging/blink_transferable_message_struct_traits.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/wtf/atomics.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

// The maximum number of MessageEvents to dispatch from one task.
static const int kMaximumMessagesPerTask = 200;

MessagePort* MessagePort::Create(ExecutionContext& execution_context) {
  return new MessagePort(execution_context);
}

MessagePort::MessagePort(ExecutionContext& execution_context)
    : ContextLifecycleObserver(&execution_context),
      task_runner_(execution_context.GetTaskRunner(TaskType::kPostedMessage)) {}

MessagePort::~MessagePort() {
  DCHECK(!started_ || !IsEntangled());
}

void MessagePort::postMessage(ScriptState* script_state,
                              scoped_refptr<SerializedScriptValue> message,
                              const MessagePortArray& ports,
                              ExceptionState& exception_state) {
  if (!IsEntangled())
    return;
  DCHECK(GetExecutionContext());
  DCHECK(!IsNeutered());

  BlinkTransferableMessage msg;
  msg.message = message;

  // Make sure we aren't connected to any of the passed-in ports.
  for (unsigned i = 0; i < ports.size(); ++i) {
    if (ports[i] == this) {
      exception_state.ThrowDOMException(
          kDataCloneError,
          "Port at index " + String::Number(i) + " contains the source port.");
      return;
    }
  }
  msg.ports = MessagePort::DisentanglePorts(
      ExecutionContext::From(script_state), ports, exception_state);
  if (exception_state.HadException())
    return;

  ThreadDebugger* debugger = ThreadDebugger::From(script_state->GetIsolate());
  if (debugger)
    msg.sender_stack_trace_id = debugger->StoreCurrentStackTrace("postMessage");

  mojo::Message mojo_message =
      mojom::blink::TransferableMessage::WrapAsMessage(std::move(msg));
  connector_->Accept(&mojo_message);
}

MessagePortChannel MessagePort::Disentangle() {
  DCHECK(!IsNeutered());
  auto result = MessagePortChannel(connector_->PassMessagePipe());
  connector_ = nullptr;
  return result;
}

void MessagePort::start() {
  // Do nothing if we've been cloned or closed.
  if (!IsEntangled())
    return;

  DCHECK(GetExecutionContext());
  if (started_)
    return;

  started_ = true;
  connector_->ResumeIncomingMethodCallProcessing();
}

void MessagePort::close() {
  if (closed_)
    return;
  // A closed port should not be neutered, so rather than merely disconnecting
  // from the mojo message pipe, also entangle with a new dangling message pipe.
  if (!IsNeutered()) {
    connector_ = nullptr;
    Entangle(mojo::MessagePipe().handle0);
  }
  closed_ = true;
}

void MessagePort::Entangle(mojo::ScopedMessagePipeHandle handle) {
  // Only invoked to set our initial entanglement.
  DCHECK(handle.is_valid());
  DCHECK(!connector_);
  DCHECK(GetExecutionContext());
  connector_ = std::make_unique<mojo::Connector>(
      std::move(handle), mojo::Connector::SINGLE_THREADED_SEND, task_runner_);
  connector_->PauseIncomingMethodCallProcessing();
  connector_->set_incoming_receiver(this);
  connector_->set_connection_error_handler(
      WTF::Bind(&MessagePort::close, WrapWeakPersistent(this)));
}

void MessagePort::Entangle(MessagePortChannel channel) {
  Entangle(channel.ReleaseHandle());
}

const AtomicString& MessagePort::InterfaceName() const {
  return EventTargetNames::MessagePort;
}

bool MessagePort::HasPendingActivity() const {
  // The spec says that entangled message ports should always be treated as if
  // they have a strong reference.
  // We'll also stipulate that the queue needs to be open (if the app drops its
  // reference to the port before start()-ing it, then it's not really entangled
  // as it's unreachable).
  return started_ && IsEntangled();
}

Vector<MessagePortChannel> MessagePort::DisentanglePorts(
    ExecutionContext* context,
    const MessagePortArray& ports,
    ExceptionState& exception_state) {
  if (!ports.size())
    return Vector<MessagePortChannel>();

  HeapHashSet<Member<MessagePort>> visited;
  bool has_closed_ports = false;

  // Walk the incoming array - if there are any duplicate ports, or null ports
  // or cloned ports, throw an error (per section 8.3.3 of the HTML5 spec).
  for (unsigned i = 0; i < ports.size(); ++i) {
    MessagePort* port = ports[i];
    if (!port || port->IsNeutered() || visited.Contains(port)) {
      String type;
      if (!port)
        type = "null";
      else if (port->IsNeutered())
        type = "already neutered";
      else
        type = "a duplicate";
      exception_state.ThrowDOMException(
          kDataCloneError,
          "Port at index " + String::Number(i) + " is " + type + ".");
      return Vector<MessagePortChannel>();
    }
    if (port->closed_)
      has_closed_ports = true;
    visited.insert(port);
  }

  UseCounter::Count(context, WebFeature::kMessagePortsTransferred);
  if (has_closed_ports)
    UseCounter::Count(context, WebFeature::kMessagePortTransferClosedPort);

  // Passed-in ports passed validity checks, so we can disentangle them.
  Vector<MessagePortChannel> channels;
  channels.ReserveInitialCapacity(ports.size());
  for (unsigned i = 0; i < ports.size(); ++i)
    channels.push_back(ports[i]->Disentangle());
  return channels;
}

MessagePortArray* MessagePort::EntanglePorts(
    ExecutionContext& context,
    Vector<MessagePortChannel> channels) {
  return EntanglePorts(context,
                       WebVector<MessagePortChannel>(std::move(channels)));
}

MessagePortArray* MessagePort::EntanglePorts(
    ExecutionContext& context,
    WebVector<MessagePortChannel> channels) {
  // https://html.spec.whatwg.org/multipage/comms.html#message-ports
  // |ports| should be an empty array, not null even when there is no ports.
  MessagePortArray* port_array = new MessagePortArray(channels.size());
  for (unsigned i = 0; i < channels.size(); ++i) {
    MessagePort* port = MessagePort::Create(context);
    port->Entangle(std::move(channels[i]));
    (*port_array)[i] = port;
  }
  return port_array;
}

MojoHandle MessagePort::EntangledHandleForTesting() const {
  return connector_->handle().value();
}

void MessagePort::Trace(blink::Visitor* visitor) {
  ContextLifecycleObserver::Trace(visitor);
  EventTargetWithInlineData::Trace(visitor);
}

bool MessagePort::Accept(mojo::Message* mojo_message) {
  // Connector repeatedly calls Accept as long as any messages are available. To
  // avoid completely starving the event loop and give some time for other tasks
  // the connector is temporarily paused after |kMaximumMessagesPerTask| have
  // been received without other tasks having had a chance to run (in particular
  // the ResetMessageCount task posted here).
  if (messages_in_current_task_ == 0) {
    task_runner_->PostTask(FROM_HERE, WTF::Bind(&MessagePort::ResetMessageCount,
                                                WrapWeakPersistent(this)));
  }
  ++messages_in_current_task_;
  if (messages_in_current_task_ > kMaximumMessagesPerTask) {
    connector_->PauseIncomingMethodCallProcessing();
  }

  BlinkTransferableMessage message;
  if (!mojom::blink::TransferableMessage::DeserializeFromMessage(
          std::move(*mojo_message), &message)) {
    return false;
  }

  // WorkerGlobalScope::close() in Worker onmessage handler should prevent
  // the next message from dispatching.
  if (GetExecutionContext()->IsWorkerGlobalScope() &&
      ToWorkerGlobalScope(GetExecutionContext())->IsClosing()) {
    return true;
  }

  MessagePortArray* ports = MessagePort::EntanglePorts(
      *GetExecutionContext(), std::move(message.ports));
  Event* evt = MessageEvent::Create(ports, std::move(message.message));

  v8::Isolate* isolate = ToIsolate(GetExecutionContext());
  ThreadDebugger* debugger = ThreadDebugger::From(isolate);
  if (debugger)
    debugger->ExternalAsyncTaskStarted(message.sender_stack_trace_id);
  DispatchEvent(evt);
  if (debugger)
    debugger->ExternalAsyncTaskFinished(message.sender_stack_trace_id);
  return true;
}

void MessagePort::ResetMessageCount() {
  DCHECK_GT(messages_in_current_task_, 0);
  messages_in_current_task_ = 0;
  // No-op if not paused already.
  if (connector_)
    connector_->ResumeIncomingMethodCallProcessing();
}

}  // namespace blink
