// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/chrome_webrtc_log_message_delegate.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "chrome/renderer/media/webrtc_logging_message_filter.h"
#include "components/webrtc_logging/common/partial_circular_buffer.h"

ChromeWebRtcLogMessageDelegate::ChromeWebRtcLogMessageDelegate(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    WebRtcLoggingMessageFilter* message_filter)
    : io_task_runner_(io_task_runner),
      logging_started_(false),
      message_filter_(message_filter) {
  content::InitWebRtcLoggingDelegate(this);
}

ChromeWebRtcLogMessageDelegate::~ChromeWebRtcLogMessageDelegate() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void ChromeWebRtcLogMessageDelegate::LogMessage(const std::string& message) {
  WebRtcLoggingMessageData data(base::Time::Now(), message);

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ChromeWebRtcLogMessageDelegate::LogMessageOnIOThread,
                     base::Unretained(this), data));
}

void ChromeWebRtcLogMessageDelegate::LogMessageOnIOThread(
    const WebRtcLoggingMessageData& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (logging_started_ && message_filter_) {
    if (!log_buffer_.empty()) {
      // A delayed task has already been posted for sending the buffer contents.
      // Just add the message to the buffer.
      log_buffer_.push_back(message);
      return;
    }

    log_buffer_.push_back(message);

    if (base::TimeTicks::Now() - last_log_buffer_send_ >
        base::TimeDelta::FromMilliseconds(100)) {
      SendLogBuffer();
    } else {
      io_task_runner_->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&ChromeWebRtcLogMessageDelegate::SendLogBuffer,
                         base::Unretained(this)),
          base::TimeDelta::FromMilliseconds(200));
    }
  }
}

void ChromeWebRtcLogMessageDelegate::OnFilterRemoved() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  message_filter_ = NULL;
}

void ChromeWebRtcLogMessageDelegate::OnStartLogging() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  logging_started_ = true;
  content::InitWebRtcLogging();
}

void ChromeWebRtcLogMessageDelegate::OnStopLogging() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!log_buffer_.empty())
    SendLogBuffer();
  if (message_filter_)
    message_filter_->LoggingStopped();
  logging_started_ = false;
}

void ChromeWebRtcLogMessageDelegate::SendLogBuffer() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (logging_started_ && message_filter_) {
    message_filter_->AddLogMessages(log_buffer_);
    last_log_buffer_send_ = base::TimeTicks::Now();
  }
  log_buffer_.clear();
}
