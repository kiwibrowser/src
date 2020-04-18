// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/chromedriver/chrome/console_logger.h"

#include <stddef.h>

#include <string>

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "chrome/test/chromedriver/chrome/devtools_client.h"
#include "chrome/test/chromedriver/chrome/log.h"
#include "chrome/test/chromedriver/chrome/status.h"

namespace {

// Translates DevTools log level strings into Log::Level.
bool ConsoleLevelToLogLevel(const std::string& name, Log::Level *out_level) {
  if (name == "verbose" || name == "debug" || name == "timeEnd")
    *out_level = Log::kDebug;
  else if (name == "log" || name == "info")
    *out_level = Log::kInfo;
  else if (name == "warning")
    *out_level = Log::kWarning;
  else if (name == "error")
    *out_level = Log::kError;
  else
    return false;
  return true;
}

}  // namespace

ConsoleLogger::ConsoleLogger(Log* log)
    : log_(log) {}

Status ConsoleLogger::OnConnected(DevToolsClient* client) {
  base::DictionaryValue params;
  Status status = client->SendCommand("Log.enable", params);
  if (status.IsError()) {
    std::string message = status.message();
    if (message.find("'Log.enable' wasn't found") != std::string::npos) {
      // If the Log.enable command doesn't exist, then we're on Chrome 53 or
      // earlier. Enable the Console domain so we can listen for
      // Console.messageAdded events.
      return client->SendCommand("Console.enable", params);
    }
    return status;
  }
  // Otherwise, we're on Chrome 54+. Enable the Log and Runtime domains so we
  // can listen for Log.entryAdded and Runtime.exceptionThrown events.
  return client->SendCommand("Runtime.enable", params);
}

Status ConsoleLogger::OnEvent(
    DevToolsClient* client,
    const std::string& method,
    const base::DictionaryValue& params) {
  if (method == "Console.messageAdded")
    return OnConsoleMessageAdded(params);
  if (method == "Log.entryAdded")
    return OnLogEntryAdded(params);
  if (method == "Runtime.consoleAPICalled")
    return OnRuntimeConsoleApiCalled(params);
  if (method == "Runtime.exceptionThrown")
    return OnRuntimeExceptionThrown(params);
  return Status(kOk);
}

Status ConsoleLogger::OnConsoleMessageAdded(
    const base::DictionaryValue& params) {
  // If the event has proper structure and fields, log formatted.
  // Else it's a weird message that we don't know how to format, log full JSON.
  const base::DictionaryValue* message_dict = nullptr;
  if (params.GetDictionary("message", &message_dict)) {
    std::string text;
    std::string level_name;
    Log::Level level = Log::kInfo;
    if (message_dict->GetString("text", &text) && !text.empty() &&
        message_dict->GetString("level", &level_name) &&
        ConsoleLevelToLogLevel(level_name, &level)) {
      const char* origin_cstr = "unknown";
      std::string origin;
      if ((message_dict->GetString("url", &origin) && !origin.empty()) ||
          (message_dict->GetString("source", &origin) && !origin.empty())) {
        origin_cstr = origin.c_str();
      }

      std::string line_column;
      int line = -1;
      if (message_dict->GetInteger("line", &line)) {
        int column = -1;
        if (message_dict->GetInteger("column", &column)) {
          base::SStringPrintf(&line_column, "%d:%d", line, column);
        } else {
          base::SStringPrintf(&line_column, "%d", line);
        }
      } else {
        // No line number, but print anyway, just to maintain the number of
        // fields in the formatted message in case someone wants to parse it.
        line_column = "-";
      }

      std::string source;
      message_dict->GetString("source", &source);
      log_->AddEntry(level, source, base::StringPrintf("%s %s %s",
                                                       origin_cstr,
                                                       line_column.c_str(),
                                                       text.c_str()));

      return Status(kOk);
    }
  }

  // Don't know how to format, log full JSON.
  std::string message_json;
  base::JSONWriter::Write(params, &message_json);
  log_->AddEntry(Log::kWarning, message_json);
  return Status(kOk);
}

Status ConsoleLogger::OnLogEntryAdded(const base::DictionaryValue& params) {
  const base::DictionaryValue* entry = nullptr;
  if (!params.GetDictionary("entry", &entry))
    return Status(kUnknownError, "missing or invalid 'entry'");

  std::string level_name;
  Log::Level level;
  if (!entry->GetString("level", &level_name) ||
      !ConsoleLevelToLogLevel(level_name, &level))
    return Status(kUnknownError, "missing or invalid 'entry.level'");

  std::string source;
  if (!entry->GetString("source", &source))
    return Status(kUnknownError, "missing or invalid 'entry.source'");

  std::string origin;
  if (!entry->GetString("url", &origin))
    origin = source;

  std::string line_number;
  int line = -1;
  if (entry->GetInteger("lineNumber", &line)) {
    line_number = base::StringPrintf("%d", line);
  } else {
    // No line number, but print anyway, just to maintain the number of fields
    // in the formatted message in case someone wants to parse it.
    line_number = "-";
  }

  std::string text;
  if (!entry->GetString("text", &text))
    return Status(kUnknownError, "missing or invalid 'entry.text'");

  log_->AddEntry(level, source, base::StringPrintf("%s %s %s",
                                                   origin.c_str(),
                                                   line_number.c_str(),
                                                   text.c_str()));
  return Status(kOk);
}

Status ConsoleLogger::OnRuntimeConsoleApiCalled(
    const base::DictionaryValue& params) {
  std::string type;
  if (!params.GetString("type", &type))
    return Status(kUnknownError, "missing or invalid type");
  Log::Level level;
  if (!ConsoleLevelToLogLevel(type, &level))
    return Status(kOk);

  std::string origin = "console-api";
  std::string line_column = "-";
  const base::DictionaryValue* stack_trace = nullptr;
  if (params.GetDictionary("stackTrace", &stack_trace)) {
    const base::ListValue* call_frames = nullptr;
    if (!stack_trace->GetList("callFrames", &call_frames))
      return Status(kUnknownError, "missing or invalid callFrames");
    const base::DictionaryValue* call_frame = nullptr;
    if (call_frames->GetDictionary(0, &call_frame)) {
      std::string url;
      if (!call_frame->GetString("url", &url))
        return Status(kUnknownError, "missing or invalid url");
      if (!url.empty())
        origin = url;
      int line = -1;
      if (!call_frame->GetInteger("lineNumber", &line))
        return Status(kUnknownError, "missing or invalid lineNumber");
      int column = -1;
      if (!call_frame->GetInteger("columnNumber", &column))
        return Status(kUnknownError, "missing or invalid columnNumber");
      line_column = base::StringPrintf("%d:%d", line, column);
    }
  }

  // TODO(samuong): Handle the case where args.GetSize() > 1. This happens when
  // the first arg is a printf-style format string. We currently return the
  // format string to the test client. For details, see
  // https://bugs.chromium.org/p/chromedriver/issues/detail?id=669
  std::string text;
  const base::ListValue* args = nullptr;
  const base::DictionaryValue* first_arg = nullptr;
  if (!params.GetList("args", &args) || args->GetSize() < 1 ||
      !args->GetDictionary(0, &first_arg)) {
    return Status(kUnknownError, "missing or invalid args");
  }

  std::string arg_type;
  if (first_arg->GetString("type", &arg_type) && arg_type == "undefined") {
    text = "undefined";
  } else if (!first_arg->GetString("description", &text)) {
    const base::Value* value = nullptr;
    if (!first_arg->Get("value", &value))
      return Status(kUnknownError, "missing or invalid arg value");
    if (!base::JSONWriter::Write(*value, &text))
      return Status(kUnknownError, "failed to convert value to text");
  }

  log_->AddEntry(level, "console-api", base::StringPrintf("%s %s %s",
                                                          origin.c_str(),
                                                          line_column.c_str(),
                                                          text.c_str()));
  return Status(kOk);
}

Status ConsoleLogger::OnRuntimeExceptionThrown(
    const base::DictionaryValue& params) {
  const base::DictionaryValue* exception_details = nullptr;
  // In Chrome 54, |url|, |lineNumber| and |columnNumber| are properties of the
  // |details| dictionary. In Chrome 55+, they are inside the |exceptionDetails|
  // dictionary.
  // TODO(samuong): Stop looking at |details| once we stop supporting Chrome 54.
  if (!params.GetDictionary("exceptionDetails", &exception_details))
    if (!params.GetDictionary("details", &exception_details))
      return Status(kUnknownError, "missing or invalid exception details");

  std::string origin;
  if (!exception_details->GetString("url", &origin))
    origin = "javascript";

  int line = -1;
  if (!exception_details->GetInteger("lineNumber", &line))
    return Status(kUnknownError, "missing or invalid lineNumber");
  int column = -1;
  if (!exception_details->GetInteger("columnNumber", &column))
    return Status(kUnknownError, "missing or invalid columnNumber");
  std::string line_column = base::StringPrintf("%d:%d", line, column);

  // In Chrome 54, the exception object is serialized as a dictionary called
  // |exception|. In Chrome 55+, the exception object properties are in the
  // |exceptionDetails| object.
  // TODO(samuong): Delete this once we stop supporting Chrome 54.
  if (!params.GetDictionary("exceptionDetails", &exception_details))
    exception_details = &params;

  std::string text;
  const base::DictionaryValue* exception = nullptr;
  const base::DictionaryValue* preview = nullptr;
  const base::ListValue* properties = nullptr;
  if (exception_details->GetDictionary("exception", &exception) &&
      exception->GetDictionary("preview", &preview) &&
      preview->GetList("properties", &properties)) {
    // If the event contains an object which is an instance of the JS Error
    // class, attempt to get the message property for the exception.
    for (size_t i = 0; i < properties->GetSize(); i++) {
      const base::DictionaryValue* property = nullptr;
      if (properties->GetDictionary(i, &property)) {
        std::string name;
        if (property->GetString("name", &name) && name == "message") {
          if (property->GetString("value", &text)) {
            std::string class_name;
            if (exception->GetString("className", &class_name))
              text = "Uncaught " + class_name + ": " + text;
            break;
          }
        }
      }
    }
  } else {
    // Since |exception.preview.properties| is optional, fall back to |text|
    // (which is required) if we don't find anything.
    if (!exception_details->GetString("text", &text))
      return Status(kUnknownError, "missing or invalid exception message text");
  }

  log_->AddEntry(Log::kError, "javascript", base::StringPrintf(
      "%s %s %s", origin.c_str(), line_column.c_str(), text.c_str()));
  return Status(kOk);
}
