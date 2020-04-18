// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/broadcastchannel/broadcast_channel.h"

#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value.h"
#include "third_party/blink/renderer/core/dom/events/event_queue.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/platform/mojo/mojo_helper.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

// To ensure proper ordering of messages send to/from multiple BroadcastChannel
// instances in the same thread this uses one BroadcastChannelService
// connection as basis for all connections to channels from the same thread. The
// actual connections used to send/receive messages are then created using
// associated interfaces, ensuring proper message ordering.
mojom::blink::BroadcastChannelProviderPtr& GetThreadSpecificProvider() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      ThreadSpecific<mojom::blink::BroadcastChannelProviderPtr>, provider, ());
  if (!provider.IsSet()) {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&*provider));
  }
  return *provider;
}

}  // namespace

// static
BroadcastChannel* BroadcastChannel::Create(ExecutionContext* execution_context,
                                           const String& name,
                                           ExceptionState& exception_state) {
  if (execution_context->GetSecurityOrigin()->IsUnique()) {
    // TODO(mek): Decide what to do here depending on
    // https://github.com/whatwg/html/issues/1319
    exception_state.ThrowDOMException(
        kNotSupportedError,
        "Can't create BroadcastChannel in an opaque origin");
    return nullptr;
  }
  return new BroadcastChannel(execution_context, name);
}

BroadcastChannel::~BroadcastChannel() = default;

void BroadcastChannel::Dispose() {
  close();
}

void BroadcastChannel::postMessage(const ScriptValue& message,
                                   ExceptionState& exception_state) {
  if (!binding_.is_bound()) {
    exception_state.ThrowDOMException(kInvalidStateError, "Channel is closed");
    return;
  }
  scoped_refptr<SerializedScriptValue> value = SerializedScriptValue::Serialize(
      message.GetIsolate(), message.V8Value(),
      SerializedScriptValue::SerializeOptions(), exception_state);
  if (exception_state.HadException())
    return;

  BlinkCloneableMessage msg;
  msg.message = std::move(value);
  remote_client_->OnMessage(std::move(msg));
}

void BroadcastChannel::close() {
  remote_client_.reset();
  if (binding_.is_bound())
    binding_.Close();
}

const AtomicString& BroadcastChannel::InterfaceName() const {
  return EventTargetNames::BroadcastChannel;
}

bool BroadcastChannel::HasPendingActivity() const {
  return binding_.is_bound() && HasEventListeners(EventTypeNames::message);
}

void BroadcastChannel::ContextDestroyed(ExecutionContext*) {
  close();
}

void BroadcastChannel::Trace(blink::Visitor* visitor) {
  ContextLifecycleObserver::Trace(visitor);
  EventTargetWithInlineData::Trace(visitor);
}

void BroadcastChannel::OnMessage(BlinkCloneableMessage message) {
  // Queue a task to dispatch the event.
  MessageEvent* event = MessageEvent::Create(
      nullptr, std::move(message.message),
      GetExecutionContext()->GetSecurityOrigin()->ToString());
  event->SetTarget(this);
  bool success =
      GetExecutionContext()->GetEventQueue()->EnqueueEvent(FROM_HERE, event);
  DCHECK(success);
  ALLOW_UNUSED_LOCAL(success);
}

void BroadcastChannel::OnError() {
  close();
}

BroadcastChannel::BroadcastChannel(ExecutionContext* execution_context,
                                   const String& name)
    : ContextLifecycleObserver(execution_context),
      origin_(execution_context->GetSecurityOrigin()),
      name_(name),
      binding_(this) {
  mojom::blink::BroadcastChannelProviderPtr& provider =
      GetThreadSpecificProvider();

  // Local BroadcastChannelClient for messages send from the browser to this
  // channel.
  mojom::blink::BroadcastChannelClientAssociatedPtrInfo local_client_info;
  binding_.Bind(mojo::MakeRequest(&local_client_info));
  binding_.set_connection_error_handler(
      WTF::Bind(&BroadcastChannel::OnError, WrapWeakPersistent(this)));

  // Remote BroadcastChannelClient for messages send from this channel to the
  // browser.
  auto remote_cient_request = mojo::MakeRequest(&remote_client_);
  remote_client_.set_connection_error_handler(
      WTF::Bind(&BroadcastChannel::OnError, WrapWeakPersistent(this)));

  provider->ConnectToChannel(origin_, name_, std::move(local_client_info),
                             std::move(remote_cient_request));
}

}  // namespace blink
