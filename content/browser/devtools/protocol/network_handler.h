// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "content/browser/devtools/devtools_url_loader_interceptor.h"
#include "content/browser/devtools/protocol/devtools_domain_handler.h"
#include "content/browser/devtools/protocol/network.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/net_errors.h"
#include "net/cookies/canonical_cookie.h"
#include "services/network/public/mojom/network_service.mojom.h"

namespace base {
class UnguessableToken;
};

namespace net {
class HttpRequestHeaders;
class URLRequest;
class SSLInfo;
}  // namespace net

namespace network {
struct ResourceResponseHead;
struct ResourceRequest;
struct URLLoaderCompletionStatus;
}  // namespace network

namespace content {
class BrowserContext;
class DevToolsAgentHostImpl;
class DevToolsIOContext;
class RenderFrameHostImpl;
class InterceptionHandle;
class NavigationHandle;
class NavigationRequest;
class NavigationThrottle;
class SignedExchangeHeader;
class StoragePartition;
struct GlobalRequestID;
struct InterceptedRequestInfo;

namespace protocol {
class BackgroundSyncRestorer;

class NetworkHandler : public DevToolsDomainHandler,
                       public Network::Backend {
 public:
  NetworkHandler(const std::string& host_id, DevToolsIOContext* io_context);
  ~NetworkHandler() override;

  static std::vector<NetworkHandler*> ForAgentHost(DevToolsAgentHostImpl* host);

  void Wire(UberDispatcher* dispatcher) override;
  void SetRenderer(int render_process_id,
                   RenderFrameHostImpl* frame_host) override;

  Response Enable(Maybe<int> max_total_size,
                  Maybe<int> max_resource_size,
                  Maybe<int> max_post_data_size) override;
  Response Disable() override;

  Response SetCacheDisabled(bool cache_disabled) override;

  void ClearBrowserCache(
      std::unique_ptr<ClearBrowserCacheCallback> callback) override;

  void ClearBrowserCookies(
      std::unique_ptr<ClearBrowserCookiesCallback> callback) override;

  void GetCookies(Maybe<protocol::Array<String>> urls,
                  std::unique_ptr<GetCookiesCallback> callback) override;
  void GetAllCookies(std::unique_ptr<GetAllCookiesCallback> callback) override;
  void DeleteCookies(const std::string& name,
                     Maybe<std::string> url,
                     Maybe<std::string> domain,
                     Maybe<std::string> path,
                     std::unique_ptr<DeleteCookiesCallback> callback) override;
  void SetCookie(const std::string& name,
                 const std::string& value,
                 Maybe<std::string> url,
                 Maybe<std::string> domain,
                 Maybe<std::string> path,
                 Maybe<bool> secure,
                 Maybe<bool> http_only,
                 Maybe<std::string> same_site,
                 Maybe<double> expires,
                 std::unique_ptr<SetCookieCallback> callback) override;
  void SetCookies(
      std::unique_ptr<protocol::Array<Network::CookieParam>> cookies,
      std::unique_ptr<SetCookiesCallback> callback) override;

  Response SetUserAgentOverride(const std::string& user_agent) override;
  Response SetExtraHTTPHeaders(
      std::unique_ptr<Network::Headers> headers) override;
  Response CanEmulateNetworkConditions(bool* result) override;
  Response EmulateNetworkConditions(
      bool offline,
      double latency,
      double download_throughput,
      double upload_throughput,
      Maybe<protocol::Network::ConnectionType> connection_type) override;
  Response SetBypassServiceWorker(bool bypass) override;

  DispatchResponse SetRequestInterception(
      std::unique_ptr<protocol::Array<protocol::Network::RequestPattern>>
          patterns) override;
  void ContinueInterceptedRequest(
      const std::string& request_id,
      Maybe<std::string> error_reason,
      Maybe<std::string> base64_raw_response,
      Maybe<std::string> url,
      Maybe<std::string> method,
      Maybe<std::string> post_data,
      Maybe<protocol::Network::Headers> headers,
      Maybe<protocol::Network::AuthChallengeResponse> auth_challenge_response,
      std::unique_ptr<ContinueInterceptedRequestCallback> callback) override;

  void GetResponseBodyForInterception(
      const String& interception_id,
      std::unique_ptr<GetResponseBodyForInterceptionCallback> callback)
      override;
  void TakeResponseBodyForInterceptionAsStream(
      const String& interception_id,
      std::unique_ptr<TakeResponseBodyForInterceptionAsStreamCallback> callback)
      override;

  bool MaybeCreateProxyForInterception(
      const base::UnguessableToken& frame_token,
      int process_id,
      bool is_download,
      network::mojom::URLLoaderFactoryRequest* target_factory_request);

  void ApplyOverrides(net::HttpRequestHeaders* headers,
                      bool* skip_service_worker,
                      bool* disable_cache);
  void NavigationRequestWillBeSent(const NavigationRequest& nav_request);
  void RequestSent(const std::string& request_id,
                   const std::string& loader_id,
                   const network::ResourceRequest& request,
                   const char* initiator_type,
                   const base::Optional<GURL>& initiator_url);
  void ResponseReceived(const std::string& request_id,
                        const std::string& loader_id,
                        const GURL& url,
                        const char* resource_type,
                        const network::ResourceResponseHead& head,
                        Maybe<std::string> frame_id);
  void LoadingComplete(
      const std::string& request_id,
      const char* resource_type,
      const network::URLLoaderCompletionStatus& completion_status);

  void OnSignedExchangeReceived(
      base::Optional<const base::UnguessableToken> devtools_navigation_token,
      const GURL& outer_request_url,
      const network::ResourceResponseHead& outer_response,
      const base::Optional<SignedExchangeHeader>& header,
      const base::Optional<net::SSLInfo>& ssl_info,
      const std::vector<std::string>& error_messages);

  bool enabled() const { return enabled_; }

  Network::Frontend* frontend() const { return frontend_.get(); }

  static GURL ClearUrlRef(const GURL& url);
  static std::unique_ptr<Network::Request> CreateRequestFromResourceRequest(
      const network::ResourceRequest& request);
  static std::unique_ptr<Network::Request> CreateRequestFromURLRequest(
      const net::URLRequest* request);

  std::unique_ptr<NavigationThrottle> CreateThrottleForNavigation(
      NavigationHandle* navigation_handle);
  bool ShouldCancelNavigation(const GlobalRequestID& global_request_id);
  void WillSendNavigationRequest(net::HttpRequestHeaders* headers,
                                 bool* skip_service_worker,
                                 bool* disable_cache);

 private:
  void RequestIntercepted(std::unique_ptr<InterceptedRequestInfo> request_info);
  void SetNetworkConditions(network::mojom::NetworkConditionsPtr conditions);

  void OnResponseBodyPipeTaken(
      std::unique_ptr<TakeResponseBodyForInterceptionAsStreamCallback> callback,
      Response response,
      mojo::ScopedDataPipeConsumerHandle pipe,
      const std::string& mime_type);

  const std::string host_id_;
  DevToolsIOContext* const io_context_;

  std::unique_ptr<Network::Frontend> frontend_;
  BrowserContext* browser_context_;
  StoragePartition* storage_partition_;
  RenderFrameHostImpl* host_;
  bool enabled_;
  std::string user_agent_;
  std::vector<std::pair<std::string, std::string>> extra_headers_;
  std::unique_ptr<InterceptionHandle> interception_handle_;
  std::unique_ptr<DevToolsURLLoaderInterceptor> url_loader_interceptor_;
  bool bypass_service_worker_;
  bool cache_disabled_;
  std::unique_ptr<BackgroundSyncRestorer> background_sync_restorer_;
  base::WeakPtrFactory<NetworkHandler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkHandler);
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_NETWORK_HANDLER_H_
