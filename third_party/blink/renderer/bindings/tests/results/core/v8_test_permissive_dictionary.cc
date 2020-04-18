// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/dictionary_v8.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "third_party/blink/renderer/bindings/tests/results/core/v8_test_permissive_dictionary.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"

namespace blink {

static const v8::Eternal<v8::Name>* eternalV8TestPermissiveDictionaryKeys(v8::Isolate* isolate) {
  static const char* const kKeys[] = {
    "booleanMember",
  };
  return V8PerIsolateData::From(isolate)->FindOrCreateEternalNameCache(
      kKeys, kKeys, arraysize(kKeys));
}

void V8TestPermissiveDictionary::ToImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, TestPermissiveDictionary& impl, ExceptionState& exceptionState) {
  if (IsUndefinedOrNull(v8Value)) {
    return;
  }
  if (!v8Value->IsObject()) {
    // Do nothing.
    return;
  }
  v8::Local<v8::Object> v8Object = v8Value.As<v8::Object>();
  ALLOW_UNUSED_LOCAL(v8Object);

  const v8::Eternal<v8::Name>* keys = eternalV8TestPermissiveDictionaryKeys(isolate);
  v8::TryCatch block(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> booleanMemberValue;
  if (!v8Object->Get(context, keys[0].Get(isolate)).ToLocal(&booleanMemberValue)) {
    exceptionState.RethrowV8Exception(block.Exception());
    return;
  }
  if (booleanMemberValue.IsEmpty() || booleanMemberValue->IsUndefined()) {
    // Do nothing.
  } else {
    bool booleanMemberCppValue = NativeValueTraits<IDLBoolean>::NativeValue(isolate, booleanMemberValue, exceptionState);
    if (exceptionState.HadException())
      return;
    impl.setBooleanMember(booleanMemberCppValue);
  }
}

v8::Local<v8::Value> TestPermissiveDictionary::ToV8Impl(v8::Local<v8::Object> creationContext, v8::Isolate* isolate) const {
  v8::Local<v8::Object> v8Object = v8::Object::New(isolate);
  if (!toV8TestPermissiveDictionary(*this, v8Object, creationContext, isolate))
    return v8::Undefined(isolate);
  return v8Object;
}

bool toV8TestPermissiveDictionary(const TestPermissiveDictionary& impl, v8::Local<v8::Object> dictionary, v8::Local<v8::Object> creationContext, v8::Isolate* isolate) {
  const v8::Eternal<v8::Name>* keys = eternalV8TestPermissiveDictionaryKeys(isolate);
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::Local<v8::Value> booleanMemberValue;
  bool booleanMemberHasValueOrDefault = false;
  if (impl.hasBooleanMember()) {
    booleanMemberValue = v8::Boolean::New(isolate, impl.booleanMember());
    booleanMemberHasValueOrDefault = true;
  }
  if (booleanMemberHasValueOrDefault &&
      !V8CallBoolean(dictionary->CreateDataProperty(context, keys[0].Get(isolate), booleanMemberValue))) {
    return false;
  }

  return true;
}

TestPermissiveDictionary NativeValueTraits<TestPermissiveDictionary>::NativeValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exceptionState) {
  TestPermissiveDictionary impl;
  V8TestPermissiveDictionary::ToImpl(isolate, value, impl, exceptionState);
  return impl;
}

}  // namespace blink
