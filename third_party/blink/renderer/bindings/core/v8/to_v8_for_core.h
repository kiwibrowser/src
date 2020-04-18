// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_FOR_CORE_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_FOR_CORE_H_

// ToV8() provides C++ -> V8 conversion. Note that ToV8() can return an empty
// handle. Call sites must check IsEmpty() before using return value.

#include "third_party/blink/renderer/bindings/core/v8/idl_dictionary_base.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_testing.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/typed_arrays/array_buffer_view_helpers.h"
#include "third_party/blink/renderer/platform/bindings/to_v8.h"
#include "v8/include/v8.h"

namespace blink {

class Dictionary;
class DOMWindow;
class EventTarget;

CORE_EXPORT v8::Local<v8::Value> ToV8(DOMWindow*,
                                      v8::Local<v8::Object> creation_context,
                                      v8::Isolate*);
CORE_EXPORT v8::Local<v8::Value> ToV8(EventTarget*,
                                      v8::Local<v8::Object> creation_context,
                                      v8::Isolate*);
inline v8::Local<v8::Value> ToV8(Node* node,
                                 v8::Local<v8::Object> creation_context,
                                 v8::Isolate* isolate) {
  return ToV8(static_cast<ScriptWrappable*>(node), creation_context, isolate);
}

inline v8::Local<v8::Value> ToV8(const Dictionary& value,
                                 v8::Local<v8::Object> creation_context,
                                 v8::Isolate* isolate) {
  NOTREACHED();
  return v8::Undefined(isolate);
}

template <typename T>
inline v8::Local<v8::Value> ToV8(NotShared<T> value,
                                 v8::Local<v8::Object> creation_context,
                                 v8::Isolate* isolate) {
  return ToV8(value.View(), creation_context, isolate);
}

// Dictionary

inline v8::Local<v8::Value> ToV8(const IDLDictionaryBase& value,
                                 v8::Local<v8::Object> creation_context,
                                 v8::Isolate* isolate) {
  return value.ToV8Impl(creation_context, isolate);
}

// ScriptValue

inline v8::Local<v8::Value> ToV8(const ScriptValue& value,
                                 v8::Local<v8::Object> creation_context,
                                 v8::Isolate* isolate) {
  if (value.IsEmpty())
    return v8::Undefined(isolate);
  return value.V8Value();
}

// Cannot define in ScriptValue because of the circular dependency between toV8
// and ScriptValue
template <typename T>
inline ScriptValue ScriptValue::From(ScriptState* script_state, T&& value) {
  return ScriptValue(script_state, ToV8(std::forward<T>(value), script_state));
}

template <typename T>
v8::Local<v8::Value> ToV8(V8TestingScope* scope, T value) {
  return blink::ToV8(value, scope->GetContext()->Global(), scope->GetIsolate());
}

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_TO_V8_FOR_CORE_H_
