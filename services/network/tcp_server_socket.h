// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TCP_SERVER_SOCKET_H_
#define SERVICES_NETWORK_TCP_SERVER_SOCKET_H_

#include <memory>
#include <vector>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "net/base/ip_endpoint.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"

namespace net {
class NetLog;
class ServerSocket;
class StreamSocket;
}  // namespace net

namespace network {
class TCPConnectedSocket;

class COMPONENT_EXPORT(NETWORK_SERVICE) TCPServerSocket
    : public mojom::TCPServerSocket {
 public:
  // A delegate interface that is notified when new connections are established.
  class Delegate {
   public:
    Delegate() {}
    ~Delegate() {}

    // Invoked when a new connection is accepted. The delegate should take
    // ownership of |socket| and set up binding for |request|.
    virtual void OnAccept(std::unique_ptr<TCPConnectedSocket> socket,
                          mojom::TCPConnectedSocketRequest request) = 0;
  };

  // Constructs a TCPServerSocket. |delegate| must outlive |this|. When a new
  // connection is accepted, |delegate| will be notified to take ownership of
  // the connection.
  TCPServerSocket(Delegate* delegate,
                  net::NetLog* net_log,
                  const net::NetworkTrafficAnnotationTag& traffic_annotation);
  ~TCPServerSocket() override;

  int Listen(const net::IPEndPoint& local_addr,
             int backlog,
             net::IPEndPoint* local_addr_out);

  // TCPServerSocket implementation.
  void Accept(mojom::SocketObserverPtr observer,
              AcceptCallback callback) override;
  void GetLocalAddress(GetLocalAddressCallback callback) override;

  // Replaces the underlying socket implementation with |socket| in tests.
  void SetSocketForTest(std::unique_ptr<net::ServerSocket> socket);

 private:
  struct PendingAccept {
    PendingAccept(AcceptCallback callback, mojom::SocketObserverPtr observer);
    ~PendingAccept();

    AcceptCallback callback;
    mojom::SocketObserverPtr observer;
  };
  // Invoked when socket_->Accept() completes.
  void OnAcceptCompleted(int result);
  // Process the next Accept() from |pending_accepts_queue_|.
  void ProcessNextAccept();

  Delegate* const delegate_;
  std::unique_ptr<net::ServerSocket> socket_;
  int backlog_;
  std::vector<std::unique_ptr<PendingAccept>> pending_accepts_queue_;
  std::unique_ptr<net::StreamSocket> accepted_socket_;
  net::NetworkTrafficAnnotationTag traffic_annotation_;

  base::WeakPtrFactory<TCPServerSocket> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(TCPServerSocket);
};

}  // namespace network

#endif  // SERVICES_NETWORK_TCP_SERVER_SOCKET_H_
