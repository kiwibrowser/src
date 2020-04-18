// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/serviceworkers/extendable_message_event.h"

namespace blink {

ExtendableMessageEvent* ExtendableMessageEvent::Create(
    const AtomicString& type,
    const ExtendableMessageEventInit& initializer) {
  return new ExtendableMessageEvent(type, initializer);
}

ExtendableMessageEvent* ExtendableMessageEvent::Create(
    const AtomicString& type,
    const ExtendableMessageEventInit& initializer,
    WaitUntilObserver* observer) {
  return new ExtendableMessageEvent(type, initializer, observer);
}

ExtendableMessageEvent* ExtendableMessageEvent::Create(
    scoped_refptr<SerializedScriptValue> data,
    const String& origin,
    MessagePortArray* ports,
    WaitUntilObserver* observer) {
  return new ExtendableMessageEvent(std::move(data), origin, ports, observer);
}

ExtendableMessageEvent* ExtendableMessageEvent::Create(
    scoped_refptr<SerializedScriptValue> data,
    const String& origin,
    MessagePortArray* ports,
    ServiceWorkerClient* source,
    WaitUntilObserver* observer) {
  ExtendableMessageEvent* event =
      new ExtendableMessageEvent(std::move(data), origin, ports, observer);
  event->source_as_client_ = source;
  return event;
}

ExtendableMessageEvent* ExtendableMessageEvent::Create(
    scoped_refptr<SerializedScriptValue> data,
    const String& origin,
    MessagePortArray* ports,
    ServiceWorker* source,
    WaitUntilObserver* observer) {
  ExtendableMessageEvent* event =
      new ExtendableMessageEvent(std::move(data), origin, ports, observer);
  event->source_as_service_worker_ = source;
  return event;
}

MessagePortArray ExtendableMessageEvent::ports() const {
  // TODO(bashi): Currently we return a copied array because the binding
  // layer could modify the content of the array while executing JS callbacks.
  // Avoid copying once we can make sure that the binding layer won't
  // modify the content.
  if (ports_) {
    return *ports_;
  }
  return MessagePortArray();
}

void ExtendableMessageEvent::source(
    ClientOrServiceWorkerOrMessagePort& result) const {
  if (source_as_client_)
    result = ClientOrServiceWorkerOrMessagePort::FromClient(source_as_client_);
  else if (source_as_service_worker_)
    result = ClientOrServiceWorkerOrMessagePort::FromServiceWorker(
        source_as_service_worker_);
  else if (source_as_message_port_)
    result = ClientOrServiceWorkerOrMessagePort::FromMessagePort(
        source_as_message_port_);
  else
    result = ClientOrServiceWorkerOrMessagePort();
}

const AtomicString& ExtendableMessageEvent::InterfaceName() const {
  return EventNames::ExtendableMessageEvent;
}

void ExtendableMessageEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(source_as_client_);
  visitor->Trace(source_as_service_worker_);
  visitor->Trace(source_as_message_port_);
  visitor->Trace(ports_);
  ExtendableEvent::Trace(visitor);
}

ExtendableMessageEvent::ExtendableMessageEvent(
    const AtomicString& type,
    const ExtendableMessageEventInit& initializer)
    : ExtendableMessageEvent(type, initializer, nullptr) {}

ExtendableMessageEvent::ExtendableMessageEvent(
    const AtomicString& type,
    const ExtendableMessageEventInit& initializer,
    WaitUntilObserver* observer)
    : ExtendableEvent(type, initializer, observer) {
  if (initializer.hasOrigin())
    origin_ = initializer.origin();
  if (initializer.hasLastEventId())
    last_event_id_ = initializer.lastEventId();
  if (initializer.hasSource()) {
    if (initializer.source().IsClient())
      source_as_client_ = initializer.source().GetAsClient();
    else if (initializer.source().IsServiceWorker())
      source_as_service_worker_ = initializer.source().GetAsServiceWorker();
    else if (initializer.source().IsMessagePort())
      source_as_message_port_ = initializer.source().GetAsMessagePort();
  }
  if (initializer.hasPorts())
    ports_ = new MessagePortArray(initializer.ports());
}

ExtendableMessageEvent::ExtendableMessageEvent(
    scoped_refptr<SerializedScriptValue> data,
    const String& origin,
    MessagePortArray* ports,
    WaitUntilObserver* observer)
    : ExtendableEvent(EventTypeNames::message,
                      ExtendableMessageEventInit(),
                      observer),
      serialized_data_(std::move(data)),
      origin_(origin),
      last_event_id_(String()),
      ports_(ports) {
  if (serialized_data_)
    serialized_data_->RegisterMemoryAllocatedWithCurrentScriptContext();
}

}  // namespace blink
