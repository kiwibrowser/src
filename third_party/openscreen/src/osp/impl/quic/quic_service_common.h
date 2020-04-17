// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_SERVICE_COMMON_H_
#define OSP_IMPL_QUIC_QUIC_SERVICE_COMMON_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "osp/impl/quic/quic_connection.h"
#include "osp/public/protocol_connection.h"

namespace openscreen {

class ServiceConnectionDelegate;

class QuicProtocolConnection final : public ProtocolConnection {
 public:
  class Owner {
   public:
    virtual ~Owner() = default;

    // Called right before |connection| is destroyed (destructor runs).
    virtual void OnConnectionDestroyed(QuicProtocolConnection* connection) = 0;
  };

  static std::unique_ptr<QuicProtocolConnection> FromExisting(
      Owner* owner,
      QuicConnection* connection,
      ServiceConnectionDelegate* delegate,
      uint64_t endpoint_id);

  QuicProtocolConnection(Owner* owner,
                         uint64_t endpoint_id,
                         uint64_t connection_id);
  ~QuicProtocolConnection() override;

  // ProtocolConnection overrides.
  void Write(const uint8_t* data, size_t data_size) override;
  void CloseWriteEnd() override;

  QuicStream* stream() { return stream_; }
  void set_stream(QuicStream* stream) { stream_ = stream; }

  void OnClose();

 private:
  Owner* const owner_;
  QuicStream* stream_ = nullptr;
};

struct ServiceStreamPair {
  ServiceStreamPair(std::unique_ptr<QuicStream> stream,
                    QuicProtocolConnection* protocol_connection);
  ~ServiceStreamPair();
  ServiceStreamPair(ServiceStreamPair&&);
  ServiceStreamPair& operator=(ServiceStreamPair&&);

  std::unique_ptr<QuicStream> stream;
  uint64_t connection_id;
  QuicProtocolConnection* protocol_connection;
};

class ServiceConnectionDelegate final : public QuicConnection::Delegate,
                                        public QuicStream::Delegate {
 public:
  class ServiceDelegate : public QuicProtocolConnection::Owner {
   public:
    ~ServiceDelegate() override = default;

    virtual uint64_t OnCryptoHandshakeComplete(
        ServiceConnectionDelegate* delegate,
        uint64_t connection_id) = 0;
    virtual void OnIncomingStream(
        std::unique_ptr<QuicProtocolConnection> connection) = 0;
    virtual void OnConnectionClosed(uint64_t endpoint_id,
                                    uint64_t connection_id) = 0;
    virtual void OnDataReceived(uint64_t endpoint_id,
                                uint64_t connection_id,
                                const uint8_t* data,
                                size_t data_size) = 0;
  };

  ServiceConnectionDelegate(ServiceDelegate* parent,
                            const IPEndpoint& endpoint);
  ~ServiceConnectionDelegate() override;

  void AddStreamPair(ServiceStreamPair&& stream_pair);
  void DropProtocolConnection(QuicProtocolConnection* connection);

  // This should be called at the end of each event loop that effects this
  // connection so streams that were closed by the other endpoint can be
  // destroyed properly.
  void DestroyClosedStreams();

  const IPEndpoint& endpoint() const { return endpoint_; }

  bool has_streams() const { return !streams_.empty(); }

  // QuicConnection::Delegate overrides.
  void OnCryptoHandshakeComplete(uint64_t connection_id) override;
  void OnIncomingStream(uint64_t connection_id,
                        std::unique_ptr<QuicStream> stream) override;
  void OnConnectionClosed(uint64_t connection_id) override;
  QuicStream::Delegate* NextStreamDelegate(uint64_t connection_id,
                                           uint64_t stream_id) override;

  // QuicStream::Delegate overrides.
  void OnReceived(QuicStream* stream,
                  const char* data,
                  size_t data_size) override;
  void OnClose(uint64_t stream_id) override;

 private:
  ServiceDelegate* const parent_;
  IPEndpoint endpoint_;
  uint64_t endpoint_id_;
  std::unique_ptr<QuicProtocolConnection> pending_connection_;
  std::map<uint64_t, ServiceStreamPair> streams_;
  std::vector<ServiceStreamPair> closed_streams_;
};

struct ServiceConnectionData {
  explicit ServiceConnectionData(
      std::unique_ptr<QuicConnection> connection,
      std::unique_ptr<ServiceConnectionDelegate> delegate);
  ServiceConnectionData(ServiceConnectionData&&);
  ~ServiceConnectionData();
  ServiceConnectionData& operator=(ServiceConnectionData&&);

  std::unique_ptr<QuicConnection> connection;
  std::unique_ptr<ServiceConnectionDelegate> delegate;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_SERVICE_COMMON_H_
