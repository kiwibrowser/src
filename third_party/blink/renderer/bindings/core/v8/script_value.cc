/*
 * Copyright (C) 2008, 2009, 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/bindings/core/v8/script_value.h"

#include "third_party/blink/renderer/bindings/core/v8/serialization/serialized_script_value_factory.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"

namespace blink {

v8::Local<v8::Value> ScriptValue::V8Value() const {
  if (IsEmpty())
    return v8::Local<v8::Value>();

  DCHECK(GetIsolate()->InContext());

  // This is a check to validate that you don't return a ScriptValue to a world
  // different from the world that created the ScriptValue.
  // Probably this could be:
  //   if (&script_state_->world() == &DOMWrapperWorld::current(isolate()))
  //       return v8::Local<v8::Value>();
  // instead of triggering CHECK.
  CHECK_EQ(&script_state_->World(), &DOMWrapperWorld::Current(GetIsolate()));
  return value_->NewLocal(GetIsolate());
}

v8::Local<v8::Value> ScriptValue::V8ValueFor(
    ScriptState* target_script_state) const {
  if (IsEmpty())
    return v8::Local<v8::Value>();
  v8::Isolate* isolate = target_script_state->GetIsolate();
  if (&script_state_->World() == &target_script_state->World())
    return value_->NewLocal(isolate);

  DCHECK(isolate->InContext());
  v8::Local<v8::Value> value = value_->NewLocal(isolate);
  scoped_refptr<SerializedScriptValue> serialized =
      SerializedScriptValue::SerializeAndSwallowExceptions(isolate, value);
  return serialized->Deserialize(isolate);
}

bool ScriptValue::ToString(String& result) const {
  if (IsEmpty())
    return false;

  ScriptState::Scope scope(script_state_.get());
  v8::Local<v8::Value> string = V8Value();
  if (string.IsEmpty() || !string->IsString())
    return false;
  result = ToCoreString(v8::Local<v8::String>::Cast(string));
  return true;
}

ScriptValue ScriptValue::CreateNull(ScriptState* script_state) {
  return ScriptValue(script_state, v8::Null(script_state->GetIsolate()));
}

}  // namespace blink
