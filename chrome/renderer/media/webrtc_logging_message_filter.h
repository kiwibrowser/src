// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_WEBRTC_LOGGING_MESSAGE_FILTER_H_
#define CHROME_RENDERER_MEDIA_WEBRTC_LOGGING_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "chrome/common/media/webrtc_logging_message_data.h"
#include "ipc/message_filter.h"

class ChromeWebRtcLogMessageDelegate;

// Filter for WebRTC logging messages. Sits between
// ChromeWebRtcLogMessageDelegate (renderer process) and
// WebRtcLoggingHandlerHost (browser process). Must be called on the IO thread.
class WebRtcLoggingMessageFilter : public IPC::MessageFilter {
 public:
  explicit WebRtcLoggingMessageFilter(
      const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner);

  virtual void AddLogMessages(
      const std::vector<WebRtcLoggingMessageData>& messages);
  virtual void LoggingStopped();

  const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner() {
    return io_task_runner_;
  }

 protected:
  ~WebRtcLoggingMessageFilter() override;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Owned by this class. The only other pointer to it is in libjingle's logging
  // file. That's a global pointer used on different threads, so we will leak
  // this object when we go away to ensure that it outlives any log messages
  // coming from libjingle.
  // This is protected for unit test purposes.
  // TODO(vrk): Remove ChromeWebRtcLogMessageDelegate's pointer to
  // WebRtcLoggingMessageFilter so that we can write a unit test that doesn't
  // need this accessor.
  ChromeWebRtcLogMessageDelegate* log_message_delegate_;

 private:
  // IPC::MessageFilter implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnFilterAdded(IPC::Channel* channel) override;
  void OnFilterRemoved() override;
  void OnChannelClosing() override;

  void CreateLoggingHandler();

  void OnStartLogging();
  void OnStopLogging();
  void Send(IPC::Message* message);

  IPC::Sender* sender_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcLoggingMessageFilter);
};

#endif  // CHROME_RENDERER_MEDIA_WEBRTC_LOGGING_MESSAGE_FILTER_H_
