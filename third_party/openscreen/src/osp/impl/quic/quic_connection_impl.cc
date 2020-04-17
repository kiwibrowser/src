// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/quic/quic_connection_impl.h"

#include <limits>
#include <memory>

#include "absl/types/optional.h"
#include "osp/impl/quic/quic_connection_factory_impl.h"
#include "osp_base/error.h"
#include "platform/api/logging.h"
#include "third_party/chromium_quic/src/net/third_party/quic/platform/impl/quic_chromium_clock.h"

namespace openscreen {

UdpTransport::UdpTransport(platform::UdpSocket* socket,
                           const IPEndpoint& destination)
    : socket_(socket), destination_(destination) {
  OSP_DCHECK(socket_);
}

UdpTransport::UdpTransport(UdpTransport&&) noexcept = default;
UdpTransport::~UdpTransport() = default;

UdpTransport& UdpTransport::operator=(UdpTransport&&) noexcept = default;

int UdpTransport::Write(const char* buffer,
                        size_t buffer_length,
                        const PacketInfo& info) {
  switch (socket_->SendMessage(buffer, buffer_length, destination_).code()) {
    case Error::Code::kNone:
      OSP_DCHECK_LE(buffer_length,
                    static_cast<size_t>(std::numeric_limits<int>::max()));
      return static_cast<int>(buffer_length);
    case Error::Code::kAgain:
      return 0;
    default:
      return -1;
  }
}

QuicStreamImpl::QuicStreamImpl(QuicStream::Delegate* delegate,
                               ::quic::QuartcStream* stream)
    : QuicStream(delegate, stream->id()), stream_(stream) {
  stream_->SetDelegate(this);
}

QuicStreamImpl::~QuicStreamImpl() = default;

void QuicStreamImpl::Write(const uint8_t* data, size_t data_size) {
  OSP_DCHECK(!stream_->write_side_closed());
  stream_->WriteOrBufferData(
      ::quic::QuicStringPiece(reinterpret_cast<const char*>(data), data_size),
      false, nullptr);
}

void QuicStreamImpl::CloseWriteEnd() {
  if (!stream_->write_side_closed())
    stream_->FinishWriting();
}

void QuicStreamImpl::OnReceived(::quic::QuartcStream* stream,
                                const char* data,
                                size_t data_size) {
  delegate_->OnReceived(this, data, data_size);
}

void QuicStreamImpl::OnClose(::quic::QuartcStream* stream) {
  delegate_->OnClose(stream->id());
}

void QuicStreamImpl::OnBufferChanged(::quic::QuartcStream* stream) {}

QuicConnectionImpl::QuicConnectionImpl(
    QuicConnectionFactoryImpl* parent_factory,
    QuicConnection::Delegate* delegate,
    std::unique_ptr<UdpTransport> udp_transport,
    std::unique_ptr<::quic::QuartcSession> session)
    : QuicConnection(delegate),
      parent_factory_(parent_factory),
      session_(std::move(session)),
      udp_transport_(std::move(udp_transport)) {
  session_->SetDelegate(this);
  session_->OnTransportCanWrite();
  session_->StartCryptoHandshake();
}

QuicConnectionImpl::~QuicConnectionImpl() = default;

void QuicConnectionImpl::OnDataReceived(const platform::ReceivedData& data) {
  session_->OnTransportReceived(
      reinterpret_cast<const char*>(data.bytes.data()), data.length);
}

std::unique_ptr<QuicStream> QuicConnectionImpl::MakeOutgoingStream(
    QuicStream::Delegate* delegate) {
  ::quic::QuartcStream* stream = session_->CreateOutgoingDynamicStream();
  return std::make_unique<QuicStreamImpl>(delegate, stream);
}

void QuicConnectionImpl::Close() {
  session_->CloseConnection("closed");
}

void QuicConnectionImpl::OnCryptoHandshakeComplete() {
  delegate_->OnCryptoHandshakeComplete(session_->connection_id());
}

void QuicConnectionImpl::OnIncomingStream(::quic::QuartcStream* stream) {
  auto public_stream = std::make_unique<QuicStreamImpl>(
      delegate_->NextStreamDelegate(session_->connection_id(), stream->id()),
      stream);
  streams_.push_back(public_stream.get());
  delegate_->OnIncomingStream(session_->connection_id(),
                              std::move(public_stream));
}

void QuicConnectionImpl::OnConnectionClosed(
    ::quic::QuicErrorCode error_code,
    const ::quic::QuicString& error_details,
    ::quic::ConnectionCloseSource source) {
  parent_factory_->OnConnectionClosed(this);
  delegate_->OnConnectionClosed(session_->connection_id());
}

}  // namespace openscreen
