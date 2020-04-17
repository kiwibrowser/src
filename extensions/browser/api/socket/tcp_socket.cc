// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/socket/tcp_socket.h"

#include <utility>

#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/api/api_resource.h"
#include "extensions/browser/api/socket/mojo_data_pump.h"
#include "net/base/address_list.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "services/network/public/mojom/ssl_config.mojom.h"

namespace extensions {

namespace {

}  // namespace

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<ResumableTCPSocket>>>::DestructorAtExit g_factory =
    LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<ResumableTCPSocket> >*
ApiResourceManager<ResumableTCPSocket>::GetFactoryInstance() {
  return g_factory.Pointer();
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ApiResourceManager<ResumableTCPServerSocket>>>::DestructorAtExit
    g_server_factory = LAZY_INSTANCE_INITIALIZER;

// static
template <>
BrowserContextKeyedAPIFactory<ApiResourceManager<ResumableTCPServerSocket> >*
ApiResourceManager<ResumableTCPServerSocket>::GetFactoryInstance() {
  return g_server_factory.Pointer();
}

TCPSocket::TCPSocket(content::BrowserContext* browser_context,
                     const std::string& owner_extension_id)
    : Socket(owner_extension_id),
      browser_context_(browser_context),
      mojo_data_pump_(nullptr),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      weak_factory_(this) {}

TCPSocket::TCPSocket(network::mojom::TCPConnectedSocketPtr socket,
                     mojo::ScopedDataPipeConsumerHandle receive_stream,
                     mojo::ScopedDataPipeProducerHandle send_stream,
                     const base::Optional<net::IPEndPoint>& remote_addr,
                     const std::string& owner_extension_id)
    : Socket(owner_extension_id),
      browser_context_(nullptr),
      client_socket_(std::move(socket)),
      mojo_data_pump_(std::make_unique<MojoDataPump>(std::move(receive_stream),
                                                     std::move(send_stream))),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      peer_addr_(remote_addr),

      weak_factory_(this) {
  is_connected_ = true;
}

TCPSocket::~TCPSocket() {
  Disconnect(true /* socket_destroying */);
}

void TCPSocket::Connect(const net::AddressList& address,
                        net::CompletionOnceCallback callback) {
  DCHECK(callback);
}

void TCPSocket::Disconnect(bool socket_destroying) {
  is_connected_ = false;
  local_addr_ = base::nullopt;
  peer_addr_ = base::nullopt;
  mojo_data_pump_ = nullptr;
  client_socket_.reset();
  server_socket_.reset();
  listen_callback_.Reset();
  connect_callback_.Reset();
  accept_callback_.Reset();
  // TODO(devlin): Should we do this for all callbacks?
  if (read_callback_) {
    std::move(read_callback_)
        .Run(net::ERR_CONNECTION_CLOSED, nullptr, socket_destroying);
  }
}

void TCPSocket::Bind(const std::string& address,
                     uint16_t port,
                     const net::CompletionCallback& callback) {
  callback.Run(net::ERR_FAILED);
}

void TCPSocket::Read(int count, ReadCompletionCallback callback) {
  DCHECK(callback);
}

void TCPSocket::RecvFrom(int count,
                         const RecvFromCompletionCallback& callback) {
  callback.Run(net::ERR_FAILED, nullptr, false /* socket_destroying */, nullptr,
               0);
}

void TCPSocket::SendTo(scoped_refptr<net::IOBuffer> io_buffer,
                       int byte_count,
                       const net::IPEndPoint& address,
                       const CompletionCallback& callback) {
  callback.Run(net::ERR_FAILED);
}

void TCPSocket::SetKeepAlive(bool enable,
                             int delay,
                             SetKeepAliveCallback callback) {
}

void TCPSocket::SetNoDelay(bool no_delay, SetNoDelayCallback callback) {
}

void TCPSocket::Listen(const std::string& address,
                       uint16_t port,
                       int backlog,
                       ListenCallback callback) {
}

void TCPSocket::Accept(AcceptCompletionCallback callback) {
}

bool TCPSocket::IsConnected() {
  return is_connected_;
}

bool TCPSocket::GetPeerAddress(net::IPEndPoint* address) {
  if (!peer_addr_)
    return false;
  *address = peer_addr_.value();
  return true;
}

bool TCPSocket::GetLocalAddress(net::IPEndPoint* address) {
  if (!local_addr_)
    return false;
  *address = local_addr_.value();
  return true;
}

Socket::SocketType TCPSocket::GetSocketType() const { return Socket::TYPE_TCP; }

int TCPSocket::WriteImpl(net::IOBuffer* io_buffer,
                         int io_buffer_size,
                         const net::CompletionCallback& callback) {
  if (!mojo_data_pump_)
    return net::ERR_SOCKET_NOT_CONNECTED;

  mojo_data_pump_->Write(io_buffer, io_buffer_size,
                         base::BindOnce(&TCPSocket::OnWriteComplete,
                                        base::Unretained(this), callback));
  return net::ERR_IO_PENDING;
}

// static
void TCPSocket::ConnectOnUIThread(
    content::StoragePartition* storage_partition,
    content::BrowserContext* browser_context,
    const net::AddressList& remote_addr_list,
    network::mojom::TCPConnectedSocketRequest request,
    network::mojom::NetworkContext::CreateTCPConnectedSocketCallback
        completion_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!storage_partition) {
    storage_partition =
        content::BrowserContext::GetDefaultStoragePartition(browser_context);
  }
  storage_partition->GetNetworkContext()->CreateTCPConnectedSocket(
      base::nullopt, remote_addr_list,
      net::MutableNetworkTrafficAnnotationTag(
          Socket::GetNetworkTrafficAnnotationTag()),
      std::move(request), nullptr /* observer */,
      std::move(completion_callback));
}

// static
void TCPSocket::OnConnectCompleteOnUIThread(
    scoped_refptr<base::SequencedTaskRunner> original_task_runner,
    network::mojom::NetworkContext::CreateTCPConnectedSocketCallback callback,
    int result,
    const base::Optional<net::IPEndPoint>& local_addr,
    const base::Optional<net::IPEndPoint>& peer_addr,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream) {
}

void TCPSocket::OnConnectComplete(
    int result,
    const base::Optional<net::IPEndPoint>& local_addr,
    const base::Optional<net::IPEndPoint>& peer_addr,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream) {
}

// static
void TCPSocket::ListenOnUIThread(
    content::StoragePartition* storage_partition,
    content::BrowserContext* browser_context,
    const net::IPEndPoint& local_addr,
    int backlog,
    network::mojom::TCPServerSocketRequest request,
    network::mojom::NetworkContext::CreateTCPServerSocketCallback callback) {
}

// static
void TCPSocket::OnListenCompleteOnUIThread(
    const scoped_refptr<base::SequencedTaskRunner>& original_task_runner,
    network::mojom::NetworkContext::CreateTCPServerSocketCallback callback,
    int result,
    const base::Optional<net::IPEndPoint>& local_addr) {
}

void TCPSocket::OnListenComplete(
    int result,
    const base::Optional<net::IPEndPoint>& local_addr) {
}

content::StoragePartition* TCPSocket::GetStoragePartitionHelper() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  return storage_partition_
             ? storage_partition_
             : content::BrowserContext::GetDefaultStoragePartition(
                   browser_context_);
}

void TCPSocket::OnAccept(int result,
                         const base::Optional<net::IPEndPoint>& remote_addr,
                         network::mojom::TCPConnectedSocketPtr connected_socket,
                         mojo::ScopedDataPipeConsumerHandle receive_stream,
                         mojo::ScopedDataPipeProducerHandle send_stream) {
  DCHECK(accept_callback_);
  std::move(accept_callback_)
      .Run(result, std::move(connected_socket), remote_addr,
           std::move(receive_stream), std::move(send_stream));
}

void TCPSocket::OnWriteComplete(const net::CompletionCallback& callback,
                                int result) {
  if (result < 0) {
    // Write side has terminated. This can be an error or a graceful close.
    // TCPSocketEventDispatcher doesn't distinguish between the two.
    Disconnect(false /* socket_destroying */);
  }
  callback.Run(result);
}

void TCPSocket::OnReadComplete(int result,
                               scoped_refptr<net::IOBuffer> io_buffer) {
  DCHECK(read_callback_);

  // Use a local variable for |read_callback_|, because otherwise Disconnect()
  // will try to invoke |read_callback_| with a hardcoded result code.
  ReadCompletionCallback callback = std::move(read_callback_);
  if (result < 0) {
    // Read side has terminated. This can be an error or a graceful close.
    // TCPSocketEventDispatcher doesn't distinguish between the two.
    Disconnect(false /* socket_destroying */);
  }
  std::move(callback).Run(result, io_buffer, false /* socket_destroying */);
}

void TCPSocket::OnUpgradeToTLSComplete(
    UpgradeToTLSCallback callback,
    network::mojom::TLSClientSocketPtr tls_socket,
    const net::IPEndPoint& local_addr,
    const net::IPEndPoint& peer_addr,
    int result,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream) {
  std::move(callback).Run(result, std::move(tls_socket), local_addr, peer_addr,
                          std::move(receive_stream), std::move(send_stream));
}

void TCPSocket::UpgradeToTLS(api::socket::SecureOptions* options,
                             UpgradeToTLSCallback callback) {
}

ResumableTCPSocket::ResumableTCPSocket(content::BrowserContext* browser_context,
                                       const std::string& owner_extension_id)
    : TCPSocket(browser_context, owner_extension_id),
      persistent_(false),
      buffer_size_(0),
      paused_(false) {}

ResumableTCPSocket::ResumableTCPSocket(
    network::mojom::TCPConnectedSocketPtr socket,
    mojo::ScopedDataPipeConsumerHandle receive_stream,
    mojo::ScopedDataPipeProducerHandle send_stream,
    const base::Optional<net::IPEndPoint>& remote_addr,
    const std::string& owner_extension_id)
    : TCPSocket(std::move(socket),
                std::move(receive_stream),
                std::move(send_stream),
                remote_addr,
                owner_extension_id),
      persistent_(false),
      buffer_size_(0),
      paused_(false) {}

ResumableTCPSocket::~ResumableTCPSocket() {
  // Despite ~TCPSocket doing basically the same, we need to disconnect
  // before ResumableTCPSocket is destroyed, because we have some extra
  // state that relies on the socket being ResumableTCPSocket, like
  // read_callback_.
  Disconnect(true /* socket_destroying */);
}

bool ResumableTCPSocket::IsPersistent() const { return persistent(); }

ResumableTCPServerSocket::ResumableTCPServerSocket(
    content::BrowserContext* browser_context,
    const std::string& owner_extension_id)
    : TCPSocket(browser_context, owner_extension_id),
      persistent_(false),
      paused_(false) {}

bool ResumableTCPServerSocket::IsPersistent() const { return persistent(); }

}  // namespace extensions
