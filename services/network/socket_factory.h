// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_SOCKET_FACTORY_H_
#define SERVICES_NETWORK_SOCKET_FACTORY_H_

#include <memory>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/public/mojom/tls_socket.mojom.h"
#include "services/network/public/mojom/udp_socket.mojom.h"
#include "services/network/tcp_connected_socket.h"
#include "services/network/tcp_server_socket.h"

namespace net {
class ClientSocketHandle;
class ClientSocketFactory;
class NetLog;
class SSLConfigService;
}  // namespace net

namespace network {

// Helper class that handles socket requests. It takes care of destroying
// socket implementation instances when mojo  pipes are broken.
class COMPONENT_EXPORT(NETWORK_SERVICE) SocketFactory
    : public TCPServerSocket::Delegate,
      public TCPConnectedSocket::Delegate {
 public:
  // Constructs a SocketFactory. If |net_log| is non-null, it is used to
  // log NetLog events when logging is enabled. |net_log| used to must outlive
  // |this|.
  SocketFactory(net::NetLog* net_log,
                net::URLRequestContext* url_request_context);
  virtual ~SocketFactory();

  void CreateUDPSocket(mojom::UDPSocketRequest request,
                       mojom::UDPSocketReceiverPtr receiver);
  void CreateTCPServerSocket(
      const net::IPEndPoint& local_addr,
      int backlog,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPServerSocketRequest request,
      mojom::NetworkContext::CreateTCPServerSocketCallback callback);
  void CreateTCPConnectedSocket(
      const base::Optional<net::IPEndPoint>& local_addr,
      const net::AddressList& remote_addr_list,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPConnectedSocketRequest request,
      mojom::SocketObserverPtr observer,
      mojom::NetworkContext::CreateTCPConnectedSocketCallback callback);

 private:
  // TCPServerSocket::Delegate implementation:
  void OnAccept(std::unique_ptr<TCPConnectedSocket> socket,
                mojom::TCPConnectedSocketRequest request) override;

  // TCPConnectedSocket::Delegate implementation:
  void CreateTLSClientSocket(
      const net::HostPortPair& host_port_pair,
      mojom::TLSClientSocketRequest request,
      std::unique_ptr<net::ClientSocketHandle> tcp_socket,
      mojom::SocketObserverPtr observer,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPConnectedSocket::UpgradeToTLSCallback callback) override;

  net::NetLog* const net_log_;
  const net::SSLClientSocketContext ssl_client_socket_context_;
  net::ClientSocketFactory* client_socket_factory_;
  scoped_refptr<net::SSLConfigService> ssl_config_service_;
  mojo::StrongBindingSet<mojom::UDPSocket> udp_socket_bindings_;
  mojo::StrongBindingSet<mojom::TCPServerSocket> tcp_server_socket_bindings_;
  mojo::StrongBindingSet<mojom::TCPConnectedSocket>
      tcp_connected_socket_bindings_;
  mojo::StrongBindingSet<mojom::TLSClientSocket> tls_socket_bindings_;

  DISALLOW_COPY_AND_ASSIGN(SocketFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_SOCKET_FACTORY_H_
