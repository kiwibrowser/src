// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/glue/xmpp_client_socket_factory.h"

#include <utility>

#include "base/logging.h"
#include "jingle/glue/fake_ssl_client_socket.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/proxy_resolving_client_socket.h"

namespace jingle_glue {

XmppClientSocketFactory::XmppClientSocketFactory(
    net::ClientSocketFactory* client_socket_factory,
    const net::SSLConfig& ssl_config,
    const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
    bool use_fake_ssl_client_socket)
    : client_socket_factory_(client_socket_factory),
      request_context_getter_(request_context_getter),
      proxy_resolving_socket_factory_(
          nullptr,
          request_context_getter->GetURLRequestContext()),
      ssl_config_(ssl_config),
      use_fake_ssl_client_socket_(use_fake_ssl_client_socket) {
  CHECK(client_socket_factory_);
}

XmppClientSocketFactory::~XmppClientSocketFactory() {}

std::unique_ptr<net::StreamSocket>
XmppClientSocketFactory::CreateTransportClientSocket(
    const net::HostPortPair& host_and_port) {
  // TODO(akalin): Use socket pools.
  auto transport_socket = proxy_resolving_socket_factory_.CreateSocket(
      ssl_config_, GURL("https://" + host_and_port.ToString()),
      false /*use_tls*/);
  return (use_fake_ssl_client_socket_
              ? std::unique_ptr<net::StreamSocket>(
                    new FakeSSLClientSocket(std::move(transport_socket)))
              : std::move(transport_socket));
}

std::unique_ptr<net::SSLClientSocket>
XmppClientSocketFactory::CreateSSLClientSocket(
    std::unique_ptr<net::ClientSocketHandle> transport_socket,
    const net::HostPortPair& host_and_port) {
  const net::URLRequestContext* url_context =
      request_context_getter_->GetURLRequestContext();
  net::SSLClientSocketContext context(
      url_context->cert_verifier(),
      nullptr, /* TODO(rkn): ChannelIDService is not thread safe. */
      url_context->transport_security_state(),
      url_context->cert_transparency_verifier(),
      url_context->ct_policy_enforcer(),
      std::string() /* TODO(rsleevi): Ensure a proper unique shard. */);
  return client_socket_factory_->CreateSSLClientSocket(
      std::move(transport_socket), host_and_port, ssl_config_, context);
}


}  // namespace jingle_glue
