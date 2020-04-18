// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_TEST_TEST_NETWORK_CONTEXT_H_
#define SERVICES_NETWORK_TEST_TEST_NETWORK_CONTEXT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/ip_endpoint.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/cookie_manager.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/network/public/mojom/restricted_cookie_manager.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/public/mojom/udp_socket.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "url/origin.h"

namespace network {

// Noop implementation of mojom::NetworkContext.  Useful to override to create
// specialized mocks or fakes.
class TestNetworkContext : public mojom::NetworkContext {
 public:
  TestNetworkContext() = default;
  ~TestNetworkContext() override = default;

  void CreateURLLoaderFactory(
      mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryParamsPtr params) override {}
  void GetCookieManager(mojom::CookieManagerRequest cookie_manager) override {}
  void GetRestrictedCookieManager(
      mojom::RestrictedCookieManagerRequest restricted_cookie_manager,
      int32_t render_process_id,
      int32_t render_frame_id) override {}
  void ClearNetworkingHistorySince(
      base::Time start_time,
      ClearNetworkingHistorySinceCallback callback) override {}
  void ClearHttpCache(base::Time start_time,
                      base::Time end_time,
                      mojom::ClearDataFilterPtr filter,
                      ClearHttpCacheCallback callback) override {}
  void ClearChannelIds(base::Time start_time,
                       base::Time end_time,
                       mojom::ClearDataFilterPtr filter,
                       ClearChannelIdsCallback callback) override {}
  void ClearHttpAuthCache(base::Time start_time,
                          ClearHttpAuthCacheCallback callback) override {}
  void ClearReportingCacheReports(
      mojom::ClearDataFilterPtr filter,
      ClearReportingCacheReportsCallback callback) override {}
  void ClearReportingCacheClients(
      mojom::ClearDataFilterPtr filter,
      ClearReportingCacheClientsCallback callback) override {}
  void ClearNetworkErrorLogging(
      mojom::ClearDataFilterPtr filter,
      ClearNetworkErrorLoggingCallback callback) override {}
  void SetNetworkConditions(const std::string& profile_id,
                            mojom::NetworkConditionsPtr conditions) override {}
  void SetAcceptLanguage(const std::string& new_accept_language) override {}
  void SetCTPolicy(
      const std::vector<std::string>& required_hosts,
      const std::vector<std::string>& excluded_hosts,
      const std::vector<std::string>& excluded_spkis,
      const std::vector<std::string>& excluded_legacy_spkis) override {}
  void CreateUDPSocket(mojom::UDPSocketRequest request,
                       mojom::UDPSocketReceiverPtr receiver) override {}
  void CreateTCPServerSocket(
      const net::IPEndPoint& local_addr,
      uint32_t backlog,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPServerSocketRequest socket,
      CreateTCPServerSocketCallback callback) override {}
  void CreateTCPConnectedSocket(
      const base::Optional<net::IPEndPoint>& local_addr,
      const net::AddressList& remote_addr_list,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPConnectedSocketRequest socket,
      mojom::SocketObserverPtr observer,
      CreateTCPConnectedSocketCallback callback) override {}
  void CreateWebSocket(mojom::WebSocketRequest request,
                       int32_t process_id,
                       int32_t render_frame_id,
                       const url::Origin& origin) override {}
  void CreateNetLogExporter(mojom::NetLogExporterRequest exporter) override {}
  void BlockThirdPartyCookies(bool block) override {}
  void AddHSTSForTesting(const std::string& host,
                         base::Time expiry,
                         bool include_subdomains,
                         AddHSTSForTestingCallback callback) override {}
  void SetFailingHttpTransactionForTesting(
      int32_t rv,
      SetFailingHttpTransactionForTestingCallback callback) override{};
};

}  // namespace network

#endif  // SERVICES_NETWORK_TEST_TEST_NETWORK_CONTEXT_H_
