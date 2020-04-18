// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PROXY_SERVICE_MOJO_H_
#define SERVICES_NETWORK_PROXY_SERVICE_MOJO_H_

#include <memory>

#include "base/component_export.h"
#include "net/proxy_resolution/dhcp_pac_file_fetcher.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

namespace net {
class HostResolver;
class NetLog;
class NetworkDelegate;
class ProxyConfigService;
class PacFileFetcher;
class ProxyResolutionService;
}  // namespace net

namespace network {

// Creates a proxy resolution service that uses |mojo_proxy_factory| to create
// and connect to a Mojo proxy resolver service. This proxy service polls
// |proxy_config_service| to notice when the proxy settings change.
//
// |pac_file_fetcher| specifies the dependency to use for downloading
// any PAC scripts.
//
// |dhcp_pac_file_fetcher| specifies the dependency to use for attempting
// to retrieve the most appropriate PAC script configured in DHCP.
//
// |host_resolver| points to the host resolving dependency the PAC script
// should use for any DNS queries. It must remain valid throughout the
// lifetime of the ProxyResolutionService.
COMPONENT_EXPORT(NETWORK_SERVICE)
std::unique_ptr<net::ProxyResolutionService>
CreateProxyResolutionServiceUsingMojoFactory(
    proxy_resolver::mojom::ProxyResolverFactoryPtr mojo_proxy_factory,
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    std::unique_ptr<net::PacFileFetcher> pac_file_fetcher,
    std::unique_ptr<net::DhcpPacFileFetcher> dhcp_pac_file_fetcher,
    net::HostResolver* host_resolver,
    net::NetLog* net_log,
    net::NetworkDelegate* network_delegate);

}  // namespace network

#endif  // SERVICES_NETWORK_PROXY_SERVICE_MOJO_H_
