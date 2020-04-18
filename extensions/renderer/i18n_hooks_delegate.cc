// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/i18n_hooks_delegate.h"

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "extensions/common/extension.h"
#include "extensions/renderer/bindings/api_signature.h"
#include "extensions/renderer/bindings/js_runner.h"
#include "extensions/renderer/get_script_context.h"
#include "extensions/renderer/i18n_hooks_util.h"
#include "extensions/renderer/script_context.h"
#include "gin/converter.h"

namespace extensions {

namespace {

constexpr char kGetMessage[] = "i18n.getMessage";
constexpr char kGetUILanguage[] = "i18n.getUILanguage";
constexpr char kDetectLanguage[] = "i18n.detectLanguage";

}  // namespace

using RequestResult = APIBindingHooks::RequestResult;

I18nHooksDelegate::I18nHooksDelegate() {}
I18nHooksDelegate::~I18nHooksDelegate() = default;

RequestResult I18nHooksDelegate::HandleRequest(
    const std::string& method_name,
    const APISignature* signature,
    v8::Local<v8::Context> context,
    std::vector<v8::Local<v8::Value>>* arguments,
    const APITypeReferenceMap& refs) {
  using Handler = RequestResult (I18nHooksDelegate::*)(
      ScriptContext*, const std::vector<v8::Local<v8::Value>>&);
  static const struct {
    Handler handler;
    base::StringPiece method;
  } kHandlers[] = {
      {&I18nHooksDelegate::HandleGetMessage, kGetMessage},
      {&I18nHooksDelegate::HandleGetUILanguage, kGetUILanguage},
      {&I18nHooksDelegate::HandleDetectLanguage, kDetectLanguage},
  };

  ScriptContext* script_context = GetScriptContextFromV8ContextChecked(context);

  Handler handler = nullptr;
  for (const auto& handler_entry : kHandlers) {
    if (handler_entry.method == method_name) {
      handler = handler_entry.handler;
      break;
    }
  }

  if (!handler)
    return RequestResult(RequestResult::NOT_HANDLED);

  std::string error;
  std::vector<v8::Local<v8::Value>> parsed_arguments;
  if (!signature->ParseArgumentsToV8(context, *arguments, refs,
                                     &parsed_arguments, &error)) {
    RequestResult result(RequestResult::INVALID_INVOCATION);
    result.error = std::move(error);
    return result;
  }

  return (this->*handler)(script_context, parsed_arguments);
}

RequestResult I18nHooksDelegate::HandleGetMessage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& parsed_arguments) {
  DCHECK(script_context->extension());
  DCHECK(parsed_arguments[0]->IsString());
  v8::Local<v8::Value> message = i18n_hooks::GetI18nMessage(
      gin::V8ToString(parsed_arguments[0]), script_context->extension()->id(),
      parsed_arguments[1], script_context->GetRenderFrame(),
      script_context->v8_context());

  RequestResult result(RequestResult::HANDLED);
  result.return_value = message;
  return result;
}

RequestResult I18nHooksDelegate::HandleGetUILanguage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& parsed_arguments) {
  RequestResult result(RequestResult::HANDLED);
  result.return_value = gin::StringToSymbol(
      script_context->isolate(), content::RenderThread::Get()->GetLocale());
  return result;
}

RequestResult I18nHooksDelegate::HandleDetectLanguage(
    ScriptContext* script_context,
    const std::vector<v8::Local<v8::Value>>& parsed_arguments) {
  DCHECK(parsed_arguments[0]->IsString());
  DCHECK(parsed_arguments[1]->IsFunction());

  v8::Local<v8::Context> v8_context = script_context->v8_context();

  v8::Local<v8::Value> detected_languages = i18n_hooks::DetectTextLanguage(
      v8_context, gin::V8ToString(parsed_arguments[0]));

  // NOTE(devlin): The JS bindings make this callback asynchronous through a
  // setTimeout, but it shouldn't be necessary.
  v8::Local<v8::Value> callback_args[] = {detected_languages};
  JSRunner::Get(v8_context)
      ->RunJSFunction(parsed_arguments[1].As<v8::Function>(),
                      script_context->v8_context(), arraysize(callback_args),
                      callback_args);

  return RequestResult(RequestResult::HANDLED);
}

}  // namespace extensions
