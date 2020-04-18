// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_custom_element_definition_builder.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/idl_types.h"
#include "third_party/blink/renderer/bindings/core/v8/native_value_traits_impl.h"
#include "third_party/blink/renderer/bindings/core/v8/script_custom_element_definition.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/dom_wrapper_world.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding_macros.h"

namespace blink {

ScriptCustomElementDefinitionBuilder::ScriptCustomElementDefinitionBuilder(
    ScriptState* script_state,
    CustomElementRegistry* registry,
    CSSStyleSheet* default_style_sheet,
    const ScriptValue& constructor,
    ExceptionState& exception_state)
    : script_state_(script_state),
      registry_(registry),
      default_style_sheet_(default_style_sheet),
      constructor_value_(constructor.V8Value()),
      exception_state_(exception_state) {}

bool ScriptCustomElementDefinitionBuilder::CheckConstructorIntrinsics() {
  DCHECK(script_state_->World().IsMainWorld());

  // The signature of CustomElementRegistry.define says this is a
  // Function
  // https://html.spec.whatwg.org/multipage/scripting.html#customelementsregistry
  CHECK(constructor_value_->IsFunction());
  constructor_ = constructor_value_.As<v8::Object>();
  if (!constructor_->IsConstructor()) {
    exception_state_.ThrowTypeError(
        "constructor argument is not a constructor");
    return false;
  }
  return true;
}

bool ScriptCustomElementDefinitionBuilder::CheckConstructorNotRegistered() {
  if (!ScriptCustomElementDefinition::ForConstructor(script_state_.get(),
                                                     registry_, constructor_))
    return true;

  // Constructor is already registered.
  exception_state_.ThrowDOMException(
      kNotSupportedError,
      "this constructor has already been used with this registry");
  return false;
}

bool ScriptCustomElementDefinitionBuilder::ValueForName(
    v8::Isolate* isolate,
    v8::Local<v8::Context>& context,
    const v8::TryCatch& try_catch,
    const v8::Local<v8::Object>& object,
    const StringView& name,
    v8::Local<v8::Value>& value) const {
  v8::Local<v8::String> name_string = V8AtomicString(isolate, name);
  if (!object->Get(context, name_string).ToLocal(&value)) {
    exception_state_.RethrowV8Exception(try_catch.Exception());
    return false;
  }
  return script_state_->ContextIsValid();
}

bool ScriptCustomElementDefinitionBuilder::CheckPrototype() {
  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::Local<v8::Context> context = script_state_->GetContext();
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Value> prototype_value;
  if (!ValueForName(isolate, context, try_catch, constructor_, "prototype",
                    prototype_value))
    return false;
  if (!prototype_value->IsObject()) {
    exception_state_.ThrowTypeError("constructor prototype is not an object");
    return false;
  }
  prototype_ = prototype_value.As<v8::Object>();
  // If retrieving the prototype destroyed the context, indicate that
  // defining the element should not proceed.
  return true;
}

bool ScriptCustomElementDefinitionBuilder::CallableForName(
    v8::Isolate* isolate,
    v8::Local<v8::Context>& context,
    const v8::TryCatch& try_catch,
    const StringView& name,
    v8::Local<v8::Function>& callback) const {
  v8::Local<v8::Value> value;
  if (!ValueForName(isolate, context, try_catch, prototype_, name, value))
    return false;
  // "undefined" means "omitted", which is valid.
  if (value->IsUndefined())
    return true;
  if (!value->IsFunction()) {
    exception_state_.ThrowTypeError(String::Format(
        "\"%s\" is not a callable object", name.ToString().Ascii().data()));
    return false;
  }
  callback = value.As<v8::Function>();
  return true;
}

bool ScriptCustomElementDefinitionBuilder::RetrieveObservedAttributes(
    v8::Isolate* isolate,
    v8::Local<v8::Context>& context,
    const v8::TryCatch& try_catch) {
  v8::Local<v8::Value> observed_attributes_value;
  if (!ValueForName(isolate, context, try_catch, constructor_,
                    "observedAttributes", observed_attributes_value))
    return false;
  if (observed_attributes_value->IsUndefined())
    return true;
  Vector<String> list = NativeValueTraits<IDLSequence<IDLString>>::NativeValue(
      isolate, observed_attributes_value, exception_state_);
  if (exception_state_.HadException() || !script_state_->ContextIsValid())
    return false;
  if (list.IsEmpty())
    return true;
  observed_attributes_.ReserveCapacityForSize(list.size());
  for (const auto& attribute : list)
    observed_attributes_.insert(AtomicString(attribute));
  return true;
}

bool ScriptCustomElementDefinitionBuilder::RememberOriginalProperties() {
  // Spec requires to use values of these properties at the point
  // CustomElementDefinition is built, even if JS changes them afterwards.
  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::Local<v8::Context> context = script_state_->GetContext();
  v8::TryCatch try_catch(isolate);
  return CallableForName(isolate, context, try_catch, "connectedCallback",
                         connected_callback_) &&
         CallableForName(isolate, context, try_catch, "disconnectedCallback",
                         disconnected_callback_) &&
         CallableForName(isolate, context, try_catch, "adoptedCallback",
                         adopted_callback_) &&
         CallableForName(isolate, context, try_catch,
                         "attributeChangedCallback",
                         attribute_changed_callback_) &&
         (attribute_changed_callback_.IsEmpty() ||
          RetrieveObservedAttributes(isolate, context, try_catch));
}

CustomElementDefinition* ScriptCustomElementDefinitionBuilder::Build(
    const CustomElementDescriptor& descriptor,
    CustomElementDefinition::Id id) {
  return ScriptCustomElementDefinition::Create(
      script_state_.get(), registry_, descriptor, id, constructor_,
      connected_callback_, disconnected_callback_, adopted_callback_,
      attribute_changed_callback_, std::move(observed_attributes_),
      default_style_sheet_);
}

}  // namespace blink
