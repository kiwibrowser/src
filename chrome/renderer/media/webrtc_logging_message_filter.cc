// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/webrtc_logging_message_filter.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "chrome/common/media/webrtc_logging_messages.h"
#include "chrome/renderer/media/chrome_webrtc_log_message_delegate.h"
#include "ipc/ipc_logging.h"

WebRtcLoggingMessageFilter::WebRtcLoggingMessageFilter(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : io_task_runner_(io_task_runner),
      log_message_delegate_(NULL),
      sender_(NULL) {
  // May be null in a browsertest using MockRenderThread.
  if (io_task_runner_.get()) {
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&WebRtcLoggingMessageFilter::CreateLoggingHandler,
                       base::Unretained(this)));
  }
}

WebRtcLoggingMessageFilter::~WebRtcLoggingMessageFilter() {
}

bool WebRtcLoggingMessageFilter::OnMessageReceived(
    const IPC::Message& message) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebRtcLoggingMessageFilter, message)
    IPC_MESSAGE_HANDLER(WebRtcLoggingMsg_StartLogging, OnStartLogging)
    IPC_MESSAGE_HANDLER(WebRtcLoggingMsg_StopLogging, OnStopLogging)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void WebRtcLoggingMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  sender_ = channel;
}

void WebRtcLoggingMessageFilter::OnFilterRemoved() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  sender_ = NULL;
  log_message_delegate_->OnFilterRemoved();
}

void WebRtcLoggingMessageFilter::OnChannelClosing() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  sender_ = NULL;
  log_message_delegate_->OnFilterRemoved();
}

void WebRtcLoggingMessageFilter::AddLogMessages(
    const std::vector<WebRtcLoggingMessageData>& messages) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  Send(new WebRtcLoggingMsg_AddLogMessages(messages));
}

void WebRtcLoggingMessageFilter::LoggingStopped() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  Send(new WebRtcLoggingMsg_LoggingStopped());
}

void WebRtcLoggingMessageFilter::CreateLoggingHandler() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  log_message_delegate_ =
      new ChromeWebRtcLogMessageDelegate(io_task_runner_, this);
}

void WebRtcLoggingMessageFilter::OnStartLogging() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  log_message_delegate_->OnStartLogging();
}

void WebRtcLoggingMessageFilter::OnStopLogging() {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  log_message_delegate_->OnStopLogging();
}

void WebRtcLoggingMessageFilter::Send(IPC::Message* message) {
  DCHECK(!io_task_runner_.get() || io_task_runner_->BelongsToCurrentThread());
  if (!sender_) {
    DLOG(ERROR) << "IPC sender not available.";
    delete message;
  } else {
    sender_->Send(message);
  }
}
