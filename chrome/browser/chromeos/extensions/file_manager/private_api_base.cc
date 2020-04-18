// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/file_manager/private_api_base.h"

#include <stdint.h>

#include "base/strings/string_number_conversions.h"
#include "chrome/browser/chromeos/extensions/file_manager/private_api_util.h"
#include "components/drive/event_logger.h"

namespace extensions {
namespace {

const int kSlowOperationThresholdMs = 500;  // In ms.

}  // namespace

LoggedAsyncExtensionFunction::LoggedAsyncExtensionFunction()
    : log_on_completion_(false) {
  start_time_  = base::Time::Now();
}

LoggedAsyncExtensionFunction::~LoggedAsyncExtensionFunction() {
}

void LoggedAsyncExtensionFunction::OnResponded() {
  drive::EventLogger* logger = file_manager::util::GetLogger(GetProfile());
  if (logger) {
    int64_t elapsed = (base::Time::Now() - start_time_).InMilliseconds();
    DCHECK(response_type());
    bool success = *response_type() == SUCCEEDED;
    if (log_on_completion_) {
      logger->Log(logging::LOG_INFO, "%s[%d] %s. (elapsed time: %sms)", name(),
                  request_id(), success ? "succeeded" : "failed",
                  base::Int64ToString(elapsed).c_str());
    } else if (elapsed >= kSlowOperationThresholdMs) {
      logger->Log(logging::LOG_WARNING,
                  "PEFORMANCE WARNING: %s[%d] was slow. (elapsed time: %sms)",
                  name(), request_id(), base::Int64ToString(elapsed).c_str());
    }
  }
  ChromeAsyncExtensionFunction::OnResponded();
}

}  // namespace extensions
