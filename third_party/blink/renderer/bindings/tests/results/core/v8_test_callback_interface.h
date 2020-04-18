// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/callback_interface.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off

#ifndef V8TestCallbackInterface_h
#define V8TestCallbackInterface_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/callback_interface_base.h"

namespace blink {

class TestInterfaceEmpty;

class CORE_EXPORT V8TestCallbackInterface final : public CallbackInterfaceBase {
 public:
  static V8TestCallbackInterface* Create(v8::Local<v8::Object> callback_object) {
    return new V8TestCallbackInterface(callback_object);
  }

  ~V8TestCallbackInterface() override = default;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethod(ScriptWrappable* callback_this_value) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<bool> booleanMethod(ScriptWrappable* callback_this_value) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethodBooleanArg(ScriptWrappable* callback_this_value, bool boolArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethodSequenceArg(ScriptWrappable* callback_this_value, const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethodFloatArg(ScriptWrappable* callback_this_value, float floatArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> voidMethodTestInterfaceEmptyStringArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> callbackWithThisValueVoidMethodStringArg(ScriptWrappable* callback_this_value, const String& stringArg) WARN_UNUSED_RESULT;

  // Performs "call a user object's operation".
  // https://heycam.github.io/webidl/#call-a-user-objects-operation
  v8::Maybe<void> customVoidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) WARN_UNUSED_RESULT;

 private:
  explicit V8TestCallbackInterface(v8::Local<v8::Object> callback_object)
      : CallbackInterfaceBase(callback_object, kNotSingleOperation) {}
};

template <>
class CORE_TEMPLATE_CLASS_EXPORT V8PersistentCallbackInterface<V8TestCallbackInterface> final : public V8PersistentCallbackInterfaceBase {
  using V8CallbackInterface = V8TestCallbackInterface;

 public:
  ~V8PersistentCallbackInterface() override = default;

  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethod(ScriptWrappable* callback_this_value) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<bool> booleanMethod(ScriptWrappable* callback_this_value) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethodBooleanArg(ScriptWrappable* callback_this_value, bool boolArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethodSequenceArg(ScriptWrappable* callback_this_value, const HeapVector<Member<TestInterfaceEmpty>>& sequenceArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethodFloatArg(ScriptWrappable* callback_this_value, float floatArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> voidMethodTestInterfaceEmptyStringArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg, const String& stringArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> callbackWithThisValueVoidMethodStringArg(ScriptWrappable* callback_this_value, const String& stringArg) WARN_UNUSED_RESULT;
  CORE_EXTERN_TEMPLATE_EXPORT
  v8::Maybe<void> customVoidMethodTestInterfaceEmptyArg(ScriptWrappable* callback_this_value, TestInterfaceEmpty* testInterfaceEmptyArg) WARN_UNUSED_RESULT;

 private:
  explicit V8PersistentCallbackInterface(V8CallbackInterface* callback_interface)
      : V8PersistentCallbackInterfaceBase(callback_interface) {}

  V8CallbackInterface* Proxy() {
    return As<V8CallbackInterface>();
  }

  template <typename V8CallbackInterface>
  friend V8PersistentCallbackInterface<V8CallbackInterface>*
  ToV8PersistentCallbackInterface(V8CallbackInterface*);
};

// V8TestCallbackInterface is designed to be used with wrapper-tracing.
// As blink::Persistent does not perform wrapper-tracing, use of
// |WrapPersistent| for callback interfaces is likely (if not always) misuse.
// Thus, this code prohibits such a use case. The call sites should explicitly
// use WrapPersistent(V8PersistentCallbackInterface<T>*).
Persistent<V8TestCallbackInterface> WrapPersistent(V8TestCallbackInterface*) = delete;

}  // namespace blink

#endif  // V8TestCallbackInterface_h
