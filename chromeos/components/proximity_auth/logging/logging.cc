// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/proximity_auth/logging/logging.h"

#include "chromeos/components/proximity_auth/logging/log_buffer.h"

namespace proximity_auth {
namespace {

bool g_logging_enabled = true;

}  // namespace

ScopedDisableLoggingForTesting::ScopedDisableLoggingForTesting() {
  g_logging_enabled = false;
}

ScopedDisableLoggingForTesting::~ScopedDisableLoggingForTesting() {
  g_logging_enabled = true;
}

ScopedLogMessage::ScopedLogMessage(const char* file,
                                   int line,
                                   logging::LogSeverity severity)
    : file_(file), line_(line), severity_(severity) {}

ScopedLogMessage::~ScopedLogMessage() {
  if (!g_logging_enabled)
    return;

  LogBuffer::GetInstance()->AddLogMessage(LogBuffer::LogMessage(
      stream_.str(), base::Time::Now(), file_, line_, severity_));

  // The destructor of |log_message| also creates a log for the standard logging
  // system.
  logging::LogMessage log_message(file_, line_, severity_);
  log_message.stream() << stream_.str();
}

}  // namespace proximity_auth
