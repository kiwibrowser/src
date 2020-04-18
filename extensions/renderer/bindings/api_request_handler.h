// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_BINDINGS_API_REQUEST_HANDLER_H_
#define EXTENSIONS_RENDERER_BINDINGS_API_REQUEST_HANDLER_H_

#include <map>
#include <memory>
#include <set>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "extensions/renderer/bindings/api_binding_types.h"
#include "extensions/renderer/bindings/api_last_error.h"
#include "third_party/blink/public/web/web_user_gesture_token.h"
#include "v8/include/v8.h"

namespace base {
class ListValue;
}

namespace extensions {
class APIResponseValidator;
class ExceptionHandler;

// A wrapper around a map for extension API calls. Contains all pending requests
// and the associated context and callback. Designed to be used on a single
// thread, but amongst multiple contexts.
class APIRequestHandler {
 public:
  // TODO(devlin): We may want to coalesce this with the
  // ExtensionHostMsg_Request_Params IPC struct.
  struct Request {
    Request();
    ~Request();

    int request_id = -1;
    std::string method_name;
    bool has_callback = false;
    bool has_user_gesture = false;
    binding::RequestThread thread = binding::RequestThread::UI;
    std::unique_ptr<base::ListValue> arguments;

   private:
    DISALLOW_COPY_AND_ASSIGN(Request);
  };

  using SendRequestMethod =
      base::Callback<void(std::unique_ptr<Request>, v8::Local<v8::Context>)>;

  APIRequestHandler(const SendRequestMethod& send_request,
                    APILastError last_error,
                    ExceptionHandler* exception_handler);
  ~APIRequestHandler();

  // Begins the process of processing the request. Returns the identifier of the
  // pending request, or -1 if no pending request was added (which can happen if
  // no callback was specified).
  int StartRequest(v8::Local<v8::Context> context,
                   const std::string& method,
                   std::unique_ptr<base::ListValue> arguments,
                   v8::Local<v8::Function> callback,
                   v8::Local<v8::Function> custom_callback,
                   binding::RequestThread thread);

  // Adds a pending request for the request handler to manage (and complete via
  // CompleteRequest). This is used by renderer-side implementations that
  // shouldn't be dispatched to the browser in the normal flow, but means other
  // classes don't have to worry about context invalidation.
  int AddPendingRequest(v8::Local<v8::Context> context,
                        v8::Local<v8::Function> callback);

  // Responds to the request with the given |request_id|, calling the callback
  // with the given |response| arguments.
  // Invalid ids are ignored.
  // Warning: This can run arbitrary JS code, so the |context| may be
  // invalidated after this!
  void CompleteRequest(int request_id,
                       const base::ListValue& response,
                       const std::string& error);
  void CompleteRequest(int request_id,
                       const std::vector<v8::Local<v8::Value>>& response,
                       const std::string& error);

  // Invalidates any requests that are associated with |context|.
  void InvalidateContext(v8::Local<v8::Context> context);

  void SetResponseValidator(std::unique_ptr<APIResponseValidator> validator);

  APILastError* last_error() { return &last_error_; }
  int last_sent_request_id() const { return last_sent_request_id_; }
  bool has_response_validator_for_testing() const {
    return response_validator_.get() != nullptr;
  }

  std::set<int> GetPendingRequestIdsForTesting() const;

 private:
  class ArgumentAdapter;

  struct PendingRequest {
    PendingRequest(
        v8::Isolate* isolate,
        v8::Local<v8::Context> context,
        const std::string& method_name,
        v8::Local<v8::Function> callback,
        const base::Optional<std::vector<v8::Local<v8::Value>>>& callback_args);
    ~PendingRequest();
    PendingRequest(PendingRequest&&);
    PendingRequest& operator=(PendingRequest&&);

    v8::Isolate* isolate;
    v8::Global<v8::Context> context;
    std::string method_name;

    // The following are only populated for requests with a callback.
    base::Optional<v8::Global<v8::Function>> callback;
    base::Optional<std::vector<v8::Global<v8::Value>>> callback_arguments;
    base::Optional<blink::WebUserGestureToken> user_gesture_token;
  };

  void CompleteRequestImpl(int request_id,
                           const ArgumentAdapter& arguments,
                           const std::string& error);

  // The next available request identifier.
  int next_request_id_ = 0;

  // The id of the last request we sent to the browser. This can be used as a
  // flag for whether or not a request was sent (if the last_sent_request_id_
  // changes).
  int last_sent_request_id_ = -1;

  // A map of all pending requests.
  std::map<int, PendingRequest> pending_requests_;

  SendRequestMethod send_request_;

  APILastError last_error_;

  // The exception handler for the bindings system; guaranteed to be valid
  // during this object's lifetime.
  ExceptionHandler* const exception_handler_;

  // The response validator used to check the responses for resolved requests.
  // Null if response validation is disabled.
  std::unique_ptr<APIResponseValidator> response_validator_;

  DISALLOW_COPY_AND_ASSIGN(APIRequestHandler);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_BINDINGS_API_REQUEST_HANDLER_H_
