// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/mock_webrtc_logging_message_filter.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/single_thread_task_runner.h"

MockWebRtcLoggingMessageFilter::MockWebRtcLoggingMessageFilter(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : WebRtcLoggingMessageFilter(io_task_runner),
      logging_stopped_(false) {
}

MockWebRtcLoggingMessageFilter::~MockWebRtcLoggingMessageFilter() {
}

void MockWebRtcLoggingMessageFilter::AddLogMessages(
    const std::vector<WebRtcLoggingMessageData>& messages) {
  CHECK(io_task_runner_->BelongsToCurrentThread());
  for (size_t i = 0; i < messages.size(); ++i)
    log_buffer_ += messages[i].message + "\n";
}

void MockWebRtcLoggingMessageFilter::LoggingStopped() {
  CHECK(io_task_runner_->BelongsToCurrentThread());
  logging_stopped_ = true;
}
