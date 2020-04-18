// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/easy_unlock_proximity_required_stub.h"

#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/bindings/api_event_handler.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"

namespace extensions {

v8::Local<v8::Object> EasyUnlockProximityRequiredStub::Create(
    v8::Isolate* isolate,
    const std::string& property_name,
    const base::ListValue* property_values,
    APIRequestHandler* request_handler,
    APIEventHandler* event_handler,
    APITypeReferenceMap* type_refs,
    const BindingAccessChecker* access_checker) {
  gin::Handle<EasyUnlockProximityRequiredStub> handle = gin::CreateHandle(
      isolate, new EasyUnlockProximityRequiredStub(event_handler));
  return handle.ToV8().As<v8::Object>();
}

EasyUnlockProximityRequiredStub::EasyUnlockProximityRequiredStub(
    APIEventHandler* event_handler)
    : event_handler_(event_handler) {}
EasyUnlockProximityRequiredStub::~EasyUnlockProximityRequiredStub() {}

gin::WrapperInfo EasyUnlockProximityRequiredStub::kWrapperInfo = {
    gin::kEmbedderNativeGin};

gin::ObjectTemplateBuilder
EasyUnlockProximityRequiredStub::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return Wrappable<EasyUnlockProximityRequiredStub>::GetObjectTemplateBuilder(
             isolate)
      .SetProperty("onChange",
                   &EasyUnlockProximityRequiredStub::GetOnChangeEvent);
}

v8::Local<v8::Value> EasyUnlockProximityRequiredStub::GetOnChangeEvent(
    gin::Arguments* arguments) {
  v8::Isolate* isolate = arguments->isolate();
  v8::Local<v8::Context> context = arguments->GetHolderCreationContext();
  v8::Local<v8::Object> wrapper = GetWrapper(isolate).ToLocalChecked();

  if (!binding::IsContextValidOrThrowError(context))
    return v8::Undefined(isolate);

  v8::Local<v8::Private> key = v8::Private::ForApi(
      isolate, gin::StringToSymbol(isolate, "onChangeEvent"));
  v8::Local<v8::Value> event;
  if (!wrapper->GetPrivate(context, key).ToLocal(&event)) {
    NOTREACHED();
    return v8::Local<v8::Value>();
  }

  DCHECK(!event.IsEmpty());
  if (event->IsUndefined()) {
    event = event_handler_->CreateAnonymousEventInstance(context);
    v8::Maybe<bool> set_result = wrapper->SetPrivate(context, key, event);
    if (!set_result.IsJust() || !set_result.FromJust()) {
      NOTREACHED();
      return v8::Local<v8::Value>();
    }
  }
  return event;
}

}  // namespace extensions
