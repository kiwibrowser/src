// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PROXY_RESOLVING_CLIENT_SOCKET_FACTORY_H_
#define SERVICES_NETWORK_PROXY_RESOLVING_CLIENT_SOCKET_FACTORY_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/ssl/ssl_config.h"
#include "url/gurl.h"

namespace net {
class ClientSocketFactory;
class HttpNetworkSession;
class URLRequestContext;
}  // namespace net

namespace network {

class ProxyResolvingClientSocket;

class COMPONENT_EXPORT(NETWORK_SERVICE) ProxyResolvingClientSocketFactory {
 public:
  // Constructs a new ProxyResolvingClientSocket. |socket_factory| is the
  // ClientSocketFactory that will be used by the underlying HttpNetworkSession.
  // If |socket_factory| is nullptr, the default socket factory
  // (net::ClientSocketFactory::GetDefaultFactory()) will be used.
  // |request_context| must outlive |this| and all sockets |this| creates.
  ProxyResolvingClientSocketFactory(net::ClientSocketFactory* socket_factory,
                                    net::URLRequestContext* request_context);
  ~ProxyResolvingClientSocketFactory();

  // Creates a socket. |url|'s host and port specify where a connection will be
  // established to. The full URL will be only used for proxy resolution. Caller
  // doesn't need to explicitly sanitize the url, any sensitive data (like
  // embedded usernames and passwords), and local data (i.e. reference fragment)
  // will be sanitized by net::ProxyService::ResolveProxyHelper() before the url
  // is disclosed to the proxy. If |use_tls|, tls connect will be used in
  // addition to tcp connect.
  std::unique_ptr<ProxyResolvingClientSocket>
  CreateSocket(const net::SSLConfig& ssl_config, const GURL& url, bool use_tls);

 private:
  std::unique_ptr<net::HttpNetworkSession> network_session_;
  net::URLRequestContext* request_context_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolvingClientSocketFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_PROXY_RESOLVING_CLIENT_SOCKET_FACTORY_H_
