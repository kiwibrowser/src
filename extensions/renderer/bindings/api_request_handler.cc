// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/api_request_handler.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/values.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/renderer/bindings/api_response_validator.h"
#include "extensions/renderer/bindings/exception_handler.h"
#include "extensions/renderer/bindings/js_runner.h"
#include "gin/converter.h"
#include "gin/data_object_builder.h"
#include "third_party/blink/public/web/web_scoped_user_gesture.h"
#include "third_party/blink/public/web/web_user_gesture_indicator.h"

namespace extensions {

// A helper class to adapt base::Value-style response arguments to v8 arguments
// lazily, or simply return v8 arguments directly (depending on which style of
// arguments were used in construction).
class APIRequestHandler::ArgumentAdapter {
 public:
  explicit ArgumentAdapter(const base::ListValue* base_argumements);
  explicit ArgumentAdapter(
      const std::vector<v8::Local<v8::Value>>& v8_arguments);
  ~ArgumentAdapter();

  const std::vector<v8::Local<v8::Value>>& GetArguments(
      v8::Local<v8::Context> context) const;

 private:
  const base::ListValue* base_arguments_ = nullptr;
  mutable std::vector<v8::Local<v8::Value>> v8_arguments_;

  DISALLOW_COPY_AND_ASSIGN(ArgumentAdapter);
};

APIRequestHandler::ArgumentAdapter::ArgumentAdapter(
    const base::ListValue* base_arguments)
    : base_arguments_(base_arguments) {}
APIRequestHandler::ArgumentAdapter::ArgumentAdapter(
    const std::vector<v8::Local<v8::Value>>& v8_arguments)
    : v8_arguments_(v8_arguments) {}
APIRequestHandler::ArgumentAdapter::~ArgumentAdapter() = default;

const std::vector<v8::Local<v8::Value>>&
APIRequestHandler::ArgumentAdapter::GetArguments(
    v8::Local<v8::Context> context) const {
  v8::Isolate* isolate = context->GetIsolate();
  DCHECK(isolate->GetCurrentContext() == context);

  if (base_arguments_) {
    DCHECK(v8_arguments_.empty())
        << "GetArguments() should only be called once.";
    std::unique_ptr<content::V8ValueConverter> converter =
        content::V8ValueConverter::Create();
    v8_arguments_.reserve(base_arguments_->GetSize());
    for (const auto& arg : *base_arguments_)
      v8_arguments_.push_back(converter->ToV8Value(&arg, context));
  }

  return v8_arguments_;
}

APIRequestHandler::Request::Request() {}
APIRequestHandler::Request::~Request() = default;

APIRequestHandler::PendingRequest::PendingRequest(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    const std::string& method_name,
    v8::Local<v8::Function> request_callback,
    const base::Optional<std::vector<v8::Local<v8::Value>>>&
        local_callback_args)
    : isolate(isolate), context(isolate, context), method_name(method_name) {
  if (!request_callback.IsEmpty()) {
    callback.emplace(isolate, request_callback);
    user_gesture_token =
        blink::WebUserGestureIndicator::CurrentUserGestureToken();

    if (local_callback_args) {
      callback_arguments = std::vector<v8::Global<v8::Value>>();
      callback_arguments->reserve(local_callback_args->size());
      for (const auto& arg : *local_callback_args)
        callback_arguments->emplace_back(isolate, arg);
    }
  }
}

APIRequestHandler::PendingRequest::~PendingRequest() {}
APIRequestHandler::PendingRequest::PendingRequest(PendingRequest&&) = default;
APIRequestHandler::PendingRequest& APIRequestHandler::PendingRequest::operator=(
    PendingRequest&&) = default;

APIRequestHandler::APIRequestHandler(const SendRequestMethod& send_request,
                                     APILastError last_error,
                                     ExceptionHandler* exception_handler)
    : send_request_(send_request),
      last_error_(std::move(last_error)),
      exception_handler_(exception_handler) {}

APIRequestHandler::~APIRequestHandler() {}

int APIRequestHandler::StartRequest(v8::Local<v8::Context> context,
                                    const std::string& method,
                                    std::unique_ptr<base::ListValue> arguments,
                                    v8::Local<v8::Function> callback,
                                    v8::Local<v8::Function> custom_callback,
                                    binding::RequestThread thread) {
  auto request = std::make_unique<Request>();

  // The request id is primarily used in the renderer to associate an API
  // request with the associated callback, but it's also used in the browser as
  // an identifier for the extension function (e.g. by the pageCapture API).
  // TODO(devlin): We should probably fix this, since the request id is only
  // unique per-isolate, rather than globally.
  // TODO(devlin): We could *probably* get away with just using an integer
  // here, but it's a little less foolproof. How slow is GenerateGUID? Should
  // we use that instead? It means updating the IPC
  // (ExtensionHostMsg_Request).
  // base::UnguessableToken is another good option.
  int request_id = next_request_id_++;
  request->request_id = request_id;

  base::Optional<std::vector<v8::Local<v8::Value>>> callback_args;
  v8::Isolate* isolate = context->GetIsolate();
  if (!custom_callback.IsEmpty() || !callback.IsEmpty()) {
    // In the JS bindings, custom callbacks are called with the arguments of
    // name, the full request object (see below), the original callback, and
    // the responses from the API. The responses from the API are handled by the
    // APIRequestHandler, but we need to curry in the other values.
    if (!custom_callback.IsEmpty()) {
      // TODO(devlin): The |request| object in the JS bindings includes
      // properties for callback, callbackSchema, args, stack, id, and
      // customCallback. Of those, it appears that we only use stack, args, and
      // id (since callback is curried in separately). We may be able to update
      // bindings to get away from some of those. For now, just pass in an
      // object with the request id.
      v8::Local<v8::Object> request =
          gin::DataObjectBuilder(isolate).Set("id", request_id).Build();
      v8::Local<v8::Value> callback_to_pass = callback;
      if (callback_to_pass.IsEmpty())
        callback_to_pass = v8::Undefined(isolate);
      callback_args = std::vector<v8::Local<v8::Value>>{
          gin::StringToSymbol(isolate, method), request, callback_to_pass};
      callback = custom_callback;
    }

    request->has_callback = true;
  }
  pending_requests_.insert(std::make_pair(
      request_id,
      PendingRequest(isolate, context, method, callback, callback_args)));

  request->has_user_gesture =
      blink::WebUserGestureIndicator::IsProcessingUserGestureThreadSafe();
  request->arguments = std::move(arguments);
  request->method_name = method;
  request->thread = thread;

  last_sent_request_id_ = request_id;
  send_request_.Run(std::move(request), context);
  return request_id;
}

void APIRequestHandler::CompleteRequest(int request_id,
                                        const base::ListValue& response_args,
                                        const std::string& error) {
  CompleteRequestImpl(request_id, ArgumentAdapter(&response_args), error);
}

void APIRequestHandler::CompleteRequest(
    int request_id,
    const std::vector<v8::Local<v8::Value>>& response_args,
    const std::string& error) {
  CompleteRequestImpl(request_id, ArgumentAdapter(response_args), error);
}

int APIRequestHandler::AddPendingRequest(v8::Local<v8::Context> context,
                                         v8::Local<v8::Function> callback) {
  int request_id = next_request_id_++;
  pending_requests_.emplace(
      request_id, PendingRequest(context->GetIsolate(), context, std::string(),
                                 callback, base::nullopt));
  return request_id;
}

void APIRequestHandler::InvalidateContext(v8::Local<v8::Context> context) {
  for (auto iter = pending_requests_.begin();
       iter != pending_requests_.end();) {
    if (iter->second.context == context)
      iter = pending_requests_.erase(iter);
    else
      ++iter;
  }
}

void APIRequestHandler::SetResponseValidator(
    std::unique_ptr<APIResponseValidator> response_validator) {
  DCHECK(!response_validator_);
  response_validator_ = std::move(response_validator);
}

std::set<int> APIRequestHandler::GetPendingRequestIdsForTesting() const {
  std::set<int> result;
  for (const auto& pair : pending_requests_)
    result.insert(pair.first);
  return result;
}

void APIRequestHandler::CompleteRequestImpl(int request_id,
                                            const ArgumentAdapter& arguments,
                                            const std::string& error) {
  auto iter = pending_requests_.find(request_id);
  // The request may have been removed if the context was invalidated before a
  // response is ready.
  if (iter == pending_requests_.end())
    return;

  PendingRequest pending_request = std::move(iter->second);
  pending_requests_.erase(iter);

  v8::Isolate* isolate = pending_request.isolate;
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = pending_request.context.Get(isolate);
  v8::Context::Scope context_scope(context);

  if (!pending_request.callback) {
    // If there's no callback associated with the request, but there is an
    // error, report the error as if it were unchecked.
    if (!error.empty()) {
      // TODO(devlin): Use pending_requeset.method_name here?
      last_error_.ReportUncheckedError(context, error);
    }
    // No callback to trigger, so we're done!
    return;
  }

  std::vector<v8::Local<v8::Value>> full_args;
  const std::vector<v8::Local<v8::Value>>& response_args =
      arguments.GetArguments(context);
  size_t curried_argument_size =
      pending_request.callback_arguments
          ? pending_request.callback_arguments->size()
          : 0u;
  full_args.reserve(response_args.size() + curried_argument_size);
  if (pending_request.callback_arguments) {
    for (const auto& arg : *pending_request.callback_arguments)
      full_args.push_back(arg.Get(isolate));
  }
  full_args.insert(full_args.end(), response_args.begin(), response_args.end());

  blink::WebScopedUserGesture user_gesture(*pending_request.user_gesture_token);
  if (!error.empty())
    last_error_.SetError(context, error);

  if (response_validator_) {
    bool has_custom_callback = !!pending_request.callback_arguments;
    response_validator_->ValidateResponse(
        context, pending_request.method_name, response_args, error,
        has_custom_callback
            ? APIResponseValidator::CallbackType::kAPIProvided
            : APIResponseValidator::CallbackType::kCallerProvided);
  }

  v8::TryCatch try_catch(isolate);
  // args.size() is converted to int, but args is controlled by chrome and is
  // never close to std::numeric_limits<int>::max.
  JSRunner::Get(context)->RunJSFunction(pending_request.callback->Get(isolate),
                                        context, full_args.size(),
                                        full_args.data());
  if (try_catch.HasCaught()) {
    v8::Local<v8::Message> v8_message = try_catch.Message();
    base::Optional<std::string> message;
    if (!v8_message.IsEmpty())
      message = gin::V8ToString(v8_message->Get());
    exception_handler_->HandleException(context, "Error handling response",
                                        &try_catch);
  }

  if (!error.empty())
    last_error_.ClearError(context, true);
}

}  // namespace extensions
