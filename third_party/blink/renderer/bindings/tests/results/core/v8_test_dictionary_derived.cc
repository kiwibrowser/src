// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/dictionary_v8.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "third_party/blink/renderer/bindings/tests/results/core/v8_test_dictionary_derived.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_test_dictionary.h"

namespace blink {

static const v8::Eternal<v8::Name>* eternalV8TestDictionaryDerivedImplementedAsKeys(v8::Isolate* isolate) {
  static const char* const kKeys[] = {
    "derivedStringMember",
    "derivedStringMemberWithDefault",
    "requiredLongMember",
    "stringOrDoubleSequenceMember",
  };
  return V8PerIsolateData::From(isolate)->FindOrCreateEternalNameCache(
      kKeys, kKeys, arraysize(kKeys));
}

void V8TestDictionaryDerivedImplementedAs::ToImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, TestDictionaryDerivedImplementedAs& impl, ExceptionState& exceptionState) {
  if (IsUndefinedOrNull(v8Value)) {
    exceptionState.ThrowTypeError("Missing required member(s): requiredLongMember.");
    return;
  }
  if (!v8Value->IsObject()) {
    exceptionState.ThrowTypeError("cannot convert to dictionary.");
    return;
  }
  v8::Local<v8::Object> v8Object = v8Value.As<v8::Object>();
  ALLOW_UNUSED_LOCAL(v8Object);

  V8TestDictionary::ToImpl(isolate, v8Value, impl, exceptionState);
  if (exceptionState.HadException())
    return;

  const v8::Eternal<v8::Name>* keys = eternalV8TestDictionaryDerivedImplementedAsKeys(isolate);
  v8::TryCatch block(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> derivedStringMemberValue;
  if (!v8Object->Get(context, keys[0].Get(isolate)).ToLocal(&derivedStringMemberValue)) {
    exceptionState.RethrowV8Exception(block.Exception());
    return;
  }
  if (derivedStringMemberValue.IsEmpty() || derivedStringMemberValue->IsUndefined()) {
    // Do nothing.
  } else {
    V8StringResource<> derivedStringMemberCppValue = derivedStringMemberValue;
    if (!derivedStringMemberCppValue.Prepare(exceptionState))
      return;
    impl.setDerivedStringMember(derivedStringMemberCppValue);
  }

  v8::Local<v8::Value> derivedStringMemberWithDefaultValue;
  if (!v8Object->Get(context, keys[1].Get(isolate)).ToLocal(&derivedStringMemberWithDefaultValue)) {
    exceptionState.RethrowV8Exception(block.Exception());
    return;
  }
  if (derivedStringMemberWithDefaultValue.IsEmpty() || derivedStringMemberWithDefaultValue->IsUndefined()) {
    // Do nothing.
  } else {
    V8StringResource<> derivedStringMemberWithDefaultCppValue = derivedStringMemberWithDefaultValue;
    if (!derivedStringMemberWithDefaultCppValue.Prepare(exceptionState))
      return;
    impl.setDerivedStringMemberWithDefault(derivedStringMemberWithDefaultCppValue);
  }

  v8::Local<v8::Value> requiredLongMemberValue;
  if (!v8Object->Get(context, keys[2].Get(isolate)).ToLocal(&requiredLongMemberValue)) {
    exceptionState.RethrowV8Exception(block.Exception());
    return;
  }
  if (requiredLongMemberValue.IsEmpty() || requiredLongMemberValue->IsUndefined()) {
    exceptionState.ThrowTypeError("required member requiredLongMember is undefined.");
    return;
  } else {
    int32_t requiredLongMemberCppValue = NativeValueTraits<IDLLong>::NativeValue(isolate, requiredLongMemberValue, exceptionState, kNormalConversion);
    if (exceptionState.HadException())
      return;
    impl.setRequiredLongMember(requiredLongMemberCppValue);
  }

  v8::Local<v8::Value> stringOrDoubleSequenceMemberValue;
  if (!v8Object->Get(context, keys[3].Get(isolate)).ToLocal(&stringOrDoubleSequenceMemberValue)) {
    exceptionState.RethrowV8Exception(block.Exception());
    return;
  }
  if (stringOrDoubleSequenceMemberValue.IsEmpty() || stringOrDoubleSequenceMemberValue->IsUndefined()) {
    // Do nothing.
  } else {
    HeapVector<StringOrDouble> stringOrDoubleSequenceMemberCppValue = NativeValueTraits<IDLSequence<StringOrDouble>>::NativeValue(isolate, stringOrDoubleSequenceMemberValue, exceptionState);
    if (exceptionState.HadException())
      return;
    impl.setStringOrDoubleSequenceMember(stringOrDoubleSequenceMemberCppValue);
  }
}

v8::Local<v8::Value> TestDictionaryDerivedImplementedAs::ToV8Impl(v8::Local<v8::Object> creationContext, v8::Isolate* isolate) const {
  v8::Local<v8::Object> v8Object = v8::Object::New(isolate);
  if (!toV8TestDictionaryDerivedImplementedAs(*this, v8Object, creationContext, isolate))
    return v8::Undefined(isolate);
  return v8Object;
}

bool toV8TestDictionaryDerivedImplementedAs(const TestDictionaryDerivedImplementedAs& impl, v8::Local<v8::Object> dictionary, v8::Local<v8::Object> creationContext, v8::Isolate* isolate) {
  if (!toV8TestDictionary(impl, dictionary, creationContext, isolate))
    return false;

  const v8::Eternal<v8::Name>* keys = eternalV8TestDictionaryDerivedImplementedAsKeys(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> derivedStringMemberValue;
  bool derivedStringMemberHasValueOrDefault = false;
  if (impl.hasDerivedStringMember()) {
    derivedStringMemberValue = V8String(isolate, impl.derivedStringMember());
    derivedStringMemberHasValueOrDefault = true;
  }
  if (derivedStringMemberHasValueOrDefault &&
      !V8CallBoolean(dictionary->CreateDataProperty(context, keys[0].Get(isolate), derivedStringMemberValue))) {
    return false;
  }

  v8::Local<v8::Value> derivedStringMemberWithDefaultValue;
  bool derivedStringMemberWithDefaultHasValueOrDefault = false;
  if (impl.hasDerivedStringMemberWithDefault()) {
    derivedStringMemberWithDefaultValue = V8String(isolate, impl.derivedStringMemberWithDefault());
    derivedStringMemberWithDefaultHasValueOrDefault = true;
  } else {
    derivedStringMemberWithDefaultValue = V8String(isolate, "default string value");
    derivedStringMemberWithDefaultHasValueOrDefault = true;
  }
  if (derivedStringMemberWithDefaultHasValueOrDefault &&
      !V8CallBoolean(dictionary->CreateDataProperty(context, keys[1].Get(isolate), derivedStringMemberWithDefaultValue))) {
    return false;
  }

  v8::Local<v8::Value> requiredLongMemberValue;
  bool requiredLongMemberHasValueOrDefault = false;
  if (impl.hasRequiredLongMember()) {
    requiredLongMemberValue = v8::Integer::New(isolate, impl.requiredLongMember());
    requiredLongMemberHasValueOrDefault = true;
  } else {
    NOTREACHED();
  }
  if (requiredLongMemberHasValueOrDefault &&
      !V8CallBoolean(dictionary->CreateDataProperty(context, keys[2].Get(isolate), requiredLongMemberValue))) {
    return false;
  }

  v8::Local<v8::Value> stringOrDoubleSequenceMemberValue;
  bool stringOrDoubleSequenceMemberHasValueOrDefault = false;
  if (impl.hasStringOrDoubleSequenceMember()) {
    stringOrDoubleSequenceMemberValue = ToV8(impl.stringOrDoubleSequenceMember(), creationContext, isolate);
    stringOrDoubleSequenceMemberHasValueOrDefault = true;
  }
  if (stringOrDoubleSequenceMemberHasValueOrDefault &&
      !V8CallBoolean(dictionary->CreateDataProperty(context, keys[3].Get(isolate), stringOrDoubleSequenceMemberValue))) {
    return false;
  }

  return true;
}

TestDictionaryDerivedImplementedAs NativeValueTraits<TestDictionaryDerivedImplementedAs>::NativeValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exceptionState) {
  TestDictionaryDerivedImplementedAs impl;
  V8TestDictionaryDerivedImplementedAs::ToImpl(isolate, value, impl, exceptionState);
  return impl;
}

}  // namespace blink
