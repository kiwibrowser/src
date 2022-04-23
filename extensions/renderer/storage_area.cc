// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/storage_area.h"

#include "base/strings/stringprintf.h"
#include "extensions/common/api/storage.h"
#include "extensions/renderer/bindings/api_binding_util.h"
#include "extensions/renderer/bindings/api_event_handler.h"
#include "extensions/renderer/bindings/api_request_handler.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/bindings/api_type_reference_map.h"
#include "extensions/renderer/bindings/binding_access_checker.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"

namespace extensions {

namespace {

#define DEFINE_STORAGE_AREA_HANDLERS()                            \
  const char* GetTypeName() override { return "StorageArea"; }    \
  void Get(gin::Arguments* arguments) {                           \
    storage_area_.HandleFunctionCall("get", arguments);           \
  }                                                               \
  void Set(gin::Arguments* arguments) {                           \
    storage_area_.HandleFunctionCall("set", arguments);           \
  }                                                               \
  void Remove(gin::Arguments* arguments) {                        \
    storage_area_.HandleFunctionCall("remove", arguments);        \
  }                                                               \
  void Clear(gin::Arguments* arguments) {                         \
    storage_area_.HandleFunctionCall("clear", arguments);         \
  }                                                               \
  void GetBytesInUse(gin::Arguments* arguments) {                 \
    storage_area_.HandleFunctionCall("getBytesInUse", arguments); \
  }                                                               \
  v8::Local<v8::Value> GetOnChangedEvent(gin::Arguments* arguments) {       \
    v8::Isolate* isolate = arguments->isolate();                            \
    v8::Local<v8::Context> context = arguments->GetHolderCreationContext(); \
    v8::Local<v8::Object> wrapper = GetWrapper(isolate).ToLocalChecked();   \
    return storage_area_.GetOnChangedEvent(isolate, context, wrapper);      \
  }

// gin::Wrappabnes for each of the storage areas. Since each has slightly
// different properties, and the object template is shared between all
// instances, this is a little verbose.
class LocalStorageArea final : public gin::Wrappable<LocalStorageArea> {
 public:
  LocalStorageArea(APIRequestHandler* request_handler,
                   APIEventHandler* event_handler,
                   const APITypeReferenceMap* type_refs,
                   const BindingAccessChecker* access_checker)
      : storage_area_(request_handler, event_handler, type_refs, "local", access_checker) {}
  ~LocalStorageArea() override = default;

  static gin::WrapperInfo kWrapperInfo;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return Wrappable<LocalStorageArea>::GetObjectTemplateBuilder(isolate)
        .SetMethod("get", &LocalStorageArea::Get)
        .SetMethod("set", &LocalStorageArea::Set)
        .SetMethod("remove", &LocalStorageArea::Remove)
        .SetMethod("clear", &LocalStorageArea::Clear)
        .SetMethod("getBytesInUse", &LocalStorageArea::GetBytesInUse)
        .SetProperty("onChanged", &LocalStorageArea::GetOnChangedEvent)
        .SetValue("QUOTA_BYTES", api::storage::local::QUOTA_BYTES);
  }

 private:
  DEFINE_STORAGE_AREA_HANDLERS()

  StorageArea storage_area_;

  DISALLOW_COPY_AND_ASSIGN(LocalStorageArea);
};

gin::WrapperInfo LocalStorageArea::kWrapperInfo = {gin::kEmbedderNativeGin};

class SyncStorageArea final : public gin::Wrappable<SyncStorageArea> {
 public:
  SyncStorageArea(APIRequestHandler* request_handler,
                  APIEventHandler* event_handler,
                  const APITypeReferenceMap* type_refs,
                  const BindingAccessChecker* access_checker)
      : storage_area_(request_handler, event_handler, type_refs, "sync", access_checker) {}
  ~SyncStorageArea() override = default;

  static gin::WrapperInfo kWrapperInfo;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return Wrappable<SyncStorageArea>::GetObjectTemplateBuilder(isolate)
        .SetMethod("get", &SyncStorageArea::Get)
        .SetMethod("set", &SyncStorageArea::Set)
        .SetMethod("remove", &SyncStorageArea::Remove)
        .SetMethod("clear", &SyncStorageArea::Clear)
        .SetMethod("getBytesInUse", &SyncStorageArea::GetBytesInUse)
        .SetProperty("onChanged", &SyncStorageArea::GetOnChangedEvent)
        .SetValue("QUOTA_BYTES", api::storage::sync::QUOTA_BYTES)
        .SetValue("QUOTA_BYTES_PER_ITEM",
                  api::storage::sync::QUOTA_BYTES_PER_ITEM)
        .SetValue("MAX_ITEMS", api::storage::sync::MAX_ITEMS)
        .SetValue("MAX_WRITE_OPERATIONS_PER_HOUR",
                  api::storage::sync::MAX_WRITE_OPERATIONS_PER_HOUR)
        .SetValue("MAX_WRITE_OPERATIONS_PER_MINUTE",
                  api::storage::sync::MAX_WRITE_OPERATIONS_PER_MINUTE)
        .SetValue(
            "MAX_SUSTAINED_WRITE_OPERATIONS_PER_MINUTE",
            api::storage::sync::MAX_SUSTAINED_WRITE_OPERATIONS_PER_MINUTE);
  }

 private:
  DEFINE_STORAGE_AREA_HANDLERS()

  StorageArea storage_area_;

  DISALLOW_COPY_AND_ASSIGN(SyncStorageArea);
};

gin::WrapperInfo SyncStorageArea::kWrapperInfo = {gin::kEmbedderNativeGin};

class ManagedStorageArea final : public gin::Wrappable<ManagedStorageArea> {
 public:
  ManagedStorageArea(APIRequestHandler* request_handler,
                     APIEventHandler* event_handler,
                     const APITypeReferenceMap* type_refs,
                     const BindingAccessChecker* access_checker)
      : storage_area_(request_handler, event_handler, type_refs, "managed", access_checker) {}
  ~ManagedStorageArea() override = default;

  static gin::WrapperInfo kWrapperInfo;

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return Wrappable<ManagedStorageArea>::GetObjectTemplateBuilder(isolate)
        .SetMethod("get", &ManagedStorageArea::Get)
        .SetMethod("set", &ManagedStorageArea::Set)
        .SetMethod("remove", &ManagedStorageArea::Remove)
        .SetMethod("clear", &ManagedStorageArea::Clear)
        .SetMethod("getBytesInUse", &ManagedStorageArea::GetBytesInUse)
        .SetProperty("onChanged", &ManagedStorageArea::GetOnChangedEvent);
  }

 private:
  DEFINE_STORAGE_AREA_HANDLERS()

  StorageArea storage_area_;

  DISALLOW_COPY_AND_ASSIGN(ManagedStorageArea);
};

gin::WrapperInfo ManagedStorageArea::kWrapperInfo = {gin::kEmbedderNativeGin};

#undef DEFINE_STORAGE_AREA_HANDLERS

}  // namespace

StorageArea::StorageArea(APIRequestHandler* request_handler,
                         APIEventHandler* event_handler,
                         const APITypeReferenceMap* type_refs,
                         const std::string& name,
                         const BindingAccessChecker* access_checker)
    : request_handler_(request_handler),
      event_handler_(event_handler),
      type_refs_(type_refs),
      name_(name),
      access_checker_(access_checker) {}
StorageArea::~StorageArea() = default;

// static
v8::Local<v8::Object> StorageArea::CreateStorageArea(
    v8::Isolate* isolate,
    const std::string& property_name,
    const base::ListValue* property_values,
    APIRequestHandler* request_handler,
    APIEventHandler* event_handler,
    APITypeReferenceMap* type_refs,
    const BindingAccessChecker* access_checker) {
  v8::Local<v8::Object> object;
  if (property_name == "local") {
    gin::Handle<LocalStorageArea> handle = gin::CreateHandle(
        isolate,
        new LocalStorageArea(request_handler, event_handler, type_refs, access_checker));
    object = handle.ToV8().As<v8::Object>();
  } else if (property_name == "sync") {
    gin::Handle<SyncStorageArea> handle = gin::CreateHandle(
        isolate,
        new SyncStorageArea(request_handler, event_handler, type_refs, access_checker));
    object = handle.ToV8().As<v8::Object>();
  } else {
    CHECK_EQ("managed", property_name);
    gin::Handle<ManagedStorageArea> handle = gin::CreateHandle(
        isolate,
        new ManagedStorageArea(request_handler, event_handler, type_refs, access_checker));
    object = handle.ToV8().As<v8::Object>();
  }
  return object;
}

void StorageArea::HandleFunctionCall(const std::string& method_name,
                                     gin::Arguments* arguments) {
  v8::Isolate* isolate = arguments->isolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = arguments->GetHolderCreationContext();

  // The context may have been invalidated, as in the case where this could be
  // a reference to an object from a removed frame.
  if (!binding::IsContextValidOrThrowError(context))
    return;

  std::string full_method_name = "storage." + method_name;
  if (!access_checker_->HasAccessOrThrowError(context, full_method_name))
    return;

  std::vector<v8::Local<v8::Value>> argument_list = arguments->GetAll();

  std::unique_ptr<base::ListValue> converted_arguments;
  v8::Local<v8::Function> callback;
  std::string error;
  const APISignature* signature = type_refs_->GetTypeMethodSignature(
      base::StringPrintf("%s.%s", "storage.StorageArea", method_name.c_str()));
  DCHECK(signature);
  if (!signature->ParseArgumentsToJSON(context, argument_list, *type_refs_,
                                       &converted_arguments, &callback,
                                       &error)) {
    arguments->ThrowTypeError("Invalid invocation");
    return;
  }

  converted_arguments->Insert(0u, std::make_unique<base::Value>(name_));
  request_handler_->StartRequest(
      context, full_method_name, std::move(converted_arguments), callback,
      v8::Local<v8::Function>(), binding::RequestThread::UI);
}

v8::Local<v8::Value> StorageArea::GetOnChangedEvent(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> wrapper) {
  if (!binding::IsContextValidOrThrowError(context))
    return v8::Undefined(isolate);

  v8::Local<v8::Private> key = v8::Private::ForApi(
      isolate, gin::StringToSymbol(isolate, "onChangedEvent"));
  v8::Local<v8::Value> event;
  if (!wrapper->GetPrivate(context, key).ToLocal(&event)) {
    NOTREACHED();
    return v8::Local<v8::Value>();
  }

  DCHECK(!event.IsEmpty());
  if (event->IsUndefined()) {
    constexpr bool kSupportsFilters = false;
    constexpr bool kSupportsLazyListeners = true;
    event = event_handler_->CreateEventInstance(
        base::StringPrintf("storage.%s.onChanged", name_.c_str()),
        kSupportsFilters, kSupportsLazyListeners, binding::kNoListenerMax, true,
        context);
    v8::Maybe<bool> set_result = wrapper->SetPrivate(context, key, event);
    if (!set_result.IsJust() || !set_result.FromJust()) {
      NOTREACHED();
      return v8::Local<v8::Value>();
    }
  }
  return event;
}

}  // namespace extensions
