// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/union_container.cpp.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#include "third_party/blink/renderer/bindings/tests/results/core/test_interface_garbage_collected_or_string.h"

#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/to_v8_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_test_interface_garbage_collected.h"

namespace blink {

TestInterfaceGarbageCollectedOrString::TestInterfaceGarbageCollectedOrString() : type_(SpecificType::kNone) {}

const String& TestInterfaceGarbageCollectedOrString::GetAsString() const {
  DCHECK(IsString());
  return string_;
}

void TestInterfaceGarbageCollectedOrString::SetString(const String& value) {
  DCHECK(IsNull());
  string_ = value;
  type_ = SpecificType::kString;
}

TestInterfaceGarbageCollectedOrString TestInterfaceGarbageCollectedOrString::FromString(const String& value) {
  TestInterfaceGarbageCollectedOrString container;
  container.SetString(value);
  return container;
}

TestInterfaceGarbageCollected* TestInterfaceGarbageCollectedOrString::GetAsTestInterfaceGarbageCollected() const {
  DCHECK(IsTestInterfaceGarbageCollected());
  return test_interface_garbage_collected_;
}

void TestInterfaceGarbageCollectedOrString::SetTestInterfaceGarbageCollected(TestInterfaceGarbageCollected* value) {
  DCHECK(IsNull());
  test_interface_garbage_collected_ = value;
  type_ = SpecificType::kTestInterfaceGarbageCollected;
}

TestInterfaceGarbageCollectedOrString TestInterfaceGarbageCollectedOrString::FromTestInterfaceGarbageCollected(TestInterfaceGarbageCollected* value) {
  TestInterfaceGarbageCollectedOrString container;
  container.SetTestInterfaceGarbageCollected(value);
  return container;
}

TestInterfaceGarbageCollectedOrString::TestInterfaceGarbageCollectedOrString(const TestInterfaceGarbageCollectedOrString&) = default;
TestInterfaceGarbageCollectedOrString::~TestInterfaceGarbageCollectedOrString() = default;
TestInterfaceGarbageCollectedOrString& TestInterfaceGarbageCollectedOrString::operator=(const TestInterfaceGarbageCollectedOrString&) = default;

void TestInterfaceGarbageCollectedOrString::Trace(blink::Visitor* visitor) {
  visitor->Trace(test_interface_garbage_collected_);
}

void V8TestInterfaceGarbageCollectedOrString::ToImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, TestInterfaceGarbageCollectedOrString& impl, UnionTypeConversionMode conversionMode, ExceptionState& exceptionState) {
  if (v8Value.IsEmpty())
    return;

  if (conversionMode == UnionTypeConversionMode::kNullable && IsUndefinedOrNull(v8Value))
    return;

  if (V8TestInterfaceGarbageCollected::hasInstance(v8Value, isolate)) {
    TestInterfaceGarbageCollected* cppValue = V8TestInterfaceGarbageCollected::ToImpl(v8::Local<v8::Object>::Cast(v8Value));
    impl.SetTestInterfaceGarbageCollected(cppValue);
    return;
  }

  {
    V8StringResource<> cppValue = v8Value;
    if (!cppValue.Prepare(exceptionState))
      return;
    impl.SetString(cppValue);
    return;
  }
}

v8::Local<v8::Value> ToV8(const TestInterfaceGarbageCollectedOrString& impl, v8::Local<v8::Object> creationContext, v8::Isolate* isolate) {
  switch (impl.type_) {
    case TestInterfaceGarbageCollectedOrString::SpecificType::kNone:
      return v8::Null(isolate);
    case TestInterfaceGarbageCollectedOrString::SpecificType::kString:
      return V8String(isolate, impl.GetAsString());
    case TestInterfaceGarbageCollectedOrString::SpecificType::kTestInterfaceGarbageCollected:
      return ToV8(impl.GetAsTestInterfaceGarbageCollected(), creationContext, isolate);
    default:
      NOTREACHED();
  }
  return v8::Local<v8::Value>();
}

TestInterfaceGarbageCollectedOrString NativeValueTraits<TestInterfaceGarbageCollectedOrString>::NativeValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exceptionState) {
  TestInterfaceGarbageCollectedOrString impl;
  V8TestInterfaceGarbageCollectedOrString::ToImpl(isolate, value, impl, UnionTypeConversionMode::kNotNullable, exceptionState);
  return impl;
}

}  // namespace blink
