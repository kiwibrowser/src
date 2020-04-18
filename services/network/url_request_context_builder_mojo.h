// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_URL_REQUEST_CONTEXT_BUILDER_MOJO_H_
#define SERVICES_NETWORK_URL_REQUEST_CONTEXT_BUILDER_MOJO_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher_factory.h"
#include "net/url_request/url_request_context_builder.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/url_request_context_owner.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

namespace net {
class HostResolver;
class NetLog;
class NetworkDelegate;
class ProxyResolutionService;
class URLRequestContext;
}  // namespace net

namespace network {

// Specialization of URLRequestContextBuilder that can create a
// ProxyResolutionService that uses a Mojo ProxyResolver. The consumer is
// responsible for providing the proxy_resolver::mojom::ProxyResolverFactory.
// If a ProxyResolutionService is set directly via the URLRequestContextBuilder
// API, it will be used instead.
class COMPONENT_EXPORT(NETWORK_SERVICE) URLRequestContextBuilderMojo
    : public net::URLRequestContextBuilder {
 public:
  URLRequestContextBuilderMojo();
  ~URLRequestContextBuilderMojo() override;

  // Overrides default DhcpPacFileFetcherFactory. Ignored if no
  // proxy_resolver::mojom::ProxyResolverFactory is provided.
  void SetDhcpFetcherFactory(
      std::unique_ptr<net::DhcpPacFileFetcherFactory> dhcp_fetcher_factory);

  // Sets Mojo factory used to create ProxyResolvers. If not set, falls back to
  // URLRequestContext's default behavior.
  void SetMojoProxyResolverFactory(
      proxy_resolver::mojom::ProxyResolverFactoryPtr
          mojo_proxy_resolver_factory);

  // Can be used to create a URLRequestContext from this consumer-configured
  // URLRequestContextBuilder, which |params| will then be applied to. The
  // results URLRequestContext will be returned along with other state that it
  // depends on. The URLRequestContext can be further modified before first use.
  //
  // This method is intended to ease the transition to an out-of-process
  // NetworkService, and will be removed once that ships.
  URLRequestContextOwner Create(
      mojom::NetworkContextParams* params,
      bool quic_disabled,
      net::NetLog* net_log,
      net::NetworkQualityEstimator* network_quality_estimator);

 private:
  std::unique_ptr<net::ProxyResolutionService> CreateProxyResolutionService(
      std::unique_ptr<net::ProxyConfigService> proxy_config_service,
      net::URLRequestContext* url_request_context,
      net::HostResolver* host_resolver,
      net::NetworkDelegate* network_delegate,
      net::NetLog* net_log) override;

  std::unique_ptr<net::DhcpPacFileFetcherFactory> dhcp_fetcher_factory_;

  proxy_resolver::mojom::ProxyResolverFactoryPtr mojo_proxy_resolver_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextBuilderMojo);
};

}  // namespace network

#endif  // SERVICES_NETWORK_URL_REQUEST_CONTEXT_BUILDER_MOJO_H_
