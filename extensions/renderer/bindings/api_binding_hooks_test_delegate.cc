// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/api_binding_hooks_test_delegate.h"

namespace extensions {

APIBindingHooksTestDelegate::APIBindingHooksTestDelegate() {}
APIBindingHooksTestDelegate::~APIBindingHooksTestDelegate() {}

bool APIBindingHooksTestDelegate::CreateCustomEvent(
    v8::Local<v8::Context> context,
    const std::string& event_name,
    v8::Local<v8::Value>* event_out) {
  if (!custom_event_.is_null()) {
    *event_out = custom_event_.Run(context, event_name);
    return true;
  }
  return false;
}

void APIBindingHooksTestDelegate::AddHandler(base::StringPiece name,
                                             const RequestHandler& handler) {
  request_handlers_[name.as_string()] = handler;
}

void APIBindingHooksTestDelegate::SetCustomEvent(
    const CustomEventFactory& custom_event) {
  custom_event_ = custom_event;
}

void APIBindingHooksTestDelegate::SetTemplateInitializer(
    const TemplateInitializer& initializer) {
  template_initializer_ = initializer;
}

void APIBindingHooksTestDelegate::SetInstanceInitializer(
    const InstanceInitializer& initializer) {
  instance_initializer_ = initializer;
}

APIBindingHooks::RequestResult APIBindingHooksTestDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  auto iter = request_handlers_.find(method_name);
  if (iter == request_handlers_.end()) {
    return APIBindingHooks::RequestResult(
        APIBindingHooks::RequestResult::NOT_HANDLED);
  }
  return iter->second.Run(signature, context, arguments, refs);
}

void APIBindingHooksTestDelegate::InitializeTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> object_template,
    const APITypeReferenceMap& type_refs) {
  if (template_initializer_)
    template_initializer_.Run(isolate, object_template, type_refs);
}

void APIBindingHooksTestDelegate::InitializeInstance(
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> instance) {
  if (instance_initializer_)
    instance_initializer_.Run(context, instance);
}

}  // namespace extensions
