// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/webrtc_data_stream_adapter.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "remoting/base/compound_buffer.h"
#include "remoting/protocol/message_serialization.h"

namespace remoting {
namespace protocol {

WebrtcDataStreamAdapter::WebrtcDataStreamAdapter(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
    : channel_(channel.get()) {
  channel_->RegisterObserver(this);
  DCHECK_EQ(channel_->state(), webrtc::DataChannelInterface::kConnecting);
}

WebrtcDataStreamAdapter::~WebrtcDataStreamAdapter() {
  if (channel_) {
    channel_->UnregisterObserver();
    channel_->Close();

    // Destroy |channel_| asynchronously as it may be on stack.
    channel_->AddRef();
    base::ThreadTaskRunnerHandle::Get()->ReleaseSoon(FROM_HERE, channel_.get());
    channel_ = nullptr;
  }
}

void WebrtcDataStreamAdapter::Start(EventHandler* event_handler) {
  DCHECK(!event_handler_);
  DCHECK(event_handler);

  event_handler_ = event_handler;
}

void WebrtcDataStreamAdapter::Send(google::protobuf::MessageLite* message,
                                   const base::Closure& done) {
  DCHECK(state_ == State::OPEN);

  rtc::CopyOnWriteBuffer buffer;
  buffer.SetSize(message->ByteSize());
  message->SerializeWithCachedSizesToArray(
      reinterpret_cast<uint8_t*>(buffer.data()));
  webrtc::DataBuffer data_buffer(std::move(buffer), true /* binary */);
  if (!channel_->Send(data_buffer)) {
    LOG(ERROR) << "Send failed on data channel " << channel_->label();
    channel_->Close();
    return;
  }

  if (!done.is_null())
    done.Run();
}

void WebrtcDataStreamAdapter::OnStateChange() {
  switch (channel_->state()) {
    case webrtc::DataChannelInterface::kOpen:
      DCHECK(state_ == State::CONNECTING);
      state_ = State::OPEN;
      event_handler_->OnMessagePipeOpen();
      break;

    case webrtc::DataChannelInterface::kClosing:
      if (state_ != State::CLOSED) {
        state_ = State::CLOSED;
        event_handler_->OnMessagePipeClosed();
      }
      break;

    case webrtc::DataChannelInterface::kConnecting:
    case webrtc::DataChannelInterface::kClosed:
      break;
  }
}

void WebrtcDataStreamAdapter::OnMessage(const webrtc::DataBuffer& rtc_buffer) {
  if (state_ != State::OPEN) {
    LOG(ERROR) << "Dropping a message received when the channel is not open.";
    return;
  }

  std::unique_ptr<CompoundBuffer> buffer(new CompoundBuffer());
  buffer->AppendCopyOf(reinterpret_cast<const char*>(rtc_buffer.data.data()),
                       rtc_buffer.data.size());
  buffer->Lock();
  event_handler_->OnMessageReceived(std::move(buffer));
}

}  // namespace protocol
}  // namespace remoting
