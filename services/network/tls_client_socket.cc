// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/tls_client_socket.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "net/base/net_errors.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"

namespace network {

TLSClientSocket::TLSClientSocket(
    mojom::TLSClientSocketRequest request,
    mojom::SocketObserverPtr observer,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : observer_(std::move(observer)), traffic_annotation_(traffic_annotation) {}

TLSClientSocket::~TLSClientSocket() {}

void TLSClientSocket::Connect(
    const net::HostPortPair& host_port_pair,
    const net::SSLConfig& ssl_config,
    std::unique_ptr<net::ClientSocketHandle> tcp_socket,
    const net::SSLClientSocketContext& ssl_client_socket_context,
    net::ClientSocketFactory* socket_factory,
    mojom::TCPConnectedSocket::UpgradeToTLSCallback callback) {
  connect_callback_ = std::move(callback);
  socket_ = socket_factory->CreateSSLClientSocket(std::move(tcp_socket),
                                                  host_port_pair, ssl_config,
                                                  ssl_client_socket_context);
  int result = socket_->Connect(base::BindRepeating(
      &TLSClientSocket::OnTLSConnectCompleted, base::Unretained(this)));
  if (result != net::ERR_IO_PENDING)
    OnTLSConnectCompleted(result);
}

void TLSClientSocket::OnTLSConnectCompleted(int result) {
  DCHECK(!connect_callback_.is_null());

  if (result != net::OK) {
    socket_ = nullptr;
    std::move(connect_callback_)
        .Run(result, mojo::ScopedDataPipeConsumerHandle(),
             mojo::ScopedDataPipeProducerHandle());
    return;
  }
  mojo::DataPipe send_pipe;
  mojo::DataPipe receive_pipe;
  socket_data_pump_ = std::make_unique<SocketDataPump>(
      socket_.get(), this /*delegate*/, std::move(receive_pipe.producer_handle),
      std::move(send_pipe.consumer_handle), traffic_annotation_);
  std::move(connect_callback_)
      .Run(net::OK, std::move(receive_pipe.consumer_handle),
           std::move(send_pipe.producer_handle));
}

void TLSClientSocket::OnNetworkReadError(int net_error) {
  if (observer_)
    observer_->OnReadError(net_error);
}

void TLSClientSocket::OnNetworkWriteError(int net_error) {
  if (observer_)
    observer_->OnWriteError(net_error);
}

void TLSClientSocket::OnShutdown() {
  // Do nothing.
}

}  // namespace network
