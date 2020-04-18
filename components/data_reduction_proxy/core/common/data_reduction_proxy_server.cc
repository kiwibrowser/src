// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/common/data_reduction_proxy_server.h"

#include "base/logging.h"

namespace data_reduction_proxy {

DataReductionProxyServer::DataReductionProxyServer(
    const net::ProxyServer& proxy_server,
    ProxyServer_ProxyType proxy_type)
    : proxy_server_(proxy_server), proxy_type_(proxy_type) {}

bool DataReductionProxyServer::operator==(
    const DataReductionProxyServer& other) const {
  return proxy_server_ == other.proxy_server_ &&
         proxy_type_ == other.proxy_type_;
}

bool DataReductionProxyServer::SupportsResourceType(
    ResourceTypeProvider::ContentType content_type) const {
  switch (proxy_type_) {
    case ProxyServer_ProxyType_CORE:
      return true;
    case ProxyServer_ProxyType_UNSPECIFIED_TYPE:
      switch (content_type) {
        case ResourceTypeProvider::CONTENT_TYPE_UNKNOWN:
          return true;
        case ResourceTypeProvider::CONTENT_TYPE_MEDIA:
          return false;
        case ResourceTypeProvider::CONTENT_TYPE_MAIN_FRAME:
          return true;
        case ResourceTypeProvider::CONTENT_TYPE_MAX:
          NOTREACHED();
          return true;
      }
  }
  return true;
}

// static
std::vector<net::ProxyServer>
DataReductionProxyServer::ConvertToNetProxyServers(
    const std::vector<DataReductionProxyServer>& data_reduction_proxy_servers) {
  std::vector<net::ProxyServer> net_proxy_servers;
  net_proxy_servers.reserve(data_reduction_proxy_servers.size());

  for (const auto& data_reduction_proxy_server : data_reduction_proxy_servers)
    net_proxy_servers.push_back(data_reduction_proxy_server.proxy_server());
  return net_proxy_servers;
}

bool DataReductionProxyServer::IsCoreProxy() const {
  return proxy_type_ == ProxyServer_ProxyType_CORE;
}

bool DataReductionProxyServer::IsSecureProxy() const {
  return proxy_server_.is_https() || proxy_server_.is_quic();
}

}  // namespace data_reduction_proxy
