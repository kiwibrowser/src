// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_MOCK_WEBRTC_LOGGING_MESSAGE_FILTER_H_
#define CHROME_RENDERER_MEDIA_MOCK_WEBRTC_LOGGING_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "chrome/renderer/media/webrtc_logging_message_filter.h"

class MockWebRtcLoggingMessageFilter
    : public WebRtcLoggingMessageFilter {
 public:
  explicit MockWebRtcLoggingMessageFilter(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);

  void AddLogMessages(
      const std::vector<WebRtcLoggingMessageData>& messages) override;
  void LoggingStopped() override;

  ChromeWebRtcLogMessageDelegate* log_message_delegate() {
    return log_message_delegate_;
  }

  std::string log_buffer_;
  bool logging_stopped_;

 protected:
  ~MockWebRtcLoggingMessageFilter() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWebRtcLoggingMessageFilter);
};

#endif  // CHROME_RENDERER_MEDIA_MOCK_WEBRTC_LOGGING_MESSAGE_FILTER_H_
