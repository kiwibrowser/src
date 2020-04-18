// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/webrtc_logging.h"

#include "base/time/time.h"
#include "content/public/renderer/webrtc_log_message_delegate.h"
#include "third_party/webrtc_overrides/rtc_base/logging.h"

namespace content {

// Shall only be set once and never go back to NULL.
WebRtcLogMessageDelegate* g_webrtc_logging_delegate = nullptr;

void InitWebRtcLoggingDelegate(WebRtcLogMessageDelegate* delegate) {
  CHECK(!g_webrtc_logging_delegate);
  CHECK(delegate);

  g_webrtc_logging_delegate = delegate;
}

void InitWebRtcLogging() {
  // Log messages from Libjingle should not have timestamps.
  rtc::InitDiagnosticLoggingDelegateFunction(&WebRtcLogMessage);
}

void WebRtcLogMessage(const std::string& message) {
  if (g_webrtc_logging_delegate)
    g_webrtc_logging_delegate->LogMessage(message);
}

}  // namespace content
