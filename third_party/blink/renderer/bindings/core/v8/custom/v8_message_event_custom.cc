/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/bindings/core/v8/v8_message_event.h"

#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_array_buffer.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_blob.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_event_target.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_message_port.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_window.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"

namespace blink {

void V8MessageEvent::dataAttributeGetterCustom(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  auto private_cached_data =
      V8PrivateProperty::GetMessageEventCachedData(isolate);
  v8::Local<v8::Value> cached_data;
  if (private_cached_data.GetOrUndefined(info.Holder()).ToLocal(&cached_data) &&
      !cached_data->IsUndefined()) {
    V8SetReturnValue(info, cached_data);
    return;
  }

  MessageEvent* event = V8MessageEvent::ToImpl(info.Holder());

  v8::Local<v8::Value> result;
  switch (event->GetDataType()) {
    case MessageEvent::kDataTypeScriptValue:
      result =
          event->DataAsScriptValue().V8ValueFor(ScriptState::Current(isolate));
      if (result.IsEmpty())
        result = v8::Null(isolate);
      break;

    case MessageEvent::kDataTypeSerializedScriptValue:
      if (UnpackedSerializedScriptValue* unpacked_value =
              event->DataAsUnpackedSerializedScriptValue()) {
        MessagePortArray ports = event->ports();
        SerializedScriptValue::DeserializeOptions options;
        options.message_ports = &ports;
        result = unpacked_value->Deserialize(isolate, options);
      } else {
        result = v8::Null(isolate);
      }
      break;

    case MessageEvent::kDataTypeString:
      result = V8String(isolate, event->DataAsString());
      break;

    case MessageEvent::kDataTypeBlob:
      result = ToV8(event->DataAsBlob(), info.Holder(), isolate);
      break;

    case MessageEvent::kDataTypeArrayBuffer:
      result = ToV8(event->DataAsArrayBuffer(), info.Holder(), isolate);
      break;
  }

  // Store the result as a private value so this callback returns the cached
  // result in future invocations.
  private_cached_data.Set(info.Holder(), result);
  V8SetReturnValue(info, result);
}

void V8MessageEvent::initMessageEventMethodCustom(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  ExceptionState exception_state(info.GetIsolate(),
                                 ExceptionState::kExecutionContext,
                                 "MessageEvent", "initMessageEvent");
  if (UNLIKELY(info.Length() < 1)) {
    exception_state.ThrowTypeError(
        ExceptionMessages::NotEnoughArguments(1, info.Length()));
    return;
  }

  MessageEvent* event = V8MessageEvent::ToImpl(info.Holder());
  TOSTRING_VOID(V8StringResource<>, type_arg, info[0]);
  bool bubbles_arg = false;
  bool cancelable_arg = false;
  if (!info[1]
           ->BooleanValue(info.GetIsolate()->GetCurrentContext())
           .To(&bubbles_arg) ||
      !info[2]
           ->BooleanValue(info.GetIsolate()->GetCurrentContext())
           .To(&cancelable_arg))
    return;
  v8::Local<v8::Value> data_arg = info[3];
  TOSTRING_VOID(V8StringResource<>, origin_arg, info[4]);
  TOSTRING_VOID(V8StringResource<>, last_event_id_arg, info[5]);
  EventTarget* source_arg =
      V8EventTarget::ToImplWithTypeCheck(info.GetIsolate(), info[6]);
  MessagePortArray* port_array = nullptr;
  const int kPortArrayIndex = 7;
  if (!IsUndefinedOrNull(info[kPortArrayIndex])) {
    port_array = new MessagePortArray;
    *port_array = NativeValueTraits<IDLSequence<MessagePort>>::NativeValue(
        info.GetIsolate(), info[kPortArrayIndex], exception_state);
    if (exception_state.HadException())
      return;
  }
  event->initMessageEvent(
      type_arg, bubbles_arg, cancelable_arg,
      ScriptValue(ScriptState::Current(info.GetIsolate()), data_arg),
      origin_arg, last_event_id_arg, source_arg, port_array);
}

}  // namespace blink
