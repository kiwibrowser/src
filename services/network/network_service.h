// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_SERVICE_H_
#define SERVICES_NETWORK_NETWORK_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/component_export.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/log/net_log.h"
#include "services/network/keepalive_statistics_recorder.h"
#include "services/network/network_change_manager.h"
#include "services/network/network_service.h"
#include "services/network/public/mojom/network_change_manager.mojom.h"
#include "services/network/public/mojom/network_service.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace net {
class HostResolver;
class LoggingNetworkChangeObserver;
class NetworkQualityEstimator;
class URLRequestContext;
}  // namespace net

namespace certificate_transparency {
class STHDistributor;
class STHReporter;
}  // namespace certificate_transparency

namespace network {

class NetworkContext;
class NetworkUsageAccumulator;
class MojoNetLog;
class URLRequestContextBuilderMojo;

class COMPONENT_EXPORT(NETWORK_SERVICE) NetworkService
    : public service_manager::Service,
      public network::mojom::NetworkService {
 public:
  // |net_log| is an optional shared NetLog, which will be used instead of the
  // service's own NetLog. It must outlive the NetworkService.
  //
  // TODO(https://crbug.com/767450): Once the NetworkService can always create
  // its own NetLog in production, remove the |net_log| argument.
  NetworkService(std::unique_ptr<service_manager::BinderRegistry> registry,
                 mojom::NetworkServiceRequest request = nullptr,
                 net::NetLog* net_log = nullptr);

  ~NetworkService() override;

  // Can be used to seed a NetworkContext with a consumer-configured
  // URLRequestContextBuilder, which |params| will then be applied to. The
  // results URLRequestContext will be written to |url_request_context|, which
  // is owned by the NetworkContext, and can be further modified before first
  // use. The returned NetworkContext must be destroyed before the
  // NetworkService.
  //
  // This method is intended to ease the transition to an out-of-process
  // NetworkService, and will be removed once that ships. It should only be
  // called if the network service is disabled.
  std::unique_ptr<mojom::NetworkContext> CreateNetworkContextWithBuilder(
      mojom::NetworkContextRequest request,
      mojom::NetworkContextParamsPtr params,
      std::unique_ptr<URLRequestContextBuilderMojo> builder,
      net::URLRequestContext** url_request_context);

  // Allows late binding if the mojo request wasn't specified in the
  // constructor.
  void Bind(mojom::NetworkServiceRequest request);

  // Creates a NetworkService instance on the current thread, optionally using
  // the passed-in NetLog. Does not take ownership of |net_log|. Must be
  // destroyed before |net_log|.
  //
  // TODO(https://crbug.com/767450): Make it so NetworkService can always create
  // its own NetLog, instead of sharing one.
  static std::unique_ptr<NetworkService> Create(
      network::mojom::NetworkServiceRequest request,
      net::NetLog* net_log = nullptr);

  static std::unique_ptr<NetworkService> CreateForTesting();

  // These are called by NetworkContexts as they are being created and
  // destroyed.
  // TODO(mmenke):  Remove once all NetworkContexts are owned by the
  // NetworkService.
  void RegisterNetworkContext(NetworkContext* network_context);
  void DeregisterNetworkContext(NetworkContext* network_context);

  // Invokes net::CreateNetLogEntriesForActiveObjects(observer) on all
  // URLRequestContext's known to |this|.
  void CreateNetLogEntriesForActiveObjects(
      net::NetLog::ThreadSafeObserver* observer);

  // mojom::NetworkService implementation:
  void SetClient(mojom::NetworkServiceClientPtr client) override;
  void CreateNetworkContext(mojom::NetworkContextRequest request,
                            mojom::NetworkContextParamsPtr params) override;
  void DisableQuic() override;
  void SetRawHeadersAccess(uint32_t process_id, bool allow) override;
  void GetNetworkChangeManager(
      mojom::NetworkChangeManagerRequest request) override;
  void GetTotalNetworkUsages(
      mojom::NetworkService::GetTotalNetworkUsagesCallback callback) override;
  void UpdateSignedTreeHead(const net::ct::SignedTreeHead& sth) override;

  bool quic_disabled() const { return quic_disabled_; }
  bool HasRawHeadersAccess(uint32_t process_id) const;

  mojom::NetworkServiceClient* client() { return client_.get(); }
  net::NetworkQualityEstimator* network_quality_estimator() {
    return network_quality_estimator_.get();
  }
  net::NetLog* net_log() const;
  KeepaliveStatisticsRecorder* keepalive_statistics_recorder() {
    return &keepalive_statistics_recorder_;
  }
  net::HostResolver* host_resolver() { return host_resolver_.get(); }
  NetworkUsageAccumulator* network_usage_accumulator() {
    return network_usage_accumulator_.get();
  }

  certificate_transparency::STHReporter* sth_reporter();

 private:
  // service_manager::Service implementation.
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

  // Called by a NetworkContext when its mojo pipe is closed. Deletes the
  // context.
  void OnNetworkContextConnectionClosed(NetworkContext* network_context);

  std::unique_ptr<MojoNetLog> owned_net_log_;
  // TODO(https://crbug.com/767450): Remove this, once Chrome no longer creates
  // its own NetLog.
  net::NetLog* net_log_;

  mojom::NetworkServiceClientPtr client_;

  KeepaliveStatisticsRecorder keepalive_statistics_recorder_;

  // Observer that logs network changes to the NetLog. Must be below the NetLog
  // and the NetworkChangeNotifier (Once this class creates it), so it's
  // destroyed before them.
  std::unique_ptr<net::LoggingNetworkChangeObserver> network_change_observer_;

  std::unique_ptr<NetworkChangeManager> network_change_manager_;

  std::unique_ptr<service_manager::BinderRegistry> registry_;

  mojo::Binding<mojom::NetworkService> binding_;

  std::unique_ptr<net::NetworkQualityEstimator> network_quality_estimator_;

  std::unique_ptr<net::HostResolver> host_resolver_;

  std::unique_ptr<NetworkUsageAccumulator> network_usage_accumulator_;

  // NetworkContexts created by CreateNetworkContext(). They call into the
  // NetworkService when their connection is closed so that it can delete
  // them.  It will also delete them when the NetworkService itself is torn
  // down, as NetworkContexts share global state owned by the NetworkService, so
  // must be destroyed first.
  //
  // NetworkContexts created by CreateNetworkContextWithBuilder() are not owned
  // by the NetworkService, and must be destroyed by their owners before the
  // NetworkService itself is.
  std::set<std::unique_ptr<NetworkContext>, base::UniquePtrComparator>
      owned_network_contexts_;

  // List of all NetworkContexts that are associated with the NetworkService,
  // including ones it does not own.
  // TODO(mmenke): Once the NetworkService always owns NetworkContexts, merge
  // this with |owned_network_contexts_|.
  std::set<NetworkContext*> network_contexts_;

  std::set<uint32_t> processes_with_raw_headers_access_;

  bool quic_disabled_ = false;

  std::unique_ptr<certificate_transparency::STHDistributor> sth_distributor_;

  DISALLOW_COPY_AND_ASSIGN(NetworkService);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_SERVICE_H_
