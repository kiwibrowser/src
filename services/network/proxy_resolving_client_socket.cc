// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/proxy_resolving_client_socket.h"

#include <stdint.h>
#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"
#include "net/base/ip_address.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_controller.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/http/proxy_client_socket.h"
#include "net/http/proxy_fallback.h"
#include "net/log/net_log_source_type.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/client_socket_pool_manager.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace network {

ProxyResolvingClientSocket::ProxyResolvingClientSocket(
    net::HttpNetworkSession* network_session,
    const net::SSLConfig& ssl_config,
    const GURL& url,
    bool use_tls)
    : network_session_(network_session),
      socket_handle_(std::make_unique<net::ClientSocketHandle>()),
      ssl_config_(ssl_config),
      proxy_resolve_request_(nullptr),
      url_(url),
      use_tls_(use_tls),
      net_log_(net::NetLogWithSource::Make(network_session_->net_log(),
                                           net::NetLogSourceType::SOCKET)),
      next_state_(STATE_NONE),
      weak_factory_(this) {
  // TODO(xunjieli): Handle invalid URLs more gracefully (at mojo API layer
  // or when the request is created).
  DCHECK(url_.is_valid());
}

ProxyResolvingClientSocket::~ProxyResolvingClientSocket() {
  Disconnect();
}

int ProxyResolvingClientSocket::Read(net::IOBuffer* buf,
                                     int buf_len,
                                     net::CompletionOnceCallback callback) {
  if (socket_handle_->socket())
    return socket_handle_->socket()->Read(buf, buf_len, std::move(callback));
  return net::ERR_SOCKET_NOT_CONNECTED;
}

int ProxyResolvingClientSocket::Write(
    net::IOBuffer* buf,
    int buf_len,
    net::CompletionOnceCallback callback,
    const net::NetworkTrafficAnnotationTag& traffic_annotation) {
  if (socket_handle_->socket()) {
    return socket_handle_->socket()->Write(buf, buf_len, std::move(callback),
                                           traffic_annotation);
  }
  return net::ERR_SOCKET_NOT_CONNECTED;
}

int ProxyResolvingClientSocket::SetReceiveBufferSize(int32_t size) {
  if (socket_handle_->socket())
    return socket_handle_->socket()->SetReceiveBufferSize(size);
  return net::ERR_SOCKET_NOT_CONNECTED;
}

int ProxyResolvingClientSocket::SetSendBufferSize(int32_t size) {
  if (socket_handle_->socket())
    return socket_handle_->socket()->SetSendBufferSize(size);
  return net::ERR_SOCKET_NOT_CONNECTED;
}

int ProxyResolvingClientSocket::Connect(net::CompletionOnceCallback callback) {
  DCHECK(user_connect_callback_.is_null());
  DCHECK(!socket_handle_->socket());

  next_state_ = STATE_PROXY_RESOLVE;
  int result = DoLoop(net::OK);
  if (result == net::ERR_IO_PENDING) {
    user_connect_callback_ = std::move(callback);
  }
  return result;
}

void ProxyResolvingClientSocket::Disconnect() {
  CloseSocket(true /*close_connection*/);
  if (proxy_resolve_request_) {
    network_session_->proxy_resolution_service()->CancelRequest(
        proxy_resolve_request_);
    proxy_resolve_request_ = nullptr;
  }
  user_connect_callback_.Reset();
}

bool ProxyResolvingClientSocket::IsConnected() const {
  if (!socket_handle_->socket())
    return false;
  return socket_handle_->socket()->IsConnected();
}

bool ProxyResolvingClientSocket::IsConnectedAndIdle() const {
  if (!socket_handle_->socket())
    return false;
  return socket_handle_->socket()->IsConnectedAndIdle();
}

int ProxyResolvingClientSocket::GetPeerAddress(net::IPEndPoint* address) const {
  if (!socket_handle_->socket()) {
    return net::ERR_SOCKET_NOT_CONNECTED;
  }

  if (proxy_info_.is_direct())
    return socket_handle_->socket()->GetPeerAddress(address);

  net::IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(url_.HostNoBrackets())) {
    // Do not expose the proxy IP address to the caller.
    return net::ERR_NAME_NOT_RESOLVED;
  }

  *address = net::IPEndPoint(ip_address, url_.EffectiveIntPort());
  return net::OK;
}

int ProxyResolvingClientSocket::GetLocalAddress(
    net::IPEndPoint* address) const {
  if (socket_handle_->socket())
    return socket_handle_->socket()->GetLocalAddress(address);
  return net::ERR_SOCKET_NOT_CONNECTED;
}

const net::NetLogWithSource& ProxyResolvingClientSocket::NetLog() const {
  if (socket_handle_->socket())
    return socket_handle_->socket()->NetLog();
  return net_log_;
}

bool ProxyResolvingClientSocket::WasEverUsed() const {
  if (socket_handle_->socket())
    return socket_handle_->socket()->WasEverUsed();
  return false;
}

bool ProxyResolvingClientSocket::WasAlpnNegotiated() const {
  if (socket_handle_->socket())
    return socket_handle_->socket()->WasAlpnNegotiated();
  return false;
}

net::NextProto ProxyResolvingClientSocket::GetNegotiatedProtocol() const {
  if (socket_handle_->socket())
    return socket_handle_->socket()->GetNegotiatedProtocol();
  return net::kProtoUnknown;
}

bool ProxyResolvingClientSocket::GetSSLInfo(net::SSLInfo* ssl_info) {
  if (socket_handle_->socket())
    return socket_handle_->socket()->GetSSLInfo(ssl_info);
  return false;
}

void ProxyResolvingClientSocket::GetConnectionAttempts(
    net::ConnectionAttempts* out) const {
  out->clear();
}

int64_t ProxyResolvingClientSocket::GetTotalReceivedBytes() const {
  NOTIMPLEMENTED();
  return 0;
}

void ProxyResolvingClientSocket::ApplySocketTag(const net::SocketTag& tag) {
  NOTIMPLEMENTED();
}

void ProxyResolvingClientSocket::OnIOComplete(int result) {
  DCHECK_NE(net::ERR_IO_PENDING, result);
  int net_error = DoLoop(result);
  if (net_error != net::ERR_IO_PENDING)
    std::move(user_connect_callback_).Run(net_error);
}

int ProxyResolvingClientSocket::DoLoop(int result) {
  DCHECK_NE(next_state_, STATE_NONE);
  int rv = result;
  do {
    State state = next_state_;
    next_state_ = STATE_NONE;
    switch (state) {
      case STATE_PROXY_RESOLVE:
        DCHECK_EQ(net::OK, rv);
        rv = DoProxyResolve();
        break;
      case STATE_PROXY_RESOLVE_COMPLETE:
        rv = DoProxyResolveComplete(rv);
        break;
      case STATE_INIT_CONNECTION:
        DCHECK_EQ(net::OK, rv);
        rv = DoInitConnection();
        break;
      case STATE_INIT_CONNECTION_COMPLETE:
        rv = DoInitConnectionComplete(rv);
        break;
      case STATE_RESTART_TUNNEL_AUTH:
        rv = DoRestartTunnelAuth(rv);
        break;
      case STATE_RESTART_TUNNEL_AUTH_COMPLETE:
        rv = DoRestartTunnelAuthComplete(rv);
        break;
      default:
        NOTREACHED() << "bad state";
        rv = net::ERR_FAILED;
        break;
    }
  } while (rv != net::ERR_IO_PENDING && next_state_ != STATE_NONE);
  return rv;
}

int ProxyResolvingClientSocket::DoProxyResolve() {
  next_state_ = STATE_PROXY_RESOLVE_COMPLETE;
  // TODO(xunjieli): Having a null ProxyDelegate is bad. Figure out how to
  // interact with the new interface for proxy delegate.
  // https://crbug.com/793071.
  // base::Unretained(this) is safe because resolution request is canceled when
  // |proxy_resolve_request_| is destroyed.
  return network_session_->proxy_resolution_service()->ResolveProxy(
      url_, "POST", &proxy_info_,
      base::BindRepeating(&ProxyResolvingClientSocket::OnIOComplete,
                          base::Unretained(this)),
      &proxy_resolve_request_, nullptr /*proxy_delegate*/, net_log_);
}

int ProxyResolvingClientSocket::DoProxyResolveComplete(int result) {
  proxy_resolve_request_ = nullptr;
  if (result == net::OK) {
    next_state_ = STATE_INIT_CONNECTION;
    // Removes unsupported proxies from the list. Currently, this removes
    // just the SCHEME_QUIC proxy, which doesn't yet support tunneling.
    // TODO(xunjieli): Allow QUIC proxy once it supports tunneling.
    proxy_info_.RemoveProxiesWithoutScheme(
        net::ProxyServer::SCHEME_DIRECT | net::ProxyServer::SCHEME_HTTP |
        net::ProxyServer::SCHEME_HTTPS | net::ProxyServer::SCHEME_SOCKS4 |
        net::ProxyServer::SCHEME_SOCKS5);

    if (proxy_info_.is_empty()) {
      // No proxies/direct to choose from. This happens when we don't support
      // any of the proxies in the returned list.
      result = net::ERR_NO_SUPPORTED_PROXIES;
    }
  }
  return result;
}

int ProxyResolvingClientSocket::DoInitConnection() {
  DCHECK(!socket_handle_->socket());

  next_state_ = STATE_INIT_CONNECTION_COMPLETE;

  // Now that the proxy is resolved, issue a socket connect.
  net::HostPortPair host_port_pair = net::HostPortPair::FromURL(url_);
  // Ignore socket limit set by socket pool for this type of socket.
  int request_load_flags = net::LOAD_IGNORE_LIMITS;
  net::RequestPriority request_priority = net::MAXIMUM_PRIORITY;

  // base::Unretained(this) is safe because request is canceled when
  // |socket_handle_| is destroyed.
  if (use_tls_) {
    return net::InitSocketHandleForTlsConnect(
        host_port_pair, network_session_, request_load_flags, request_priority,
        proxy_info_, ssl_config_, ssl_config_, net::PRIVACY_MODE_DISABLED,
        net_log_, socket_handle_.get(),
        base::BindRepeating(&ProxyResolvingClientSocket::OnIOComplete,
                            base::Unretained(this)));
  }
  return net::InitSocketHandleForRawConnect(
      host_port_pair, network_session_, request_load_flags, request_priority,
      proxy_info_, ssl_config_, ssl_config_, net::PRIVACY_MODE_DISABLED,
      net_log_, socket_handle_.get(),
      base::BindRepeating(&ProxyResolvingClientSocket::OnIOComplete,
                          base::Unretained(this)));
}

int ProxyResolvingClientSocket::DoInitConnectionComplete(int result) {
  if (result == net::ERR_PROXY_AUTH_REQUESTED) {
    if (use_tls_) {
      // Put the in-progress HttpProxyClientSocket into |socket_handle_| to
      // do tunnel auth. After auth completes, it's important to reset
      // |socket_handle_|, so it doesn't have a HttpProxyClientSocket when the
      // code expects an SSLClientSocket. The tunnel restart code is careful to
      // put it back to the socket pool before returning control to the rest of
      // this class.
      socket_handle_ = socket_handle_->release_pending_http_proxy_connection();
    }
    next_state_ = STATE_RESTART_TUNNEL_AUTH;
    return result;
  }

  if (result != net::OK) {
    // ReconsiderProxyAfterError either returns an error (in which case it is
    // not reconsidering a proxy) or returns ERR_IO_PENDING if it is considering
    // another proxy.
    return ReconsiderProxyAfterError(result);
  }

  network_session_->proxy_resolution_service()->ReportSuccess(proxy_info_,
                                                              nullptr);
  return net::OK;
}

int ProxyResolvingClientSocket::DoRestartTunnelAuth(int result) {
  DCHECK_EQ(net::ERR_PROXY_AUTH_REQUESTED, result);

  net::ProxyClientSocket* proxy_socket =
      static_cast<net::ProxyClientSocket*>(socket_handle_->socket());

  if (proxy_socket->GetAuthController() &&
      proxy_socket->GetAuthController()->HaveAuth()) {
    next_state_ = STATE_RESTART_TUNNEL_AUTH_COMPLETE;
    // base::Unretained(this) is safe because |proxy_socket| is owned by this.
    return proxy_socket->RestartWithAuth(base::BindRepeating(
        &ProxyResolvingClientSocket::OnIOComplete, base::Unretained(this)));
  }
  // This socket is unusable if the underlying authentication handler doesn't
  // already have credentials.  It is possible to overcome this hurdle and
  // finish the handshake if this class exposes an interface for an embedder to
  // supply credentials.
  CloseSocket(true /*close_connection*/);
  return result;
}

int ProxyResolvingClientSocket::DoRestartTunnelAuthComplete(int result) {
  if (result == net::ERR_PROXY_AUTH_REQUESTED) {
    // Handle multi-round auth challenge.
    next_state_ = STATE_RESTART_TUNNEL_AUTH;
    return result;
  }
  if (result == net::OK) {
    CloseSocket(false /*close_connection*/);
    // Now that the HttpProxyClientSocket is connected, release it as an idle
    // socket into the pool and start the connection process from the beginning.
    next_state_ = STATE_INIT_CONNECTION;
    return net::OK;
  }
  CloseSocket(true /*close_connection*/);
  return ReconsiderProxyAfterError(result);
}

void ProxyResolvingClientSocket::CloseSocket(bool close_connection) {
  if (close_connection && socket_handle_->socket())
    socket_handle_->socket()->Disconnect();
  socket_handle_->Reset();
}

int ProxyResolvingClientSocket::ReconsiderProxyAfterError(int error) {
  DCHECK(!socket_handle_->socket());
  DCHECK(!proxy_resolve_request_);
  DCHECK_NE(error, net::OK);
  DCHECK_NE(error, net::ERR_IO_PENDING);

  // Check if the error was a proxy failure.
  if (!net::CanFalloverToNextProxy(proxy_info_.proxy_server(), error, &error))
    return error;

  if (proxy_info_.is_https() && ssl_config_.send_client_cert) {
    network_session_->ssl_client_auth_cache()->Remove(
        proxy_info_.proxy_server().host_port_pair());
  }

  // There was nothing left to fall-back to, so fail the transaction
  // with the last connection error we got.
  if (!proxy_info_.Fallback(error, net_log_))
    return error;

  next_state_ = STATE_INIT_CONNECTION;
  return net::OK;
}

}  // namespace network
