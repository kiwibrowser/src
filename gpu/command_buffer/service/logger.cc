// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/logger.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "gpu/command_buffer/common/debug_marker_manager.h"
#include "gpu/command_buffer/service/decoder_client.h"
#include "gpu/command_buffer/service/gpu_switches.h"

namespace gpu {
namespace gles2 {

Logger::Logger(const DebugMarkerManager* debug_marker_manager,
               DecoderClient* client)
    : debug_marker_manager_(debug_marker_manager),
      client_(client),
      log_message_count_(0),
      log_synthesized_gl_errors_(true) {
  Logger* this_temp = this;
  this_in_hex_ = std::string("GroupMarkerNotSet(crbug.com/242999)!:") +
      base::HexEncode(&this_temp, sizeof(this_temp));
}

Logger::~Logger() = default;

void Logger::LogMessage(
    const char* filename, int line, const std::string& msg) {
  if (log_message_count_ < kMaxLogMessages ||
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableGLErrorLimit)) {
    std::string prefixed_msg(std::string("[") + GetLogPrefix() + "]" + msg);
    ++log_message_count_;
    // LOG this unless logging is turned off as any chromium code that
    // generates these errors probably has a bug.
    if (log_synthesized_gl_errors_) {
      ::logging::LogMessage(
          filename, line, ::logging::LOG_ERROR).stream() << prefixed_msg;
    }
    client_->OnConsoleMessage(0, prefixed_msg);
  } else {
    if (log_message_count_ == kMaxLogMessages) {
      ++log_message_count_;
      LOG(ERROR)
          << "Too many GL errors, not reporting any more for this context."
          << " use --disable-gl-error-limit to see all errors.";
    }
  }
}

const std::string& Logger::GetLogPrefix() const {
  const std::string& prefix(debug_marker_manager_->GetMarker());
  return prefix.empty() ? this_in_hex_ : prefix;
}

}  // namespace gles2
}  // namespace gpu
