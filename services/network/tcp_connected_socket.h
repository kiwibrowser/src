// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TCP_CONNECTED_SOCKET_H_
#define SERVICES_NETWORK_TCP_CONNECTED_SOCKET_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "net/base/address_family.h"
#include "net/base/completion_callback.h"
#include "net/base/ip_endpoint.h"
#include "net/interfaces/address_family.mojom.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "net/socket/tcp_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/net_adapters.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/socket_data_pump.h"

namespace net {
class NetLog;
class StreamSocket;
class ClientSocketFactory;
class ClientSocketHandle;
}  // namespace net

namespace network {

class COMPONENT_EXPORT(NETWORK_SERVICE) TCPConnectedSocket
    : public mojom::TCPConnectedSocket,
      public SocketDataPump::Delegate {
 public:
  // Interface to handle a mojom::TLSClientSocketRequest.
  class Delegate {
   public:
    // Handles a mojom::TLSClientSocketRequest.
    virtual void CreateTLSClientSocket(
        const net::HostPortPair& host_port_pair,
        mojom::TLSClientSocketRequest request,
        std::unique_ptr<net::ClientSocketHandle> tcp_socket,
        mojom::SocketObserverPtr observer,
        const net::NetworkTrafficAnnotationTag& traffic_annotation,
        mojom::TCPConnectedSocket::UpgradeToTLSCallback callback) = 0;
  };
  TCPConnectedSocket(
      mojom::SocketObserverPtr observer,
      net::NetLog* net_log,
      Delegate* delegate,
      net::ClientSocketFactory* client_socket_factory,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  TCPConnectedSocket(
      mojom::SocketObserverPtr observer,
      std::unique_ptr<net::StreamSocket> socket,
      mojo::ScopedDataPipeProducerHandle receive_pipe_handle,
      mojo::ScopedDataPipeConsumerHandle send_pipe_handle,
      const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~TCPConnectedSocket() override;
  void Connect(
      const base::Optional<net::IPEndPoint>& local_addr,
      const net::AddressList& remote_addr_list,
      mojom::NetworkContext::CreateTCPConnectedSocketCallback callback);

  // mojom::TCPConnectedSocket implementation.
  void GetLocalAddress(GetLocalAddressCallback callback) override;

  void UpgradeToTLS(
      const net::HostPortPair& host_port_pair,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TLSClientSocketRequest request,
      mojom::SocketObserverPtr observer,
      UpgradeToTLSCallback callback) override;

 private:
  // Invoked when net::TCPClientSocket::Connect() completes.
  void OnConnectCompleted(int net_result);

  // SocketDataPump::Delegate implementation.
  void OnNetworkReadError(int net_error) override;
  void OnNetworkWriteError(int net_error) override;
  void OnShutdown() override;

  const mojom::SocketObserverPtr observer_;

  net::NetLog* const net_log_;
  Delegate* const delegate_;
  net::ClientSocketFactory* const client_socket_factory_;

  std::unique_ptr<net::StreamSocket> socket_;

  mojom::NetworkContext::CreateTCPConnectedSocketCallback connect_callback_;

  base::OnceClosure pending_upgrade_to_tls_callback_;

  std::unique_ptr<SocketDataPump> socket_data_pump_;

  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  DISALLOW_COPY_AND_ASSIGN(TCPConnectedSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TCP_CONNECTED_SOCKET_H_
