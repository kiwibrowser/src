// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_QUIC_QUIC_CLIENT_H_
#define OSP_IMPL_QUIC_QUIC_CLIENT_H_

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "osp/impl/quic/quic_connection_factory.h"
#include "osp/impl/quic/quic_service_common.h"
#include "osp/public/protocol_connection_client.h"
#include "osp_base/ip_address.h"

namespace openscreen {

// This class is the default implementation of ProtocolConnectionClient for the
// library.  It manages connections to other endpoints as well as the lifetime
// of each incoming and outgoing stream.  It works in conjunction with a
// QuicConnectionFactory implementation and MessageDemuxer.
// QuicConnectionFactory provides the actual ability to make a new QUIC
// connection with another endpoint.  Incoming data is given to the QuicClient
// by the underlying QUIC implementation (through QuicConnectionFactory) and
// this is in turn handed to MessageDemuxer for routing CBOR messages.
//
// The two most significant methods of this class are Connect and
// CreateProtocolConnection.  Both will return a new QUIC stream to a given
// endpoint to which the caller can write but the former is allowed to be
// asynchronous.  If there isn't currently a connection to the specified
// endpoint, Connect will start a connection attempt and store the callback for
// when the connection completes.  CreateProtocolConnection simply returns
// nullptr if there's no existing connection.
class QuicClient final : public ProtocolConnectionClient,
                         public ServiceConnectionDelegate::ServiceDelegate {
 public:
  QuicClient(MessageDemuxer* demuxer,
             std::unique_ptr<QuicConnectionFactory> connection_factory,
             ProtocolConnectionServiceObserver* observer);
  ~QuicClient() override;

  // ProtocolConnectionClient overrides.
  bool Start() override;
  bool Stop() override;
  void RunTasks() override;
  ConnectRequest Connect(const IPEndpoint& endpoint,
                         ConnectionRequestCallback* request) override;
  std::unique_ptr<ProtocolConnection> CreateProtocolConnection(
      uint64_t endpoint_id) override;

  // QuicProtocolConnection::Owner overrides.
  void OnConnectionDestroyed(QuicProtocolConnection* connection) override;

  // ServiceConnectionDelegate::ServiceDelegate overrides.
  uint64_t OnCryptoHandshakeComplete(ServiceConnectionDelegate* delegate,
                                     uint64_t connection_id) override;
  void OnIncomingStream(
      std::unique_ptr<QuicProtocolConnection> connection) override;
  void OnConnectionClosed(uint64_t endpoint_id,
                          uint64_t connection_id) override;
  void OnDataReceived(uint64_t endpoint_id,
                      uint64_t connection_id,
                      const uint8_t* data,
                      size_t data_size) override;

 private:
  struct PendingConnectionData {
    explicit PendingConnectionData(ServiceConnectionData&& data);
    PendingConnectionData(PendingConnectionData&&);
    ~PendingConnectionData();
    PendingConnectionData& operator=(PendingConnectionData&&);

    ServiceConnectionData data;

    // Pairs of request IDs and the associated connection callback.
    std::vector<std::pair<uint64_t, ConnectionRequestCallback*>> callbacks;
  };

  ConnectRequest CreatePendingConnection(const IPEndpoint& endpoint,
                                         ConnectionRequestCallback* request);
  uint64_t StartConnectionRequest(const IPEndpoint& endpoint,
                                  ConnectionRequestCallback* request);
  void CloseAllConnections();
  std::unique_ptr<QuicProtocolConnection> MakeProtocolConnection(
      QuicConnection* connection,
      ServiceConnectionDelegate* delegate,
      uint64_t endpoint_id);

  void CancelConnectRequest(uint64_t request_id) override;

  std::unique_ptr<QuicConnectionFactory> connection_factory_;

  // Maps an IPEndpoint to a generated endpoint ID.  This is used to insulate
  // callers from post-handshake changes to a connections actual peer endpoint.
  std::map<IPEndpoint, uint64_t, IPEndpointComparator> endpoint_map_;

  // Value that will be used for the next new endpoint in a Connect call.
  uint64_t next_endpoint_id_ = 0;

  // Maps request IDs to their callbacks.  The callback is paired with the
  // IPEndpoint it originally requested to connect to so cancelling the request
  // can also remove a pending connection.
  std::map<uint64_t, std::pair<IPEndpoint, ConnectionRequestCallback*>>
      request_map_;

  // Value that will be used for the next new connection request.
  uint64_t next_request_id_ = 1;

  // Maps endpoint addresses to data about connections that haven't successfully
  // completed the QUIC handshake.
  std::map<IPEndpoint, PendingConnectionData, IPEndpointComparator>
      pending_connections_;

  // Maps endpoint IDs to data about connections that have successfully
  // completed the QUIC handshake.
  std::map<uint64_t, ServiceConnectionData> connections_;

  // Connections that need to be destroyed, but have to wait for the next event
  // loop due to the underlying QUIC implementation's way of referencing them.
  std::vector<decltype(connections_)::iterator> delete_connections_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_QUIC_QUIC_CLIENT_H_
