// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/socket/tcp_socket.h"

#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "extensions/browser/api/api_resource.h"
#include "net/base/address_list.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/log/net_log_source.h"
#include "net/socket/tcp_client_socket.h"

namespace extensions {

const char kTCPSocketTypeInvalidError[] =
    "Cannot call both connect and listen on the same socket.";
const char kSocketListenError[] = "Could not listen on the specified port.";

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

TCPSocket::TCPSocket(const std::string& owner_extension_id)
    : Socket(owner_extension_id), socket_mode_(UNKNOWN) {}

TCPSocket::TCPSocket(std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
                     const std::string& owner_extension_id,
                     bool is_connected)
    : Socket(owner_extension_id),
      socket_(std::move(tcp_client_socket)),
      socket_mode_(CLIENT) {
  this->is_connected_ = is_connected;
}

TCPSocket::TCPSocket(std::unique_ptr<net::TCPServerSocket> tcp_server_socket,
                     const std::string& owner_extension_id)
    : Socket(owner_extension_id),
      server_socket_(std::move(tcp_server_socket)),
      socket_mode_(SERVER) {}

// static
TCPSocket* TCPSocket::CreateSocketForTesting(
    std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
    const std::string& owner_extension_id,
    bool is_connected) {
  return new TCPSocket(std::move(tcp_client_socket), owner_extension_id,
                       is_connected);
}

// static
TCPSocket* TCPSocket::CreateServerSocketForTesting(
    std::unique_ptr<net::TCPServerSocket> tcp_server_socket,
    const std::string& owner_extension_id) {
  return new TCPSocket(std::move(tcp_server_socket), owner_extension_id);
}

TCPSocket::~TCPSocket() {
  Disconnect(true /* socket_destroying */);
}

void TCPSocket::Connect(const net::AddressList& address,
                        const CompletionCallback& callback) {
  DCHECK(!callback.is_null());

  if (socket_mode_ == SERVER || !connect_callback_.is_null()) {
    callback.Run(net::ERR_CONNECTION_FAILED);
    return;
  }

  if (is_connected_) {
    callback.Run(net::ERR_SOCKET_IS_CONNECTED);
    return;
  }

  DCHECK(!server_socket_.get());
  socket_mode_ = CLIENT;
  connect_callback_ = callback;

  int result = net::ERR_CONNECTION_FAILED;
  if (!is_connected_) {
    socket_.reset(
        new net::TCPClientSocket(address, NULL, NULL, net::NetLogSource()));
    result = socket_->Connect(
        base::Bind(&TCPSocket::OnConnectComplete, base::Unretained(this)));
  }

  if (result != net::ERR_IO_PENDING)
    OnConnectComplete(result);
}

void TCPSocket::Disconnect(bool socket_destroying) {
  is_connected_ = false;
  if (socket_.get())
    socket_->Disconnect();
  server_socket_.reset(NULL);
  connect_callback_.Reset();
  // TODO(devlin): Should we do this for all callbacks?
  if (!read_callback_.is_null()) {
    base::ResetAndReturn(&read_callback_)
        .Run(net::ERR_CONNECTION_CLOSED, nullptr, socket_destroying);
  }
  accept_callback_.Reset();
  accept_socket_.reset(NULL);
}

void TCPSocket::Bind(const std::string& address,
                     uint16_t port,
                     const net::CompletionCallback& callback) {
  callback.Run(net::ERR_FAILED);
}

void TCPSocket::Read(int count, const ReadCompletionCallback& callback) {
  DCHECK(!callback.is_null());

  const bool socket_destroying = false;
  if (socket_mode_ != CLIENT) {
    callback.Run(net::ERR_FAILED, nullptr, socket_destroying);
    return;
  }

  if (!read_callback_.is_null() || !connect_callback_.is_null()) {
    // It's illegal to read a net::TCPSocket while a pending Connect or Read is
    // already in progress.
    callback.Run(net::ERR_IO_PENDING, nullptr, socket_destroying);
    return;
  }

  if (count < 0) {
    callback.Run(net::ERR_INVALID_ARGUMENT, nullptr, socket_destroying);
    return;
  }

  if (!socket_.get() || !is_connected_) {
    callback.Run(net::ERR_SOCKET_NOT_CONNECTED, nullptr, socket_destroying);
    return;
  }

  read_callback_ = callback;
  scoped_refptr<net::IOBuffer> io_buffer = new net::IOBuffer(count);
  int result = socket_->Read(
      io_buffer.get(),
      count,
      base::Bind(
          &TCPSocket::OnReadComplete, base::Unretained(this), io_buffer));

  if (result != net::ERR_IO_PENDING)
    OnReadComplete(io_buffer, result);
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

bool TCPSocket::SetKeepAlive(bool enable, int delay) {
  if (!socket_.get())
    return false;
  return socket_->SetKeepAlive(enable, delay);
}

bool TCPSocket::SetNoDelay(bool no_delay) {
  if (!socket_.get())
    return false;
  return socket_->SetNoDelay(no_delay);
}

int TCPSocket::Listen(const std::string& address,
                      uint16_t port,
                      int backlog,
                      std::string* error_msg) {
  if (socket_mode_ == CLIENT) {
    *error_msg = kTCPSocketTypeInvalidError;
    return net::ERR_NOT_IMPLEMENTED;
  }
  DCHECK(!socket_.get());
  socket_mode_ = SERVER;

  if (!server_socket_.get()) {
    server_socket_.reset(new net::TCPServerSocket(NULL, net::NetLogSource()));
  }

  int result = server_socket_->ListenWithAddressAndPort(address, port, backlog);
  if (result) {
    server_socket_.reset();
    *error_msg = kSocketListenError;
  }
  return result;
}

void TCPSocket::Accept(const AcceptCompletionCallback& callback) {
  if (socket_mode_ != SERVER || !server_socket_.get()) {
    callback.Run(net::ERR_FAILED, NULL);
    return;
  }

  // Limits to only 1 blocked accept call.
  if (!accept_callback_.is_null()) {
    callback.Run(net::ERR_FAILED, NULL);
    return;
  }

  int result = server_socket_->Accept(
      &accept_socket_,
      base::Bind(&TCPSocket::OnAccept, base::Unretained(this)));
  if (result == net::ERR_IO_PENDING) {
    accept_callback_ = callback;
  } else if (result == net::OK) {
    accept_callback_ = callback;
    this->OnAccept(result);
  } else {
    callback.Run(result, NULL);
  }
}

bool TCPSocket::IsConnected() {
  RefreshConnectionStatus();
  return is_connected_;
}

bool TCPSocket::GetPeerAddress(net::IPEndPoint* address) {
  if (!socket_.get())
    return false;
  return !socket_->GetPeerAddress(address);
}

bool TCPSocket::GetLocalAddress(net::IPEndPoint* address) {
  if (socket_.get()) {
    return !socket_->GetLocalAddress(address);
  } else if (server_socket_.get()) {
    return !server_socket_->GetLocalAddress(address);
  } else {
    return false;
  }
}

Socket::SocketType TCPSocket::GetSocketType() const { return Socket::TYPE_TCP; }

int TCPSocket::WriteImpl(net::IOBuffer* io_buffer,
                         int io_buffer_size,
                         const net::CompletionCallback& callback) {
  if (socket_mode_ != CLIENT)
    return net::ERR_FAILED;
  else if (!socket_.get() || !IsConnected())
    return net::ERR_SOCKET_NOT_CONNECTED;
  else
    return socket_->Write(io_buffer, io_buffer_size, callback,
                          Socket::GetNetworkTrafficAnnotationTag());
}

void TCPSocket::RefreshConnectionStatus() {
  if (!is_connected_)
    return;
  if (server_socket_)
    return;
  if (!socket_->IsConnected()) {
    Disconnect(false /* socket_destroying */);
  }
}

void TCPSocket::OnConnectComplete(int result) {
  DCHECK(!connect_callback_.is_null());
  DCHECK(!is_connected_);
  is_connected_ = result == net::OK;

  // The completion callback may re-enter TCPSocket, e.g. to Read(); therefore
  // we reset |connect_callback_| before calling it.
  CompletionCallback connect_callback = connect_callback_;
  connect_callback_.Reset();
  connect_callback.Run(result);
}

void TCPSocket::OnReadComplete(scoped_refptr<net::IOBuffer> io_buffer,
                               int result) {
  DCHECK(!read_callback_.is_null());
  read_callback_.Run(result, io_buffer, false /* socket_destroying */);
  read_callback_.Reset();
}

void TCPSocket::OnAccept(int result) {
  DCHECK(!accept_callback_.is_null());
  if (result == net::OK && accept_socket_.get()) {
    accept_callback_.Run(result,
                         base::WrapUnique(static_cast<net::TCPClientSocket*>(
                             accept_socket_.release())));
  } else {
    accept_callback_.Run(result, NULL);
  }
  accept_callback_.Reset();
}

void TCPSocket::Release() {
  // Release() is only invoked when the underlying sockets are taken (via
  // ClientStream()) by TLSSocket. TLSSocket only supports CLIENT-mode
  // sockets.
  DCHECK(!server_socket_.release() && !accept_socket_.release() &&
         socket_mode_ == CLIENT)
      << "Called in server mode.";

  // Release() doesn't disconnect the underlying sockets, but it does
  // disconnect them from this TCPSocket.
  is_connected_ = false;

  connect_callback_.Reset();
  read_callback_.Reset();
  accept_callback_.Reset();

  DCHECK(socket_.get()) << "Called on null client socket.";
  ignore_result(socket_.release());
}

net::TCPClientSocket* TCPSocket::ClientStream() {
  if (socket_mode_ != CLIENT || GetSocketType() != TYPE_TCP)
    return NULL;
  return socket_.get();
}

bool TCPSocket::HasPendingRead() const {
  return !read_callback_.is_null();
}

ResumableTCPSocket::ResumableTCPSocket(const std::string& owner_extension_id)
    : TCPSocket(owner_extension_id),
      persistent_(false),
      buffer_size_(0),
      paused_(false) {}

ResumableTCPSocket::ResumableTCPSocket(
    std::unique_ptr<net::TCPClientSocket> tcp_client_socket,
    const std::string& owner_extension_id,
    bool is_connected)
    : TCPSocket(std::move(tcp_client_socket), owner_extension_id, is_connected),
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
    const std::string& owner_extension_id)
    : TCPSocket(owner_extension_id), persistent_(false), paused_(false) {}

bool ResumableTCPServerSocket::IsPersistent() const { return persistent(); }

}  // namespace extensions
