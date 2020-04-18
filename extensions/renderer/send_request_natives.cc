// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/send_request_natives.h"

#include <stdint.h>

#include "base/json/json_reader.h"
#include "base/metrics/histogram_macros.h"
#include "base/timer/elapsed_timer.h"
#include "content/public/renderer/v8_value_converter.h"
#include "extensions/renderer/request_sender.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

SendRequestNatives::SendRequestNatives(RequestSender* request_sender,
                                       ScriptContext* context)
    : ObjectBackedNativeHandler(context), request_sender_(request_sender) {}

void SendRequestNatives::AddRoutes() {
  RouteHandlerFunction(
      "StartRequest",
      base::Bind(&SendRequestNatives::StartRequest, base::Unretained(this)));
  RouteHandlerFunction("GetGlobal", base::Bind(&SendRequestNatives::GetGlobal,
                                               base::Unretained(this)));
}

// Starts an API request to the browser, with an optional callback.  The
// callback will be dispatched to EventBindings::HandleResponse.
void SendRequestNatives::StartRequest(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::ElapsedTimer timer;
  CHECK_EQ(5, args.Length());
  std::string name = *v8::String::Utf8Value(args.GetIsolate(), args[0]);
  bool has_callback = args[2]->BooleanValue();
  bool for_io_thread = args[3]->BooleanValue();
  bool preserve_null_in_objects = args[4]->BooleanValue();

  int request_id = request_sender_->GetNextRequestId();
  args.GetReturnValue().Set(static_cast<int32_t>(request_id));

  std::unique_ptr<content::V8ValueConverter> converter =
      content::V8ValueConverter::Create();

  // See http://crbug.com/149880. The context menus APIs relies on this, but
  // we shouldn't really be doing it (e.g. for the sake of the storage API).
  converter->SetFunctionAllowed(true);

  // See http://crbug.com/694248.
  converter->SetConvertNegativeZeroToInt(true);

  if (!preserve_null_in_objects)
    converter->SetStripNullFromObjects(true);

  std::unique_ptr<base::Value> value_args(
      converter->FromV8Value(args[1], context()->v8_context()));
  if (!value_args.get() || !value_args->is_list()) {
    NOTREACHED() << "Unable to convert args passed to StartRequest";
    return;
  }

  if (request_sender_->StartRequest(
          context(),
          name,
          request_id,
          has_callback,
          for_io_thread,
          static_cast<base::ListValue*>(value_args.get()))) {
    // TODO(devlin): Would it be useful to partition this data based on
    // extension function once we have a suitable baseline? crbug.com/608561.
    UMA_HISTOGRAM_TIMES("Extensions.Functions.StartRequestElapsedTime",
                        timer.Elapsed());
  }
}

void SendRequestNatives::GetGlobal(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(1, args.Length());
  CHECK(args[0]->IsObject());
  v8::Local<v8::Context> v8_context =
      v8::Local<v8::Object>::Cast(args[0])->CreationContext();
  if (ContextCanAccessObject(context()->v8_context(), v8_context->Global(),
                             false)) {
    args.GetReturnValue().Set(v8_context->Global());
  }
}

}  // namespace extensions
