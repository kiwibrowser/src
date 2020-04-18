// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/media/webrtc_logging_message_data.h"

#include <stdint.h>

#include "base/strings/stringprintf.h"

WebRtcLoggingMessageData::WebRtcLoggingMessageData() {}

WebRtcLoggingMessageData::WebRtcLoggingMessageData(base::Time time,
                                                   const std::string& message)
    : timestamp(time), message(message) {}

// static
std::string WebRtcLoggingMessageData::Format(const std::string& message,
                                             base::Time timestamp,
                                             base::Time start_time) {
  int32_t interval_ms =
      static_cast<int32_t>((timestamp - start_time).InMilliseconds());
  return base::StringPrintf("[%03d:%03d] %s", interval_ms / 1000,
                            interval_ms % 1000, message.c_str());
}

std::string WebRtcLoggingMessageData::Format(base::Time start_time) const {
  return Format(message, timestamp, start_time);
}
