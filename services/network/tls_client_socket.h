// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TLS_CLIENT_SOCKET_H_
#define SERVICES_NETWORK_TLS_CLIENT_SOCKET_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "net/base/address_family.h"
#include "net/interfaces/address_family.mojom.h"
#include "net/interfaces/ip_endpoint.mojom.h"
#include "net/socket/ssl_client_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/public/mojom/tls_socket.mojom.h"
#include "services/network/socket_data_pump.h"

namespace net {
class SSLClientSocket;
class ClientSocketHandle;
class ClientSocketFactory;
}  // namespace net

namespace network {

class COMPONENT_EXPORT(NETWORK_SERVICE) TLSClientSocket
    : public mojom::TLSClientSocket,
      public SocketDataPump::Delegate {
 public:
  TLSClientSocket(mojom::TLSClientSocketRequest request,
                  mojom::SocketObserverPtr observer,
                  const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~TLSClientSocket() override;

  void Connect(const net::HostPortPair& host_port_pair,
               const net::SSLConfig& ssl_config,
               std::unique_ptr<net::ClientSocketHandle> tcp_socket,
               const net::SSLClientSocketContext& ssl_client_socket_context,
               net::ClientSocketFactory* socket_factory,
               mojom::TCPConnectedSocket::UpgradeToTLSCallback callback);

 private:
  void OnTLSConnectCompleted(int result);

  // SocketDataPump::Delegate implementation.
  void OnNetworkReadError(int net_error) override;
  void OnNetworkWriteError(int net_error) override;
  void OnShutdown() override;

  const mojom::SocketObserverPtr observer_;
  std::unique_ptr<SocketDataPump> socket_data_pump_;
  std::unique_ptr<net::SSLClientSocket> socket_;
  mojom::TCPConnectedSocket::UpgradeToTLSCallback connect_callback_;
  const net::NetworkTrafficAnnotationTag traffic_annotation_;

  DISALLOW_COPY_AND_ASSIGN(TLSClientSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TLS_CLIENT_SOCKET_H_
