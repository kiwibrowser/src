// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_WEBRTC_DATA_STREAM_ADAPTER_H_
#define REMOTING_PROTOCOL_WEBRTC_DATA_STREAM_ADAPTER_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "remoting/protocol/message_pipe.h"
#include "third_party/webrtc/api/peerconnectioninterface.h"
#include "third_party/webrtc/rtc_base/refcount.h"

namespace remoting {
namespace protocol {

// WebrtcDataStreamAdapter implements MessagePipe for WebRTC data channels.
class WebrtcDataStreamAdapter : public MessagePipe,
                                public webrtc::DataChannelObserver {
 public:
  explicit WebrtcDataStreamAdapter(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel);
  ~WebrtcDataStreamAdapter() override;

  std::string name() { return channel_->label(); }

  // MessagePipe interface.
  void Start(EventHandler* event_handler) override;
  void Send(google::protobuf::MessageLite* message,
            const base::Closure& done) override;

 private:
  enum class State { CONNECTING, OPEN, CLOSED };

  // webrtc::DataChannelObserver interface.
  void OnStateChange() override;
  void OnMessage(const webrtc::DataBuffer& buffer) override;

  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;

  EventHandler* event_handler_ = nullptr;

  State state_ = State::CONNECTING;

  DISALLOW_COPY_AND_ASSIGN(WebrtcDataStreamAdapter);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_WEBRTC_DATA_STREAM_ADAPTER_H_
