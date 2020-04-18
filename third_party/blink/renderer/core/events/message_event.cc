/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/events/message_event.h"

#include <memory>
#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_array_buffer.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"

namespace blink {

static inline bool IsValidSource(EventTarget* source) {
  return !source || source->ToLocalDOMWindow() || source->ToMessagePort() ||
         source->ToServiceWorker();
}

MessageEvent::V8GCAwareString::V8GCAwareString(const String& value)
    : string_(value) {
  const int64_t size = string_.length();
  v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(size);
}

MessageEvent::V8GCAwareString::~V8GCAwareString() {
  const int64_t size = string_.length();
  v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(-size);
}

MessageEvent::V8GCAwareString& MessageEvent::V8GCAwareString::operator=(
    const String& other) {
  const int64_t old_size = string_.length();
  const int64_t new_size = other.length();
  string_ = other;
  v8::Isolate::GetCurrent()->AdjustAmountOfExternalAllocatedMemory(new_size -
                                                                   old_size);
  return *this;
}

MessageEvent::MessageEvent() : data_type_(kDataTypeScriptValue) {}

MessageEvent::MessageEvent(const AtomicString& type,
                           const MessageEventInit& initializer)
    : Event(type, initializer),
      data_type_(kDataTypeScriptValue),
      source_(nullptr) {
  if (initializer.hasData())
    data_as_script_value_ = initializer.data();
  if (initializer.hasOrigin())
    origin_ = initializer.origin();
  if (initializer.hasLastEventId())
    last_event_id_ = initializer.lastEventId();
  if (initializer.hasSource() && IsValidSource(initializer.source()))
    source_ = initializer.source();
  if (initializer.hasPorts())
    ports_ = new MessagePortArray(initializer.ports());
  DCHECK(IsValidSource(source_.Get()));
}

MessageEvent::MessageEvent(const String& origin,
                           const String& last_event_id,
                           EventTarget* source,
                           MessagePortArray* ports)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeScriptValue),
      origin_(origin),
      last_event_id_(last_event_id),
      source_(source),
      ports_(ports) {
  DCHECK(IsValidSource(source_.Get()));
}

MessageEvent::MessageEvent(scoped_refptr<SerializedScriptValue> data,
                           const String& origin,
                           const String& last_event_id,
                           EventTarget* source,
                           MessagePortArray* ports)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeSerializedScriptValue),
      data_as_serialized_script_value_(
          SerializedScriptValue::Unpack(std::move(data))),
      origin_(origin),
      last_event_id_(last_event_id),
      source_(source),
      ports_(ports) {
  DCHECK(IsValidSource(source_.Get()));
}

MessageEvent::MessageEvent(scoped_refptr<SerializedScriptValue> data,
                           const String& origin,
                           const String& last_event_id,
                           EventTarget* source,
                           Vector<MessagePortChannel> channels)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeSerializedScriptValue),
      data_as_serialized_script_value_(
          SerializedScriptValue::Unpack(std::move(data))),
      origin_(origin),
      last_event_id_(last_event_id),
      source_(source),
      channels_(std::move(channels)) {
  DCHECK(IsValidSource(source_.Get()));
}

MessageEvent::MessageEvent(const String& data, const String& origin)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeString),
      data_as_string_(data),
      origin_(origin) {}

MessageEvent::MessageEvent(Blob* data, const String& origin)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeBlob),
      data_as_blob_(data),
      origin_(origin) {}

MessageEvent::MessageEvent(DOMArrayBuffer* data, const String& origin)
    : Event(EventTypeNames::message, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeArrayBuffer),
      data_as_array_buffer_(data),
      origin_(origin) {}

MessageEvent::~MessageEvent() = default;

MessageEvent* MessageEvent::Create(const AtomicString& type,
                                   const MessageEventInit& initializer,
                                   ExceptionState& exception_state) {
  if (initializer.source() && !IsValidSource(initializer.source())) {
    exception_state.ThrowTypeError(
        "The optional 'source' property is neither a Window nor MessagePort.");
    return nullptr;
  }
  return new MessageEvent(type, initializer);
}

void MessageEvent::initMessageEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    ScriptValue data,
                                    const String& origin,
                                    const String& last_event_id,
                                    EventTarget* source,
                                    MessagePortArray* ports) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  data_type_ = kDataTypeScriptValue;
  data_as_script_value_ = data;
  origin_ = origin;
  last_event_id_ = last_event_id;
  source_ = source;
  ports_ = ports;
  is_ports_dirty_ = true;
}

void MessageEvent::initMessageEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    scoped_refptr<SerializedScriptValue> data,
                                    const String& origin,
                                    const String& last_event_id,
                                    EventTarget* source,
                                    MessagePortArray* ports) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  data_type_ = kDataTypeSerializedScriptValue;
  data_as_serialized_script_value_ =
      SerializedScriptValue::Unpack(std::move(data));
  origin_ = origin;
  last_event_id_ = last_event_id;
  source_ = source;
  ports_ = ports;
  is_ports_dirty_ = true;
}

void MessageEvent::initMessageEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    const String& data,
                                    const String& origin,
                                    const String& last_event_id,
                                    EventTarget* source,
                                    MessagePortArray* ports) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  data_type_ = kDataTypeString;
  data_as_string_ = data;
  origin_ = origin;
  last_event_id_ = last_event_id;
  source_ = source;
  ports_ = ports;
  is_ports_dirty_ = true;
}

const AtomicString& MessageEvent::InterfaceName() const {
  return EventNames::MessageEvent;
}

MessagePortArray MessageEvent::ports() {
  // TODO(bashi): Currently we return a copied array because the binding
  // layer could modify the content of the array while executing JS callbacks.
  // Avoid copying once we can make sure that the binding layer won't
  // modify the content.
  is_ports_dirty_ = false;
  return ports_ ? *ports_ : MessagePortArray();
}

void MessageEvent::EntangleMessagePorts(ExecutionContext* context) {
  ports_ = MessagePort::EntanglePorts(*context, std::move(channels_));
  is_ports_dirty_ = true;
}

void MessageEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(data_as_serialized_script_value_);
  visitor->Trace(data_as_blob_);
  visitor->Trace(data_as_array_buffer_);
  visitor->Trace(source_);
  visitor->Trace(ports_);
  Event::Trace(visitor);
}

v8::Local<v8::Object> MessageEvent::AssociateWithWrapper(
    v8::Isolate* isolate,
    const WrapperTypeInfo* wrapper_type,
    v8::Local<v8::Object> wrapper) {
  wrapper = Event::AssociateWithWrapper(isolate, wrapper_type, wrapper);

  // Ensures a wrapper is created for the data to return now so that V8 knows
  // how much memory is used via the wrapper. To keep the wrapper alive, it's
  // set to the wrapper of the MessageEvent as a private value.
  switch (GetDataType()) {
    case MessageEvent::kDataTypeScriptValue:
    case MessageEvent::kDataTypeSerializedScriptValue:
      break;
    case MessageEvent::kDataTypeString:
      V8PrivateProperty::GetMessageEventCachedData(isolate).Set(
          wrapper, V8String(isolate, DataAsString()));
      break;
    case MessageEvent::kDataTypeBlob:
      break;
    case MessageEvent::kDataTypeArrayBuffer:
      V8PrivateProperty::GetMessageEventCachedData(isolate).Set(
          wrapper, ToV8(DataAsArrayBuffer(), wrapper, isolate));
      break;
  }

  return wrapper;
}

}  // namespace blink
