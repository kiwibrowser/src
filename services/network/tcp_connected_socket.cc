// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/tcp_connected_socket.h"

#include <utility>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/log/net_log.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "services/network/tls_client_socket.h"

namespace network {

TCPConnectedSocket::TCPConnectedSocket(
    mojom::SocketObserverPtr observer,
    net::NetLog* net_log,
    Delegate* delegate,
    net::ClientSocketFactory* client_socket_factory,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : observer_(std::move(observer)),
      net_log_(net_log),
      delegate_(delegate),
      client_socket_factory_(client_socket_factory),
      traffic_annotation_(traffic_annotation) {}

TCPConnectedSocket::TCPConnectedSocket(
    mojom::SocketObserverPtr observer,
    std::unique_ptr<net::StreamSocket> socket,
    mojo::ScopedDataPipeProducerHandle receive_pipe_handle,
    mojo::ScopedDataPipeConsumerHandle send_pipe_handle,
    const net::NetworkTrafficAnnotationTag& traffic_annotation)
    : observer_(std::move(observer)),
      net_log_(nullptr),
      delegate_(nullptr),
      client_socket_factory_(nullptr),
      socket_(std::move(socket)),
      traffic_annotation_(traffic_annotation) {
  socket_data_pump_ = std::make_unique<SocketDataPump>(
      socket_.get(), this /*delegate*/, std::move(receive_pipe_handle),
      std::move(send_pipe_handle), traffic_annotation);
}

TCPConnectedSocket::~TCPConnectedSocket() {}

void TCPConnectedSocket::Connect(
    const base::Optional<net::IPEndPoint>& local_addr,
    const net::AddressList& remote_addr_list,
    mojom::NetworkContext::CreateTCPConnectedSocketCallback callback) {
  DCHECK(!socket_);
  DCHECK(callback);

  auto socket = client_socket_factory_->CreateTransportClientSocket(
      remote_addr_list, nullptr /*socket_performance_watcher*/, net_log_,
      net::NetLogSource());
  connect_callback_ = std::move(callback);
  int result = net::OK;
  if (local_addr)
    result = socket->Bind(local_addr.value());
  if (result == net::OK) {
    result = socket->Connect(base::BindRepeating(
        &TCPConnectedSocket::OnConnectCompleted, base::Unretained(this)));
  }
  socket_ = std::move(socket);
  if (result == net::ERR_IO_PENDING)
    return;
  OnConnectCompleted(result);
}

void TCPConnectedSocket::GetLocalAddress(GetLocalAddressCallback callback) {
  DCHECK(socket_);

  net::IPEndPoint local_addr;
  int result = socket_->GetLocalAddress(&local_addr);
  if (result != net::OK) {
    std::move(callback).Run(result, base::nullopt);
    return;
  }
  std::move(callback).Run(result, local_addr);
}

void TCPConnectedSocket::UpgradeToTLS(
    const net::HostPortPair& host_port_pair,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojom::TLSClientSocketRequest request,
    mojom::SocketObserverPtr observer,
    UpgradeToTLSCallback callback) {
  // Wait for data pipes to be closed by the client before doing the upgrade.
  if (socket_data_pump_) {
    pending_upgrade_to_tls_callback_ = base::BindOnce(
        &TCPConnectedSocket::UpgradeToTLS, base::Unretained(this),
        host_port_pair, traffic_annotation, std::move(request),
        std::move(observer), std::move(callback));
    return;
  }
  if (!socket_ || !socket_->IsConnected()) {
    std::move(callback).Run(net::ERR_SOCKET_NOT_CONNECTED,
                            mojo::ScopedDataPipeConsumerHandle(),
                            mojo::ScopedDataPipeProducerHandle());
    return;
  }
  auto socket_handle = std::make_unique<net::ClientSocketHandle>();
  socket_handle->SetSocket(std::move(socket_));
  delegate_->CreateTLSClientSocket(
      host_port_pair, std::move(request), std::move(socket_handle),
      std::move(observer),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation),
      std::move(callback));
}

void TCPConnectedSocket::OnConnectCompleted(int result) {
  DCHECK(!connect_callback_.is_null());
  DCHECK(!socket_data_pump_);

  if (result != net::OK) {
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

void TCPConnectedSocket::OnNetworkReadError(int net_error) {
  if (observer_)
    observer_->OnReadError(net_error);
}

void TCPConnectedSocket::OnNetworkWriteError(int net_error) {
  if (observer_)
    observer_->OnWriteError(net_error);
}

void TCPConnectedSocket::OnShutdown() {
  socket_data_pump_ = nullptr;
  if (!pending_upgrade_to_tls_callback_.is_null())
    std::move(pending_upgrade_to_tls_callback_).Run();
}

}  // namespace network
