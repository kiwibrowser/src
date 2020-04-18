// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_object_parser.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

namespace blink {

bool V8ObjectParser::ParsePrototype(v8::Local<v8::Context> context,
                                    v8::Local<v8::Function> constructor,
                                    v8::Local<v8::Object>* prototype,
                                    ExceptionState* exception_state) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch block(isolate);

  v8::Local<v8::Value> prototype_value;
  if (!constructor->Get(context, V8AtomicString(isolate, "prototype"))
           .ToLocal(&prototype_value)) {
    exception_state->RethrowV8Exception(block.Exception());
    return false;
  }

  if (prototype_value->IsNullOrUndefined()) {
    exception_state->ThrowTypeError(
        "The 'prototype' object on the class does not exist.");
    return false;
  }

  if (!prototype_value->IsObject()) {
    exception_state->ThrowTypeError(
        "The 'prototype' property on the class is not an object.");
    return false;
  }

  *prototype = v8::Local<v8::Object>::Cast(prototype_value);
  return true;
}

bool V8ObjectParser::ParseFunction(v8::Local<v8::Context> context,
                                   v8::Local<v8::Object> prototype,
                                   const AtomicString function_name,
                                   v8::Local<v8::Function>* function,
                                   ExceptionState* exception_state) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch block(isolate);

  v8::Local<v8::Value> function_value;
  if (!prototype->Get(context, V8AtomicString(isolate, function_name))
           .ToLocal(&function_value)) {
    exception_state->RethrowV8Exception(block.Exception());
    return false;
  }

  if (function_value->IsNullOrUndefined()) {
    exception_state->ThrowTypeError(
        "The '" + function_name +
        "' property on the prototype does not exist.");
    return false;
  }

  if (!function_value->IsFunction()) {
    exception_state->ThrowTypeError(
        "The '" + function_name +
        "' property on the prototype is not a function.");
    return false;
  }

  *function = v8::Local<v8::Function>::Cast(function_value);
  return true;
}

bool V8ObjectParser::ParseGeneratorFunction(v8::Local<v8::Context> context,
                                            v8::Local<v8::Object> prototype,
                                            const AtomicString function_name,
                                            v8::Local<v8::Function>* function,
                                            ExceptionState* exception_state) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch block(isolate);

  v8::Local<v8::Value> function_value;
  if (!prototype->Get(context, V8AtomicString(isolate, function_name))
           .ToLocal(&function_value)) {
    exception_state->RethrowV8Exception(block.Exception());
    return false;
  }

  if (function_value->IsNullOrUndefined()) {
    exception_state->ThrowTypeError(
        "The '" + function_name +
        "' property on the prototype does not exist.");
    return false;
  }

  if (!function_value->IsGeneratorFunction()) {
    exception_state->ThrowTypeError(
        "The '" + function_name +
        "' property on the prototype is not a generator function.");
    return false;
  }

  *function = v8::Local<v8::Function>::Cast(function_value);
  return true;
}

bool V8ObjectParser::ParseCSSPropertyList(
    v8::Local<v8::Context> context,
    v8::Local<v8::Function> constructor,
    const AtomicString list_name,
    Vector<CSSPropertyID>* native_properties,
    Vector<AtomicString>* custom_properties,
    ExceptionState* exception_state) {
  v8::Isolate* isolate = context->GetIsolate();
  v8::TryCatch block(isolate);

  v8::Local<v8::Value> list_value;
  if (!constructor->Get(context, V8AtomicString(isolate, list_name))
           .ToLocal(&list_value)) {
    exception_state->RethrowV8Exception(block.Exception());
    return false;
  }

  if (!list_value->IsNullOrUndefined()) {
    Vector<String> properties =
        NativeValueTraits<IDLSequence<IDLString>>::NativeValue(
            isolate, list_value, *exception_state);

    if (exception_state->HadException())
      return false;

    for (const auto& property : properties) {
      CSSPropertyID property_id = cssPropertyID(property);
      if (property_id == CSSPropertyVariable) {
        custom_properties->push_back(std::move(property));
      } else if (property_id != CSSPropertyInvalid) {
        native_properties->push_back(std::move(property_id));
      }
    }
  }

  return true;
}

}  // namespace blink
