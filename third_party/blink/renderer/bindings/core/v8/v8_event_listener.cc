/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/bindings/core/v8/v8_event_listener.h"

#include "third_party/blink/renderer/bindings/core/v8/script_controller.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

V8EventListener::V8EventListener(bool is_attribute, ScriptState* script_state)
    : V8AbstractEventListener(script_state->GetIsolate(),
                              is_attribute,
                              script_state->World()) {}

v8::Local<v8::Function> V8EventListener::GetListenerFunction(
    ScriptState* script_state) {
  v8::Local<v8::Object> listener =
      GetListenerObject(ExecutionContext::From(script_state));

  // Has the listener been disposed?
  if (listener.IsEmpty())
    return v8::Local<v8::Function>();

  if (listener->IsFunction())
    return v8::Local<v8::Function>::Cast(listener);

  // The EventHandler callback function type (used for event handler
  // attributes in HTML) has [TreatNonObjectAsNull], which implies that
  // non-function objects should be treated as no-op functions that return
  // undefined.
  if (IsAttribute())
    return v8::Local<v8::Function>();

  // Getting the handleEvent property can runs script in the getter.
  if (ScriptForbiddenScope::IsScriptForbidden()) {
    V8ThrowException::ThrowError(GetIsolate(),
                                 "Script execution is forbidden.");
    return v8::Local<v8::Function>();
  }

  if (listener->IsObject()) {
    // Check that no exceptions were thrown when getting the
    // handleEvent property and that the value is a function.
    v8::Local<v8::Value> property;
    if (listener
            ->Get(script_state->GetContext(),
                  V8AtomicString(GetIsolate(), "handleEvent"))
            .ToLocal(&property) &&
        property->IsFunction())
      return v8::Local<v8::Function>::Cast(property);
  }

  return v8::Local<v8::Function>();
}

v8::Local<v8::Value> V8EventListener::CallListenerFunction(
    ScriptState* script_state,
    v8::Local<v8::Value> js_event,
    Event* event) {
  DCHECK(!js_event.IsEmpty());
  v8::Local<v8::Function> handler_function = GetListenerFunction(script_state);
  v8::Local<v8::Object> receiver = GetReceiverObject(script_state, event);
  if (handler_function.IsEmpty() || receiver.IsEmpty())
    return v8::Local<v8::Value>();

  ExecutionContext* execution_context =
      ToExecutionContext(script_state->GetContext());
  if (!execution_context->IsDocument())
    return v8::Local<v8::Value>();

  LocalFrame* frame = ToDocument(execution_context)->GetFrame();
  if (!frame)
    return v8::Local<v8::Value>();

  // TODO(jochen): Consider moving this check into canExecuteScripts.
  // http://crbug.com/608641
  if (script_state->World().IsMainWorld() &&
      !execution_context->CanExecuteScripts(kAboutToExecuteScript))
    return v8::Local<v8::Value>();

  v8::Local<v8::Value> parameters[1] = {js_event};
  v8::Local<v8::Value> result;
  if (!V8ScriptRunner::CallFunction(handler_function, frame->GetDocument(),
                                    receiver, arraysize(parameters), parameters,
                                    script_state->GetIsolate())
           .ToLocal(&result))
    return v8::Local<v8::Value>();
  return result;
}

}  // namespace blink
