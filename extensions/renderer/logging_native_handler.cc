// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "extensions/renderer/logging_native_handler.h"
#include "extensions/renderer/script_context.h"

namespace extensions {

LoggingNativeHandler::LoggingNativeHandler(ScriptContext* context)
    : ObjectBackedNativeHandler(context) {}

LoggingNativeHandler::~LoggingNativeHandler() {}

void LoggingNativeHandler::AddRoutes() {
  RouteHandlerFunction("DCHECK", base::Bind(&LoggingNativeHandler::Dcheck,
                                            base::Unretained(this)));
  RouteHandlerFunction("CHECK", base::Bind(&LoggingNativeHandler::Check,
                                           base::Unretained(this)));
  RouteHandlerFunction(
      "DCHECK_IS_ON",
      base::Bind(&LoggingNativeHandler::DcheckIsOn, base::Unretained(this)));
  RouteHandlerFunction(
      "LOG", base::Bind(&LoggingNativeHandler::Log, base::Unretained(this)));
  RouteHandlerFunction("WARNING", base::Bind(&LoggingNativeHandler::Warning,
                                             base::Unretained(this)));
}

void LoggingNativeHandler::Check(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool check_value;
  std::string error_message;
  ParseArgs(args, &check_value, &error_message);
  CHECK(check_value) << error_message;
}

void LoggingNativeHandler::Dcheck(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  bool check_value;
  std::string error_message;
  ParseArgs(args, &check_value, &error_message);
  DCHECK(check_value) << error_message;
}

void LoggingNativeHandler::DcheckIsOn(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  args.GetReturnValue().Set(DCHECK_IS_ON());
}

void LoggingNativeHandler::Log(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(1, args.Length());
  LOG(INFO) << *v8::String::Utf8Value(args.GetIsolate(), args[0]);
}

void LoggingNativeHandler::Warning(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(1, args.Length());
  LOG(WARNING) << *v8::String::Utf8Value(args.GetIsolate(), args[0]);
}

void LoggingNativeHandler::ParseArgs(
    const v8::FunctionCallbackInfo<v8::Value>& args,
    bool* check_value,
    std::string* error_message) {
  CHECK_LE(args.Length(), 2);
  *check_value = args[0]->BooleanValue();
  if (args.Length() == 2) {
    *error_message = "Error: " + std::string(*v8::String::Utf8Value(
                                     args.GetIsolate(), args[1]));
  }

  if (!check_value)
    *error_message += "\n" + context()->GetStackTraceAsString();
}

}  // namespace extensions
