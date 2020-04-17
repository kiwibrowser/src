// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quiche/src/quic/core/http/quic_receive_control_stream.h"

#include "net/third_party/quiche/src/quic/core/http/quic_spdy_session.h"

namespace quic {

// Visitor of HttpDecoder that passes data frame to QuicSpdyStream and closes
// the connection on unexpected frames.
class QuicReceiveControlStream::HttpDecoderVisitor
    : public HttpDecoder::Visitor {
 public:
  explicit HttpDecoderVisitor(QuicReceiveControlStream* stream)
      : stream_(stream) {}
  HttpDecoderVisitor(const HttpDecoderVisitor&) = delete;
  HttpDecoderVisitor& operator=(const HttpDecoderVisitor&) = delete;

  void OnError(HttpDecoder* decoder) override {
    stream_->session()->connection()->CloseConnection(
        QUIC_HTTP_DECODER_ERROR, "Http decoder internal error",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
  }

  bool OnPriorityFrame(const PriorityFrame& frame) override {
    CloseConnectionOnWrongFrame("Priority");
    return false;
  }

  bool OnCancelPushFrame(const CancelPushFrame& frame) override {
    CloseConnectionOnWrongFrame("Cancel Push");
    return false;
  }

  bool OnMaxPushIdFrame(const MaxPushIdFrame& frame) override {
    CloseConnectionOnWrongFrame("Max Push Id");
    return false;
  }

  bool OnGoAwayFrame(const GoAwayFrame& frame) override {
    CloseConnectionOnWrongFrame("Goaway");
    return false;
  }

  bool OnSettingsFrameStart(Http3FrameLengths frame_lengths) override {
    return stream_->OnSettingsFrameStart(frame_lengths);
  }

  bool OnSettingsFrame(const SettingsFrame& frame) override {
    return stream_->OnSettingsFrame(frame);
  }

  bool OnDuplicatePushFrame(const DuplicatePushFrame& frame) override {
    CloseConnectionOnWrongFrame("Duplicate Push");
    return false;
  }

  bool OnDataFrameStart(Http3FrameLengths frame_lengths) override {
    CloseConnectionOnWrongFrame("Data");
    return false;
  }

  bool OnDataFramePayload(QuicStringPiece payload) override {
    CloseConnectionOnWrongFrame("Data");
    return false;
  }

  bool OnDataFrameEnd() override {
    CloseConnectionOnWrongFrame("Data");
    return false;
  }

  bool OnHeadersFrameStart(Http3FrameLengths frame_length) override {
    CloseConnectionOnWrongFrame("Headers");
    return false;
  }

  bool OnHeadersFramePayload(QuicStringPiece payload) override {
    CloseConnectionOnWrongFrame("Headers");
    return false;
  }

  bool OnHeadersFrameEnd() override {
    CloseConnectionOnWrongFrame("Headers");
    return false;
  }

  bool OnPushPromiseFrameStart(PushId push_id) override {
    CloseConnectionOnWrongFrame("Push Promise");
    return false;
  }

  bool OnPushPromiseFramePayload(QuicStringPiece payload) override {
    CloseConnectionOnWrongFrame("Push Promise");
    return false;
  }

  bool OnPushPromiseFrameEnd() override {
    CloseConnectionOnWrongFrame("Push Promise");
    return false;
  }

 private:
  void CloseConnectionOnWrongFrame(std::string frame_type) {
    // TODO(renjietang): Change to HTTP/3 error type.
    stream_->session()->connection()->CloseConnection(
        QUIC_HTTP_DECODER_ERROR,
        frame_type + " frame received on control stream",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
  }

  QuicReceiveControlStream* stream_;
};

QuicReceiveControlStream::QuicReceiveControlStream(QuicStreamId id,
                                                   QuicSpdySession* session)
    : QuicStream(id, session, /*is_static = */ true, READ_UNIDIRECTIONAL),
      received_settings_length_(0),
      http_decoder_visitor_(new HttpDecoderVisitor(this)) {
  decoder_.set_visitor(http_decoder_visitor_.get());
  sequencer()->set_level_triggered(true);
}

QuicReceiveControlStream::QuicReceiveControlStream(PendingStream* pending)
    : QuicStream(pending, READ_UNIDIRECTIONAL, /*is_static=*/true),
      received_settings_length_(0),
      http_decoder_visitor_(new HttpDecoderVisitor(this)) {
  decoder_.set_visitor(http_decoder_visitor_.get());
  sequencer()->set_level_triggered(true);
}

QuicReceiveControlStream::~QuicReceiveControlStream() {}

void QuicReceiveControlStream::OnStreamReset(const QuicRstStreamFrame& frame) {
  // TODO(renjietang) Change the error code to H/3 specific
  // HTTP_CLOSED_CRITICAL_STREAM.
  session()->connection()->CloseConnection(
      QUIC_INVALID_STREAM_ID, "Attempt to reset receive control stream",
      ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
}

void QuicReceiveControlStream::OnDataAvailable() {
  iovec iov;
  while (!reading_stopped() && sequencer()->PrefetchNextRegion(&iov)) {
    decoder_.ProcessInput(reinterpret_cast<const char*>(iov.iov_base),
                          iov.iov_len);
  }
}

bool QuicReceiveControlStream::OnSettingsFrameStart(
    Http3FrameLengths frame_lengths) {
  if (received_settings_length_ != 0) {
    // TODO(renjietang): Change error code to HTTP_UNEXPECTED_FRAME.
    session()->connection()->CloseConnection(
        QUIC_INVALID_STREAM_ID, "Settings frames are received twice.",
        ConnectionCloseBehavior::SEND_CONNECTION_CLOSE_PACKET);
    return false;
  }
  received_settings_length_ +=
      frame_lengths.header_length + frame_lengths.payload_length;
  return true;
}

bool QuicReceiveControlStream::OnSettingsFrame(const SettingsFrame& settings) {
  QuicSpdySession* spdy_session = static_cast<QuicSpdySession*>(session());
  for (auto& it : settings.values) {
    uint64_t setting_id = it.first;
    switch (setting_id) {
      case kSettingsMaxHeaderListSize:
        spdy_session->set_max_outbound_header_list_size(it.second);
        break;
      case kSettingsNumPlaceholders:
        // TODO: Support placeholder setting
        break;
      default:
        break;
    }
  }
  sequencer()->MarkConsumed(received_settings_length_);
  return true;
}

}  // namespace quic
