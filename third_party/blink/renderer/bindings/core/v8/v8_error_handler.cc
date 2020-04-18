/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/bindings/core/v8/v8_error_handler.h"

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_error_event.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"

namespace blink {

V8ErrorHandler::V8ErrorHandler(bool is_inline, ScriptState* script_state)
    : V8EventListener(is_inline, script_state) {}

v8::Local<v8::Value> V8ErrorHandler::CallListenerFunction(
    ScriptState* script_state,
    v8::Local<v8::Value> js_event,
    Event* event) {
  DCHECK(!js_event.IsEmpty());
  if (!event->HasInterface(EventNames::ErrorEvent))
    return V8EventListener::CallListenerFunction(script_state, js_event, event);

  ErrorEvent* error_event = static_cast<ErrorEvent*>(event);
  if (error_event->World() && error_event->World() != &World())
    return v8::Null(GetIsolate());

  v8::Local<v8::Context> context = script_state->GetContext();
  ExecutionContext* execution_context = ToExecutionContext(context);
  v8::Local<v8::Object> listener = GetListenerObject(execution_context);
  if (listener.IsEmpty() || !listener->IsFunction())
    return v8::Null(GetIsolate());

  v8::Local<v8::Function> call_function =
      v8::Local<v8::Function>::Cast(listener);
  v8::Local<v8::Object> this_value = context->Global();

  v8::Local<v8::Object> event_object;
  if (!js_event->ToObject(context).ToLocal(&event_object))
    return v8::Null(GetIsolate());
  auto private_error = V8PrivateProperty::GetErrorEventError(GetIsolate());
  v8::Local<v8::Value> error;
  if (!private_error.GetOrUndefined(event_object).ToLocal(&error) ||
      error->IsUndefined())
    error = v8::Null(GetIsolate());

  v8::Local<v8::Value> parameters[5] = {
      V8String(GetIsolate(), error_event->message()),
      V8String(GetIsolate(), error_event->filename()),
      v8::Integer::New(GetIsolate(), error_event->lineno()),
      v8::Integer::New(GetIsolate(), error_event->colno()), error};
  v8::TryCatch try_catch(GetIsolate());
  try_catch.SetVerbose(true);

  v8::MaybeLocal<v8::Value> result = V8ScriptRunner::CallFunction(
      call_function, execution_context, this_value, arraysize(parameters),
      parameters, GetIsolate());
  v8::Local<v8::Value> return_value;
  if (!result.ToLocal(&return_value))
    return v8::Null(GetIsolate());

  return return_value;
}

// static
void V8ErrorHandler::StoreExceptionOnErrorEventWrapper(
    ScriptState* script_state,
    ErrorEvent* event,
    v8::Local<v8::Value> data,
    v8::Local<v8::Object> creation_context) {
  v8::Local<v8::Value> wrapped_event =
      ToV8(event, creation_context, script_state->GetIsolate());
  if (wrapped_event.IsEmpty())
    return;

  DCHECK(wrapped_event->IsObject());
  auto private_error =
      V8PrivateProperty::GetErrorEventError(script_state->GetIsolate());
  private_error.Set(wrapped_event.As<v8::Object>(), data);
}

// static
v8::Local<v8::Value> V8ErrorHandler::LoadExceptionFromErrorEventWrapper(
    ScriptState* script_state,
    ErrorEvent* event,
    v8::Local<v8::Object> creation_context) {
  v8::Local<v8::Value> wrapped_event =
      ToV8(event, creation_context, script_state->GetIsolate());
  if (wrapped_event.IsEmpty() || !wrapped_event->IsObject())
    return v8::Local<v8::Value>();

  DCHECK(wrapped_event->IsObject());
  auto private_error =
      V8PrivateProperty::GetErrorEventError(script_state->GetIsolate());
  v8::Local<v8::Value> error;
  if (!private_error.GetOrUndefined(wrapped_event.As<v8::Object>())
           .ToLocal(&error) ||
      error->IsUndefined()) {
    return v8::Local<v8::Value>();
  }
  return error;
}

bool V8ErrorHandler::ShouldPreventDefault(v8::Local<v8::Value> return_value) {
  return return_value->IsBoolean() && return_value.As<v8::Boolean>()->Value();
}

}  // namespace blink
