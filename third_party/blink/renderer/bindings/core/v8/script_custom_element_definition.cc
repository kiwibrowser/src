// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/script_custom_element_definition.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_binding_for_core.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_custom_element_registry.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_element.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_error_handler.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_script_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_throw_dom_exception.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/events/error_event.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/custom/custom_element.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding_macros.h"
#include "third_party/blink/renderer/platform/bindings/v8_private_property.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "v8/include/v8.h"

namespace blink {

class CSSStyleSheet;

ScriptCustomElementDefinition* ScriptCustomElementDefinition::ForConstructor(
    ScriptState* script_state,
    CustomElementRegistry* registry,
    const v8::Local<v8::Value>& constructor) {
  V8PerContextData* per_context_data = script_state->PerContextData();
  // TODO(yukishiino): Remove this check when crbug.com/583429 is fixed.
  if (UNLIKELY(!per_context_data))
    return nullptr;
  auto private_id = per_context_data->GetPrivateCustomElementDefinitionId();
  v8::Local<v8::Value> id_value;
  if (!constructor.As<v8::Object>()
           ->GetPrivate(script_state->GetContext(), private_id)
           .ToLocal(&id_value))
    return nullptr;
  if (!id_value->IsUint32())
    return nullptr;
  uint32_t id = id_value.As<v8::Uint32>()->Value();

  // This downcast is safe because only ScriptCustomElementDefinitions
  // have an ID associated with them. This relies on three things:
  //
  // 1. Only ScriptCustomElementDefinition::Create sets the private
  //    property on a constructor.
  //
  // 2. CustomElementRegistry adds ScriptCustomElementDefinitions
  //    assigned an ID to the list of definitions without fail.
  //
  // 3. The relationship between the CustomElementRegistry and its
  //    private property is never mixed up; this is guaranteed by the
  //    bindings system because the registry is associated with its
  //    context.
  //
  // At a meta-level, this downcast is safe because there is
  // currently only one implementation of CustomElementDefinition in
  // product code and that is ScriptCustomElementDefinition. But
  // that may change in the future.
  CustomElementDefinition* definition = registry->DefinitionForId(id);
  CHECK(definition);
  return static_cast<ScriptCustomElementDefinition*>(definition);
}

ScriptCustomElementDefinition* ScriptCustomElementDefinition::Create(
    ScriptState* script_state,
    CustomElementRegistry* registry,
    const CustomElementDescriptor& descriptor,
    CustomElementDefinition::Id id,
    const v8::Local<v8::Object>& constructor,
    const v8::Local<v8::Function>& connected_callback,
    const v8::Local<v8::Function>& disconnected_callback,
    const v8::Local<v8::Function>& adopted_callback,
    const v8::Local<v8::Function>& attribute_changed_callback,
    HashSet<AtomicString>&& observed_attributes,
    CSSStyleSheet* default_style_sheet) {
  ScriptCustomElementDefinition* definition = new ScriptCustomElementDefinition(
      script_state, descriptor, constructor, connected_callback,
      disconnected_callback, adopted_callback, attribute_changed_callback,
      std::move(observed_attributes), default_style_sheet);

  // Tag the JavaScript constructor object with its ID.
  v8::Local<v8::Value> id_value =
      v8::Integer::NewFromUnsigned(script_state->GetIsolate(), id);
  auto private_id =
      script_state->PerContextData()->GetPrivateCustomElementDefinitionId();
  CHECK(
      constructor->SetPrivate(script_state->GetContext(), private_id, id_value)
          .ToChecked());

  return definition;
}

ScriptCustomElementDefinition::ScriptCustomElementDefinition(
    ScriptState* script_state,
    const CustomElementDescriptor& descriptor,
    const v8::Local<v8::Object>& constructor,
    const v8::Local<v8::Function>& connected_callback,
    const v8::Local<v8::Function>& disconnected_callback,
    const v8::Local<v8::Function>& adopted_callback,
    const v8::Local<v8::Function>& attribute_changed_callback,
    HashSet<AtomicString>&& observed_attributes,
    CSSStyleSheet* default_style_sheet)
    : CustomElementDefinition(descriptor,
                              default_style_sheet,
                              std::move(observed_attributes)),
      script_state_(script_state),
      constructor_(script_state->GetIsolate(), constructor) {
  v8::Isolate* isolate = script_state->GetIsolate();
  if (!connected_callback.IsEmpty())
    connected_callback_.Set(isolate, connected_callback);
  if (!disconnected_callback.IsEmpty())
    disconnected_callback_.Set(isolate, disconnected_callback);
  if (!adopted_callback.IsEmpty())
    adopted_callback_.Set(isolate, adopted_callback);
  if (!attribute_changed_callback.IsEmpty())
    attribute_changed_callback_.Set(isolate, attribute_changed_callback);
}

void ScriptCustomElementDefinition::TraceWrappers(
    ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(constructor_.Cast<v8::Value>());
  visitor->TraceWrappers(connected_callback_.Cast<v8::Value>());
  visitor->TraceWrappers(disconnected_callback_.Cast<v8::Value>());
  visitor->TraceWrappers(adopted_callback_.Cast<v8::Value>());
  visitor->TraceWrappers(attribute_changed_callback_.Cast<v8::Value>());
  CustomElementDefinition::TraceWrappers(visitor);
}

HTMLElement* ScriptCustomElementDefinition::HandleCreateElementSyncException(
    Document& document,
    const QualifiedName& tag_name,
    v8::Isolate* isolate,
    ExceptionState& exception_state) {
  DCHECK(exception_state.HadException());
  // 6.1."If any of these subsubsteps threw an exception".1
  // Report the exception.
  V8ScriptRunner::ReportException(isolate, exception_state.GetException());
  exception_state.ClearException();
  // ... .2 Return HTMLUnknownElement.
  return CustomElement::CreateFailedElement(document, tag_name);
}

HTMLElement* ScriptCustomElementDefinition::CreateAutonomousCustomElementSync(
    Document& document,
    const QualifiedName& tag_name) {
  if (!script_state_->ContextIsValid())
    return CustomElement::CreateFailedElement(document, tag_name);
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();

  ExceptionState exception_state(isolate, ExceptionState::kConstructionContext,
                                 "CustomElement");

  // Create an element with the synchronous custom elements flag set.
  // https://dom.spec.whatwg.org/#concept-create-element

  // TODO(dominicc): Implement step 5 which constructs customized
  // built-in elements.

  Element* element = nullptr;
  {
    v8::TryCatch try_catch(script_state_->GetIsolate());

    if (document.IsHTMLImport()) {
      // V8HTMLElement::constructorCustom() can only refer to
      // window.document() which is not the import document. Create
      // elements in import documents ahead of time so they end up in
      // the right document. This subtly violates recursive
      // construction semantics, but only in import documents.
      element = CreateElementForConstructor(document);
      DCHECK(!try_catch.HasCaught());

      ConstructionStackScope construction_stack_scope(this, element);
      element = CallConstructor();
    } else {
      element = CallConstructor();
    }

    if (try_catch.HasCaught()) {
      exception_state.RethrowV8Exception(try_catch.Exception());
      return HandleCreateElementSyncException(document, tag_name, isolate,
                                              exception_state);
    }
  }

  // 6.1.3. through 6.1.9.
  CheckConstructorResult(element, document, tag_name, exception_state);
  if (exception_state.HadException()) {
    return HandleCreateElementSyncException(document, tag_name, isolate,
                                            exception_state);
  }
  // 6.1.10. Set result’s namespace prefix to prefix.
  if (element->prefix() != tag_name.Prefix())
    element->SetTagNameForCreateElementNS(tag_name);
  DCHECK_EQ(element->GetCustomElementState(), CustomElementState::kCustom);
  return ToHTMLElement(element);
}

// https://html.spec.whatwg.org/multipage/scripting.html#upgrades
bool ScriptCustomElementDefinition::RunConstructor(Element* element) {
  if (!script_state_->ContextIsValid())
    return false;
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();

  // Step 5 says to rethrow the exception; but there is no one to
  // catch it. The side effect is to report the error.
  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);

  Element* result = CallConstructor();

  // To report exception thrown from callConstructor()
  if (try_catch.HasCaught())
    return false;

  // To report InvalidStateError Exception, when the constructor returns some
  // different object
  if (result != element) {
    const String& message =
        "custom element constructors must call super() first and must "
        "not return a different object";
    v8::Local<v8::Value> exception = V8ThrowDOMException::CreateDOMException(
        script_state_->GetIsolate(), kInvalidStateError, message);
    V8ScriptRunner::ReportException(isolate, exception);
    return false;
  }

  return true;
}

Element* ScriptCustomElementDefinition::CallConstructor() {
  v8::Isolate* isolate = script_state_->GetIsolate();
  DCHECK(ScriptState::Current(isolate) == script_state_);
  ExecutionContext* execution_context =
      ExecutionContext::From(script_state_.get());
  v8::Local<v8::Value> result;
  if (!V8ScriptRunner::CallAsConstructor(isolate, Constructor(),
                                         execution_context, 0, nullptr)
           .ToLocal(&result)) {
    return nullptr;
  }
  return V8Element::ToImplWithTypeCheck(isolate, result);
}

v8::Local<v8::Object> ScriptCustomElementDefinition::Constructor() const {
  DCHECK(!constructor_.IsEmpty());
  return constructor_.NewLocal(script_state_->GetIsolate());
}

// CustomElementDefinition
ScriptValue ScriptCustomElementDefinition::GetConstructorForScript() {
  return ScriptValue(script_state_.get(), Constructor());
}

bool ScriptCustomElementDefinition::HasConnectedCallback() const {
  return !connected_callback_.IsEmpty();
}

bool ScriptCustomElementDefinition::HasDisconnectedCallback() const {
  return !disconnected_callback_.IsEmpty();
}

bool ScriptCustomElementDefinition::HasAdoptedCallback() const {
  return !adopted_callback_.IsEmpty();
}

void ScriptCustomElementDefinition::RunCallback(
    v8::Local<v8::Function> callback,
    Element* element,
    int argc,
    v8::Local<v8::Value> argv[]) {
  DCHECK(ScriptState::Current(script_state_->GetIsolate()) == script_state_);
  v8::Isolate* isolate = script_state_->GetIsolate();

  // Invoke custom element reactions
  // https://html.spec.whatwg.org/multipage/scripting.html#invoke-custom-element-reactions
  // If this throws any exception, then report the exception.
  v8::TryCatch try_catch(isolate);
  try_catch.SetVerbose(true);

  ExecutionContext* execution_context =
      ExecutionContext::From(script_state_.get());
  v8::Local<v8::Value> element_handle =
      ToV8(element, script_state_->GetContext()->Global(), isolate);
  V8ScriptRunner::CallFunction(callback, execution_context, element_handle,
                               argc, argv, isolate);
}

void ScriptCustomElementDefinition::RunConnectedCallback(Element* element) {
  if (!script_state_->ContextIsValid())
    return;
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();
  RunCallback(connected_callback_.NewLocal(isolate), element);
}

void ScriptCustomElementDefinition::RunDisconnectedCallback(Element* element) {
  if (!script_state_->ContextIsValid())
    return;
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();
  RunCallback(disconnected_callback_.NewLocal(isolate), element);
}

void ScriptCustomElementDefinition::RunAdoptedCallback(Element* element,
                                                       Document* old_owner,
                                                       Document* new_owner) {
  if (!script_state_->ContextIsValid())
    return;
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::Local<v8::Value> argv[] = {
      ToV8(old_owner, script_state_->GetContext()->Global(), isolate),
      ToV8(new_owner, script_state_->GetContext()->Global(), isolate)};
  RunCallback(adopted_callback_.NewLocal(isolate), element, arraysize(argv),
              argv);
}

void ScriptCustomElementDefinition::RunAttributeChangedCallback(
    Element* element,
    const QualifiedName& name,
    const AtomicString& old_value,
    const AtomicString& new_value) {
  if (!script_state_->ContextIsValid())
    return;
  ScriptState::Scope scope(script_state_.get());
  v8::Isolate* isolate = script_state_->GetIsolate();
  v8::Local<v8::Value> argv[] = {
      V8String(isolate, name.LocalName()), V8StringOrNull(isolate, old_value),
      V8StringOrNull(isolate, new_value),
      V8StringOrNull(isolate, name.NamespaceURI()),
  };
  RunCallback(attribute_changed_callback_.NewLocal(isolate), element,
              arraysize(argv), argv);
}

}  // namespace blink
