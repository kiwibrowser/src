// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/callback_function.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off

#ifndef V8AnyCallbackFunctionOptionalAnyArg_h
#define V8AnyCallbackFunctionOptionalAnyArg_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/callback_function_base.h"

namespace blink {

class ScriptWrappable;

class CORE_EXPORT V8AnyCallbackFunctionOptionalAnyArg final : public CallbackFunctionBase {
 public:
  static V8AnyCallbackFunctionOptionalAnyArg* Create(v8::Local<v8::Function> callback_function) {
    return new V8AnyCallbackFunctionOptionalAnyArg(callback_function);
  }

  ~V8AnyCallbackFunctionOptionalAnyArg() override = default;

  // Performs "invoke".
  // https://heycam.github.io/webidl/#es-invoking-callback-functions
  v8::Maybe<ScriptValue> Invoke(ScriptWrappable* callback_this_value, ScriptValue optionalAnyArg) WARN_UNUSED_RESULT;

 private:
  explicit V8AnyCallbackFunctionOptionalAnyArg(v8::Local<v8::Function> callback_function)
      : CallbackFunctionBase(callback_function) {}
};

template <>
class CORE_TEMPLATE_CLASS_EXPORT V8PersistentCallbackFunction<V8AnyCallbackFunctionOptionalAnyArg> final : public V8PersistentCallbackFunctionBase {
  using V8CallbackFunction = V8AnyCallbackFunctionOptionalAnyArg;

 public:
  ~V8PersistentCallbackFunction() override = default;

  // Returns a wrapper-tracing version of this callback function.
  V8CallbackFunction* ToNonV8Persistent() { return Proxy(); }

  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<ScriptValue> Invoke(ScriptWrappable* callback_this_value, ScriptValue optionalAnyArg) WARN_UNUSED_RESULT;

 private:
  explicit V8PersistentCallbackFunction(V8CallbackFunction* callback_function)
      : V8PersistentCallbackFunctionBase(callback_function) {}

  V8CallbackFunction* Proxy() {
    return As<V8CallbackFunction>();
  }

  template <typename V8CallbackFunction>
  friend V8PersistentCallbackFunction<V8CallbackFunction>*
  ToV8PersistentCallbackFunction(V8CallbackFunction*);
};

// V8AnyCallbackFunctionOptionalAnyArg is designed to be used with wrapper-tracing.
// As blink::Persistent does not perform wrapper-tracing, use of
// |WrapPersistent| for callback functions is likely (if not always) misuse.
// Thus, this code prohibits such a use case. The call sites should explicitly
// use WrapPersistent(V8PersistentCallbackFunction<T>*).
Persistent<V8AnyCallbackFunctionOptionalAnyArg> WrapPersistent(V8AnyCallbackFunctionOptionalAnyArg*) = delete;

}  // namespace blink

#endif  // V8AnyCallbackFunctionOptionalAnyArg_h
