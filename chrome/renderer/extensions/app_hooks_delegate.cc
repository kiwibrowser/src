// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/extensions/app_hooks_delegate.h"

#include "extensions/renderer/api_activity_logger.h"
#include "extensions/renderer/bindings/api_request_handler.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/script_context_set.h"
#include "gin/converter.h"

namespace extensions {

namespace {

void IsInstalledGetterCallback(
    v8::Local<v8::String> property,
    const v8::PropertyCallbackInfo<v8::Value>& info) {
  v8::HandleScope handle_scope(info.GetIsolate());
  v8::Local<v8::Context> context = info.Holder()->CreationContext();
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);
  auto* core =
      static_cast<AppBindingsCore*>(info.Data().As<v8::External>()->Value());
  // Since this is more-or-less an API, log it as an API call.
  APIActivityLogger::LogAPICall(context, "app.getIsInstalled",
                                std::vector<v8::Local<v8::Value>>());
  info.GetReturnValue().Set(core->GetIsInstalled(script_context));
}

}  // namespace

AppHooksDelegate::AppHooksDelegate(Dispatcher* dispatcher,
                                   APIRequestHandler* request_handler)
    : app_core_(dispatcher), request_handler_(request_handler) {}
AppHooksDelegate::~AppHooksDelegate() {}

APIBindingHooks::RequestResult AppHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  using RequestResult = APIBindingHooks::RequestResult;

  v8::Isolate* isolate = context->GetIsolate();
  std::vector<v8::Local<v8::Value>> arguments_out;
  std::string error;
  v8::TryCatch try_catch(isolate);
  if (!signature->ParseArgumentsToV8(context, *arguments, refs, &arguments_out,
                                     &error)) {
    if (try_catch.HasCaught()) {
      try_catch.ReThrow();
      return RequestResult(RequestResult::THROWN);
    }
    return RequestResult(RequestResult::INVALID_INVOCATION);
  }

  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  DCHECK(script_context);

  APIBindingHooks::RequestResult result(
      APIBindingHooks::RequestResult::HANDLED);
  if (method_name == "app.getIsInstalled") {
    result.return_value =
        v8::Boolean::New(isolate, app_core_.GetIsInstalled(script_context));
  } else if (method_name == "app.getDetails") {
    result.return_value = app_core_.GetDetails(script_context);
  } else if (method_name == "app.runningState") {
    result.return_value =
        gin::StringToSymbol(isolate, app_core_.GetRunningState(script_context));
  } else if (method_name == "app.installState") {
    DCHECK_EQ(1u, arguments_out.size());
    DCHECK(arguments_out[0]->IsFunction());
    int request_id = request_handler_->AddPendingRequest(
        context, arguments_out[0].As<v8::Function>());
    app_core_.GetInstallState(
        script_context, base::BindOnce(&AppHooksDelegate::OnGotInstallState,
                                       base::Unretained(this), request_id));
  } else {
    NOTREACHED();
  }

  return result;
}

void AppHooksDelegate::InitializeTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> object_template,
    const APITypeReferenceMap& type_refs) {
  // We expose a boolean isInstalled on the chrome.app API object, as well as
  // the getIsInstalled() method.
  // TODO(devlin): :(. Hopefully we can just remove this whole API, but this is
  // particularly silly.
  // This object should outlive contexts, so the app_core_ v8::External is safe.
  // TODO(devlin): This is getting pretty common. We should find a generalized
  // solution, or make gin::ObjectTemplateBuilder work for these use cases.
  object_template->SetAccessor(gin::StringToSymbol(isolate, "isInstalled"),
                               &IsInstalledGetterCallback, nullptr,
                               v8::External::New(isolate, &app_core_));
}

void AppHooksDelegate::OnGotInstallState(int request_id,
                                         const std::string& install_state) {
  // Note: it's kind of lame that we serialize the install state to a
  // base::Value here when we're just going to later convert it to v8, but it's
  // not worth the specialization on APIRequestHandler for this oddball API.
  base::ListValue response;
  response.AppendString(install_state);
  request_handler_->CompleteRequest(request_id, response, std::string());
}

}  // namespace extensions
