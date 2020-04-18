// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_RENDERER_WEBRTC_LOG_MESSAGE_DELEGATE_H_
#define CONTENT_PUBLIC_RENDERER_WEBRTC_LOG_MESSAGE_DELEGATE_H_

#include <string>

#include "content/common/content_export.h"

namespace content {

// This interface is implemented by a handler in the embedder and used for
// initializing the logging and passing log messages to the handler. The
// purpose is to forward mainly libjingle log messages to embedder (besides
// the ordinary logging stream) that will be used for diagnostic purposes.
class WebRtcLogMessageDelegate {
 public:
  // Pass a diagnostic WebRTC log message.
  virtual void LogMessage(const std::string& message) = 0;

 protected:
  virtual ~WebRtcLogMessageDelegate() {}
};

// Must be called on IO thread.
CONTENT_EXPORT void InitWebRtcLoggingDelegate(
    WebRtcLogMessageDelegate* delegate);

// Must be called on IO thread.
CONTENT_EXPORT void InitWebRtcLogging();

}  // namespace content

#endif  // CONTENT_PUBLIC_RENDERER_WEBRTC_LOG_MESSAGE_DELEGATE_H_
