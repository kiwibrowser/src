// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_SEND_REQUEST_NATIVES_H_
#define EXTENSIONS_RENDERER_SEND_REQUEST_NATIVES_H_

#include "base/macros.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace extensions {
class RequestSender;
class ScriptContext;

// Native functions exposed to extensions via JS for calling API functions in
// the browser.
class SendRequestNatives : public ObjectBackedNativeHandler {
 public:
  SendRequestNatives(RequestSender* request_sender, ScriptContext* context);

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  // Starts an API request to the browser, with an optional callback.  The
  // callback will be dispatched to EventBindings::HandleResponse.
  void StartRequest(const v8::FunctionCallbackInfo<v8::Value>& args);

  // Gets a reference to an object's global object.
  void GetGlobal(const v8::FunctionCallbackInfo<v8::Value>& args);

  RequestSender* request_sender_;

  DISALLOW_COPY_AND_ASSIGN(SendRequestNatives);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_SEND_REQUEST_NATIVES_H_
