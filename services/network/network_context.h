// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_NETWORK_CONTEXT_H_
#define SERVICES_NETWORK_NETWORK_CONTEXT_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/component_export.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "services/network/cookie_manager.h"
#include "services/network/http_cache_data_remover.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/tcp_socket.mojom.h"
#include "services/network/public/mojom/udp_socket.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "services/network/socket_factory.h"
#include "services/network/url_request_context_owner.h"

namespace net {
class CertVerifier;
class HttpAuthPreferences;
class NetworkQualityEstimator;
class ReportSender;
class StaticHttpUserAgentSettings;
class URLRequestContext;
}  // namespace net

namespace certificate_transparency {
class ChromeRequireCTDelegate;
class TreeStateTracker;
class STHReporter;
}  // namespace certificate_transparency

namespace network {
class NetworkService;
class ResourceScheduler;
class ResourceSchedulerClient;
class URLLoaderFactory;
class URLRequestContextBuilderMojo;
class WebSocketFactory;

// A NetworkContext creates and manages access to a URLRequestContext.
//
// When the network service is enabled, NetworkContexts are created through
// NetworkService's mojo interface and are owned jointly by the NetworkService
// and the NetworkContextPtr used to talk to them, and the NetworkContext is
// destroyed when either one is torn down.
//
// When the network service is disabled, NetworkContexts may be created through
// NetworkService::CreateNetworkContextWithBuilder, and take in a
// URLRequestContextBuilderMojo to seed construction of the NetworkContext's
// URLRequestContext. When that happens, the consumer takes ownership of the
// NetworkContext directly, has direct access to its URLRequestContext, and is
// responsible for destroying it before the NetworkService.
class COMPONENT_EXPORT(NETWORK_SERVICE) NetworkContext
    : public mojom::NetworkContext {
 public:
  using OnConnectionCloseCallback =
      base::OnceCallback<void(NetworkContext* network_context)>;

  NetworkContext(NetworkService* network_service,
                 mojom::NetworkContextRequest request,
                 mojom::NetworkContextParamsPtr params,
                 OnConnectionCloseCallback on_connection_close_callback =
                     OnConnectionCloseCallback());

  // DEPRECATED: Creates an in-process NetworkContext with a partially
  // pre-populated URLRequestContextBuilderMojo. This API should not be used
  // in new code, as some |params| configuration may be ignored, in favor of
  // the pre-configured URLRequestContextBuilderMojo configuration.
  NetworkContext(NetworkService* network_service,
                 mojom::NetworkContextRequest request,
                 mojom::NetworkContextParamsPtr params,
                 std::unique_ptr<URLRequestContextBuilderMojo> builder);

  // DEPRECATED: Creates a NetworkContext that simply wraps a consumer-provided
  // URLRequestContext that is not owned by the NetworkContext.
  // TODO(mmenke):  Remove this constructor when the network service ships.
  NetworkContext(NetworkService* network_service,
                 mojom::NetworkContextRequest request,
                 net::URLRequestContext* url_request_context);

  ~NetworkContext() override;

  // Sets a global CertVerifier to use when initializing all profiles.
  static void SetCertVerifierForTesting(net::CertVerifier* cert_verifier);

  net::URLRequestContext* url_request_context() { return url_request_context_; }

  NetworkService* network_service() { return network_service_; }

  ResourceScheduler* resource_scheduler() { return resource_scheduler_.get(); }

  bool block_third_party_cookies() const { return block_third_party_cookies_; }

  // Creates a URLLoaderFactory with a ResourceSchedulerClient specified. This
  // is used to reuse the existing ResourceSchedulerClient for cloned
  // URLLoaderFactory.
  void CreateURLLoaderFactory(
      mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryParamsPtr params,
      scoped_refptr<ResourceSchedulerClient> resource_scheduler_client);

  // mojom::NetworkContext implementation:
  void CreateURLLoaderFactory(
      mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryParamsPtr params) override;
  void GetCookieManager(mojom::CookieManagerRequest request) override;
  void GetRestrictedCookieManager(mojom::RestrictedCookieManagerRequest request,
                                  int32_t render_process_id,
                                  int32_t render_frame_id) override;
  void ClearNetworkingHistorySince(
      base::Time time,
      base::OnceClosure completion_callback) override;
  void ClearHttpCache(base::Time start_time,
                      base::Time end_time,
                      mojom::ClearDataFilterPtr filter,
                      ClearHttpCacheCallback callback) override;
  void ClearChannelIds(base::Time start_time,
                       base::Time end_time,
                       mojom::ClearDataFilterPtr filter,
                       ClearChannelIdsCallback callback) override;
  void ClearHttpAuthCache(base::Time start_time,
                          ClearHttpAuthCacheCallback callback) override;
  void ClearReportingCacheReports(
      mojom::ClearDataFilterPtr filter,
      ClearReportingCacheReportsCallback callback) override;
  void ClearReportingCacheClients(
      mojom::ClearDataFilterPtr filter,
      ClearReportingCacheClientsCallback callback) override;
  void ClearNetworkErrorLogging(
      mojom::ClearDataFilterPtr filter,
      ClearNetworkErrorLoggingCallback callback) override;
  void SetNetworkConditions(const std::string& profile_id,
                            mojom::NetworkConditionsPtr conditions) override;
  void SetAcceptLanguage(const std::string& new_accept_language) override;
  void SetCTPolicy(
      const std::vector<std::string>& required_hosts,
      const std::vector<std::string>& excluded_hosts,
      const std::vector<std::string>& excluded_spkis,
      const std::vector<std::string>& excluded_legacy_spkis) override;
  void CreateUDPSocket(mojom::UDPSocketRequest request,
                       mojom::UDPSocketReceiverPtr receiver) override;
  void CreateTCPServerSocket(
      const net::IPEndPoint& local_addr,
      uint32_t backlog,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPServerSocketRequest request,
      CreateTCPServerSocketCallback callback) override;
  void CreateTCPConnectedSocket(
      const base::Optional<net::IPEndPoint>& local_addr,
      const net::AddressList& remote_addr_list,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::TCPConnectedSocketRequest request,
      mojom::SocketObserverPtr observer,
      CreateTCPConnectedSocketCallback callback) override;
  void CreateWebSocket(mojom::WebSocketRequest request,
                       int32_t process_id,
                       int32_t render_frame_id,
                       const url::Origin& origin) override;
  void CreateNetLogExporter(mojom::NetLogExporterRequest request) override;
  void BlockThirdPartyCookies(bool block) override;
  void AddHSTSForTesting(const std::string& host,
                         base::Time expiry,
                         bool include_subdomains,
                         AddHSTSForTestingCallback callback) override;
  void SetFailingHttpTransactionForTesting(
      int32_t rv,
      SetFailingHttpTransactionForTestingCallback callback) override;

  // Disables use of QUIC by the NetworkContext.
  void DisableQuic();

  // Destroys the specified URLLoaderFactory. Called by the URLLoaderFactory
  // itself when it has no open pipes.
  void DestroyURLLoaderFactory(URLLoaderFactory* url_loader_factory);

 private:
  friend class URLRequestContextBuilderMojo;

  // Applies the values in |network_context_params| to |builder|, and builds
  // the URLRequestContext. If |out_http_user_agent_settings| is non-null, it
  // will be set to point to StaticHttpUserAgentSettings owned by the
  // URLRequestContext.
  static URLRequestContextOwner ApplyContextParamsToBuilder(
      URLRequestContextBuilderMojo* builder,
      mojom::NetworkContextParams* network_context_params,
      bool quic_disabled,
      net::NetLog* net_log,
      net::NetworkQualityEstimator* network_quality_estimator,
      certificate_transparency::STHReporter* sth_reporter,
      std::unique_ptr<certificate_transparency::TreeStateTracker>*
          out_tree_state_tracker,
      std::unique_ptr<certificate_transparency::ChromeRequireCTDelegate>*
          out_require_ct_delegate,
      std::unique_ptr<net::ReportSender>* out_certificate_report_sender,
      net::StaticHttpUserAgentSettings** out_http_user_agent_settings);

  // Invoked when the HTTP cache was cleared. Invokes |callback|.
  void OnHttpCacheCleared(ClearHttpCacheCallback callback,
                          HttpCacheDataRemover* remover);

  // On connection errors the NetworkContext destroys itself.
  void OnConnectionError();

  URLRequestContextOwner MakeURLRequestContext(
      mojom::NetworkContextParams* network_context_params);

  NetworkService* const network_service_;

  std::unique_ptr<ResourceScheduler> resource_scheduler_;

  std::unique_ptr<net::HttpAuthPreferences> http_auth_preferences_;

  // Holds owning pointer to |url_request_context_|. Will contain a nullptr for
  // |url_request_context| when the NetworkContextImpl doesn't own its own
  // URLRequestContext.
  URLRequestContextOwner url_request_context_owner_;

  net::URLRequestContext* url_request_context_;

  mojom::NetworkContextParamsPtr params_;

  // If non-null, called when the mojo pipe for the NetworkContext is closed.
  OnConnectionCloseCallback on_connection_close_callback_;

  mojo::Binding<mojom::NetworkContext> binding_;

  std::unique_ptr<CookieManager> cookie_manager_;

  std::unique_ptr<SocketFactory> socket_factory_;

#if !defined(OS_IOS)
  std::unique_ptr<WebSocketFactory> websocket_factory_;
#endif  // !defined(OS_IOS)

  std::vector<std::unique_ptr<HttpCacheDataRemover>> http_cache_data_removers_;

  // This must be below |url_request_context_| so that the URLRequestContext
  // outlives all the URLLoaderFactories and URLLoaders that depend on it.
  std::set<std::unique_ptr<URLLoaderFactory>, base::UniquePtrComparator>
      url_loader_factories_;

  mojo::StrongBindingSet<mojom::NetLogExporter> net_log_exporter_bindings_;

  int current_resource_scheduler_client_id_ = 0;

  // Owned by the URLRequestContext
  net::StaticHttpUserAgentSettings* user_agent_settings_ = nullptr;

  // TODO(yhirano): Consult with switches::kDisableResourceScheduler.
  constexpr static bool enable_resource_scheduler_ = true;

  bool block_third_party_cookies_ = false;

  // Pointed to by the TransportSecurityState (owned by the
  // URLRequestContext), and must be disconnected from it before it's destroyed.
  std::unique_ptr<net::ReportSender> certificate_report_sender_;

  std::unique_ptr<certificate_transparency::ChromeRequireCTDelegate>
      require_ct_delegate_;
  std::unique_ptr<certificate_transparency::TreeStateTracker> ct_tree_tracker_;

  DISALLOW_COPY_AND_ASSIGN(NetworkContext);
};

}  // namespace network

#endif  // SERVICES_NETWORK_NETWORK_CONTEXT_H_
