// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef JINGLE_GLUE_XMPP_CLIENT_SOCKET_FACTORY_H_
#define JINGLE_GLUE_XMPP_CLIENT_SOCKET_FACTORY_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "jingle/glue/resolving_client_socket_factory.h"
#include "net/ssl/ssl_config_service.h"
#include "services/network/proxy_resolving_client_socket_factory.h"

namespace network {
class ProxyResolvingClientSocketFactory;
}  // namespace network

namespace net {
class ClientSocketFactory;
class ClientSocketHandle;
class HostPortPair;
class SSLClientSocket;
class StreamSocket;
class URLRequestContextGetter;
}  // namespace net

namespace jingle_glue {

class XmppClientSocketFactory : public ResolvingClientSocketFactory {
 public:
  // Does not take ownership of |client_socket_factory|.
  XmppClientSocketFactory(
      net::ClientSocketFactory* client_socket_factory,
      const net::SSLConfig& ssl_config,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      bool use_fake_ssl_client_socket);

  ~XmppClientSocketFactory() override;

  // ResolvingClientSocketFactory implementation.
  std::unique_ptr<net::StreamSocket> CreateTransportClientSocket(
      const net::HostPortPair& host_and_port) override;

  std::unique_ptr<net::SSLClientSocket> CreateSSLClientSocket(
      std::unique_ptr<net::ClientSocketHandle> transport_socket,
      const net::HostPortPair& host_and_port) override;

 private:
  net::ClientSocketFactory* const client_socket_factory_;
  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
  // |proxy_resolving_socket_factory_| retains a reference to the raw
  // net::URLRequestContext pointer, and thus must not outlive
  // |request_context_getter_|.
  network::ProxyResolvingClientSocketFactory proxy_resolving_socket_factory_;
  const net::SSLConfig ssl_config_;
  const bool use_fake_ssl_client_socket_;

  DISALLOW_COPY_AND_ASSIGN(XmppClientSocketFactory);
};

}  // namespace jingle_glue

#endif  // JINGLE_GLUE_XMPP_CLIENT_SOCKET_FACTORY_H_
