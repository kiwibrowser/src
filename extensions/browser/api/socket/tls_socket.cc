// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/socket/tls_socket.h"

#include <utility>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/api_resource.h"
#include "net/base/address_list.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/rand_callback.h"
#include "net/base/url_util.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/tcp_client_socket.h"
#include "url/url_canon.h"

namespace {

// Returns the SSL protocol version (as a uint16_t) represented by a string.
// Returns 0 if the string is invalid.
uint16_t SSLProtocolVersionFromString(const std::string& version_str) {
  uint16_t version = 0;  // Invalid.
  if (version_str == "tls1") {
    version = net::SSL_PROTOCOL_VERSION_TLS1;
  } else if (version_str == "tls1.1") {
    version = net::SSL_PROTOCOL_VERSION_TLS1_1;
  } else if (version_str == "tls1.2") {
    version = net::SSL_PROTOCOL_VERSION_TLS1_2;
  }
  return version;
}

void TlsConnectDone(std::unique_ptr<net::SSLClientSocket> ssl_socket,
                    const std::string& extension_id,
                    const extensions::TLSSocket::SecureCallback& callback,
                    int result) {
  DVLOG(1) << "Got back result " << result << " " << net::ErrorToString(result);

  // No matter how the TLS connection attempt went, the underlying socket's
  // no longer bound to the original TCPSocket. It belongs to |ssl_socket|,
  // which is promoted here to a new API-accessible socket (via a TLSSocket
  // wrapper), or deleted.
  if (result != net::OK) {
    callback.Run(std::unique_ptr<extensions::TLSSocket>(), result);
    return;
  };

  // Wrap the StreamSocket in a TLSSocket, which matches the extension socket
  // API. Set the handle of the socket to the new value, so that it can be
  // used for read/write/close/etc.
  std::unique_ptr<extensions::TLSSocket> wrapper(
      new extensions::TLSSocket(std::move(ssl_socket), extension_id));

  // Caller will end up deleting the prior TCPSocket, once it calls
  // SetSocket(..,wrapper).
  callback.Run(std::move(wrapper), result);
}

}  // namespace

namespace extensions {

const char kTLSSocketTypeInvalidError[] =
    "Cannot listen on a socket that is already connected.";

TLSSocket::TLSSocket(std::unique_ptr<net::StreamSocket> tls_socket,
                     const std::string& owner_extension_id)
    : ResumableTCPSocket(owner_extension_id),
      tls_socket_(std::move(tls_socket)) {}

TLSSocket::~TLSSocket() {
  Disconnect(true /* socket_destroying */);
}

void TLSSocket::Connect(const net::AddressList& address,
                        const CompletionCallback& callback) {
  callback.Run(net::ERR_CONNECTION_FAILED);
}

void TLSSocket::Disconnect(bool socket_destroying) {
  if (tls_socket_) {
    tls_socket_->Disconnect();
    tls_socket_.reset();
  }
}

void TLSSocket::Read(int count, const ReadCompletionCallback& callback) {
  DCHECK(!callback.is_null());

  const bool socket_destroying = false;
  if (!read_callback_.is_null()) {
    callback.Run(net::ERR_IO_PENDING, nullptr, socket_destroying);
    return;
  }

  if (count <= 0) {
    callback.Run(net::ERR_INVALID_ARGUMENT, nullptr, socket_destroying);
    return;
  }

  if (!tls_socket_.get()) {
    callback.Run(net::ERR_SOCKET_NOT_CONNECTED, nullptr, socket_destroying);
    return;
  }

  read_callback_ = callback;
  scoped_refptr<net::IOBuffer> io_buffer(new net::IOBuffer(count));
  // |tls_socket_| is owned by this class and the callback won't be run once
  // |tls_socket_| is gone (as in an a call to Disconnect()). Therefore, it is
  // safe to use base::Unretained() here.
  int result = tls_socket_->Read(
      io_buffer.get(),
      count,
      base::Bind(
          &TLSSocket::OnReadComplete, base::Unretained(this), io_buffer));

  if (result != net::ERR_IO_PENDING) {
    OnReadComplete(io_buffer, result);
  }
}

void TLSSocket::OnReadComplete(const scoped_refptr<net::IOBuffer>& io_buffer,
                               int result) {
  DCHECK(!read_callback_.is_null());
  base::ResetAndReturn(&read_callback_)
      .Run(result, io_buffer, false /* socket_destroying */);
}

int TLSSocket::WriteImpl(net::IOBuffer* io_buffer,
                         int io_buffer_size,
                         const net::CompletionCallback& callback) {
  if (!IsConnected()) {
    return net::ERR_SOCKET_NOT_CONNECTED;
  }
  return tls_socket_->Write(io_buffer, io_buffer_size, callback,
                            Socket::GetNetworkTrafficAnnotationTag());
}

bool TLSSocket::SetKeepAlive(bool enable, int delay) {
  return false;
}

bool TLSSocket::SetNoDelay(bool no_delay) {
  return false;
}

int TLSSocket::Listen(const std::string& address,
                      uint16_t port,
                      int backlog,
                      std::string* error_msg) {
  *error_msg = kTLSSocketTypeInvalidError;
  return net::ERR_NOT_IMPLEMENTED;
}

void TLSSocket::Accept(const AcceptCompletionCallback& callback) {
  callback.Run(net::ERR_FAILED, NULL);
}

bool TLSSocket::IsConnected() {
  return tls_socket_.get() && tls_socket_->IsConnected();
}

bool TLSSocket::GetPeerAddress(net::IPEndPoint* address) {
  return IsConnected() && tls_socket_->GetPeerAddress(address);
}

bool TLSSocket::GetLocalAddress(net::IPEndPoint* address) {
  return IsConnected() && tls_socket_->GetLocalAddress(address);
}

Socket::SocketType TLSSocket::GetSocketType() const {
  return Socket::TYPE_TLS;
}

// static
void TLSSocket::UpgradeSocketToTLS(
    Socket* socket,
    scoped_refptr<net::SSLConfigService> ssl_config_service,
    net::CertVerifier* cert_verifier,
    net::TransportSecurityState* transport_security_state,
    net::CTVerifier* ct_verifier,
    net::CTPolicyEnforcer* ct_policy_enforcer,
    const std::string& extension_id,
    api::socket::SecureOptions* options,
    const TLSSocket::SecureCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  TCPSocket* tcp_socket = static_cast<TCPSocket*>(socket);
  std::unique_ptr<net::SSLClientSocket> null_sock;

  if (!tcp_socket || tcp_socket->GetSocketType() != Socket::TYPE_TCP ||
      !tcp_socket->ClientStream() || !tcp_socket->IsConnected() ||
      tcp_socket->HasPendingRead()) {
    DVLOG(1) << "Failing before trying. socket is " << tcp_socket;
    if (tcp_socket) {
      DVLOG(1) << "type: " << tcp_socket->GetSocketType()
               << ", ClientStream is " << tcp_socket->ClientStream()
               << ", IsConnected: " << tcp_socket->IsConnected()
               << ", HasPendingRead: " << tcp_socket->HasPendingRead();
    }
    TlsConnectDone(std::move(null_sock), extension_id, callback,
                   net::ERR_INVALID_ARGUMENT);
    return;
  }

  net::IPEndPoint dest_host_port_pair;
  if (!tcp_socket->GetPeerAddress(&dest_host_port_pair)) {
    DVLOG(1) << "Could not get peer address.";
    TlsConnectDone(std::move(null_sock), extension_id, callback,
                   net::ERR_INVALID_ARGUMENT);
    return;
  }

  // Convert any U-LABELs to A-LABELs.
  url::CanonHostInfo host_info;
  std::string canon_host =
      net::CanonicalizeHost(tcp_socket->hostname(), &host_info);

  // Canonicalization shouldn't fail: the socket is already connected with a
  // host, using this hostname.
  if (host_info.family == url::CanonHostInfo::BROKEN) {
    DVLOG(1) << "Could not canonicalize hostname";
    TlsConnectDone(std::move(null_sock), extension_id, callback,
                   net::ERR_INVALID_ARGUMENT);
    return;
  }

  net::HostPortPair host_and_port(canon_host, dest_host_port_pair.port());

  std::unique_ptr<net::ClientSocketHandle> socket_handle(
      new net::ClientSocketHandle());

  // Set the socket handle to the socket's client stream (that should be the
  // only one active here). Then have the old socket release ownership on
  // that client stream.
  socket_handle->SetSocket(
      std::unique_ptr<net::StreamSocket>(tcp_socket->ClientStream()));
  tcp_socket->Release();

  DCHECK(transport_security_state);
  net::SSLClientSocketContext context;
  context.cert_verifier = cert_verifier;
  context.transport_security_state = transport_security_state;
  context.cert_transparency_verifier = ct_verifier;
  context.ct_policy_enforcer = ct_policy_enforcer;

  // Fill in the SSL socket params.
  net::SSLConfig ssl_config;
  ssl_config_service->GetSSLConfig(&ssl_config);
  if (options && options->tls_version.get()) {
    uint16_t version_min = 0, version_max = 0;
    api::socket::TLSVersionConstraints* versions = options->tls_version.get();
    if (versions->min.get()) {
      version_min = SSLProtocolVersionFromString(*versions->min);
    }
    if (versions->max.get()) {
      version_max = SSLProtocolVersionFromString(*versions->max);
    }
    if (version_min) {
      ssl_config.version_min = version_min;
    }
    if (version_max) {
      ssl_config.version_max = version_max;
    }
  }

  net::ClientSocketFactory* socket_factory =
      net::ClientSocketFactory::GetDefaultFactory();

  // Create the socket.
  std::unique_ptr<net::SSLClientSocket> ssl_socket(
      socket_factory->CreateSSLClientSocket(
          std::move(socket_handle), host_and_port, ssl_config, context));

  DVLOG(1) << "Attempting to secure a connection to " << tcp_socket->hostname()
           << ":" << dest_host_port_pair.port();

  // We need the contents of |ssl_socket| in order to invoke its Connect()
  // method. It belongs to |ssl_socket|, and we own that until our internal
  // callback (|connect_cb|, below) is invoked.
  net::SSLClientSocket* saved_ssl_socket = ssl_socket.get();

  // Try establish a TLS connection. Pass ownership of |ssl_socket| to
  // TlsConnectDone, which will pass it on to |callback|. |connect_cb| below
  // is only for UpgradeSocketToTLS use, and not be confused with the
  // argument |callback|, which gets invoked by TlsConnectDone() after
  // Connect() below returns.
  base::Callback<void(int)> connect_cb(base::Bind(
      &TlsConnectDone, base::Passed(&ssl_socket), extension_id, callback));
  int status = saved_ssl_socket->Connect(connect_cb);
  saved_ssl_socket = NULL;

  // Connect completed synchronously, or failed.
  if (status != net::ERR_IO_PENDING) {
    // Note: this can't recurse -- if |socket| is already a connected
    // TLSSocket, it will return TYPE_TLS instead of TYPE_TCP, causing
    // UpgradeSocketToTLS() to fail with an error above. If
    // UpgradeSocketToTLS() is called on |socket| twice, the call to
    // Release() on |socket| above causes the additional call to
    // fail with an error above.
    if (status != net::OK) {
      DVLOG(1) << "Status is not OK or IO-pending: "
               << net::ErrorToString(status);
    }
    connect_cb.Run(status);
  }
}

}  // namespace extensions

