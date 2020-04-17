// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/api/logging.h"

namespace openscreen {
namespace platform {

LogMessage::LogMessage(LogLevel level, absl::string_view file, int line)
    : level_(level), file_(file), line_(line) {}

LogMessage::~LogMessage() {
  LogWithLevel(level_, file_, line_, stream_.str().c_str());
  if (level_ == LogLevel::kFatal)
    Break();
}

std::string LogLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::kVerbose:
      return "VERBOSE";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarning:
      return "WARNING";
    case LogLevel::kError:
      return "ERROR";
    default:
      return "";
  }
}

}  // namespace platform
}  // namespace openscreen
