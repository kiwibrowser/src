// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_CONNECTION_IMPL_H_
#define OSP_IMPL_QUIC_QUIC_CONNECTION_IMPL_H_

#include <list>
#include <memory>

#include "osp/impl/quic/quic_connection.h"
#include "osp_base/ip_address.h"
#include "platform/api/udp_socket.h"
#include "third_party/chromium_quic/src/base/callback.h"
#include "third_party/chromium_quic/src/base/location.h"
#include "third_party/chromium_quic/src/base/task_runner.h"
#include "third_party/chromium_quic/src/base/time/time.h"
#include "third_party/chromium_quic/src/net/third_party/quic/quartc/quartc_packet_writer.h"
#include "third_party/chromium_quic/src/net/third_party/quic/quartc/quartc_session.h"
#include "third_party/chromium_quic/src/net/third_party/quic/quartc/quartc_stream.h"

namespace openscreen {

class QuicConnectionFactoryImpl;

class UdpTransport final : public ::quic::QuartcPacketTransport {
 public:
  UdpTransport(platform::UdpSocket* socket, const IPEndpoint& destination);
  UdpTransport(UdpTransport&&) noexcept;
  ~UdpTransport() override;

  UdpTransport& operator=(UdpTransport&&) noexcept;

  // ::quic::QuartcPacketTransport overrides.
  int Write(const char* buffer,
            size_t buffer_length,
            const PacketInfo& info) override;

  platform::UdpSocket* socket() const { return socket_; }

 private:
  platform::UdpSocket* socket_;
  IPEndpoint destination_;
};

class QuicStreamImpl final : public QuicStream,
                             public ::quic::QuartcStream::Delegate {
 public:
  QuicStreamImpl(QuicStream::Delegate* delegate, ::quic::QuartcStream* stream);
  ~QuicStreamImpl() override;

  // QuicStream overrides.
  void Write(const uint8_t* data, size_t size) override;
  void CloseWriteEnd() override;

  // ::quic::QuartcStream::Delegate overrides.
  void OnReceived(::quic::QuartcStream* stream,
                  const char* data,
                  size_t data_size) override;
  void OnClose(::quic::QuartcStream* stream) override;
  void OnBufferChanged(::quic::QuartcStream* stream) override;

 private:
  ::quic::QuartcStream* const stream_;
};

class QuicConnectionImpl final : public QuicConnection,
                                 public ::quic::QuartcSession::Delegate {
 public:
  QuicConnectionImpl(QuicConnectionFactoryImpl* parent_factory,
                     QuicConnection::Delegate* delegate,
                     std::unique_ptr<UdpTransport> udp_transport,
                     std::unique_ptr<::quic::QuartcSession> session);

  ~QuicConnectionImpl() override;

  // QuicConnection overrides.
  void OnDataReceived(const platform::ReceivedData& data) override;
  std::unique_ptr<QuicStream> MakeOutgoingStream(
      QuicStream::Delegate* delegate) override;
  void Close() override;

  // ::quic::QuartcSession::Delegate overrides.
  void OnCryptoHandshakeComplete() override;
  void OnIncomingStream(::quic::QuartcStream* stream) override;
  void OnConnectionClosed(::quic::QuicErrorCode error_code,
                          const ::quic::QuicString& error_details,
                          ::quic::ConnectionCloseSource source) override;

 private:
  QuicConnectionFactoryImpl* const parent_factory_;
  const std::unique_ptr<::quic::QuartcSession> session_;
  const std::unique_ptr<UdpTransport> udp_transport_;
  std::vector<QuicStream*> streams_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_CONNECTION_IMPL_H_
