// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_MEDIA_WEBRTC_LOGGING_MESSAGE_DATA_H_
#define CHROME_COMMON_MEDIA_WEBRTC_LOGGING_MESSAGE_DATA_H_

#include <string>

#include "base/time/time.h"

// A struct representing a logging message with its creation time.
struct WebRtcLoggingMessageData {
  WebRtcLoggingMessageData();
  WebRtcLoggingMessageData(base::Time time, const std::string& message);

  // Returns a string formatted as "[XXX:YYY] $message", where "[XXX:YYY]" is
  // the timestamp relative to |start_time| converted to seconds (XXX) plus
  // milliseconds (YYY).
  static std::string Format(const std::string& message, base::Time timestamp,
                            base::Time start_time);
  std::string Format(base::Time start_time) const;

  base::Time timestamp;
  std::string message;
};

#endif  // CHROME_COMMON_MEDIA_WEBRTC_LOGGING_MESSAGE_DATA_H_
