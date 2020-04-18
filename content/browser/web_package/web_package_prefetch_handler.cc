// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/web_package_prefetch_handler.h"

#include "base/callback.h"
#include "base/feature_list.h"
#include "content/browser/web_package/signed_exchange_devtools_proxy.h"
#include "content/browser/web_package/signed_exchange_url_loader_factory_for_non_network_service.h"
#include "content/browser/web_package/web_package_loader.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {

WebPackagePrefetchHandler::WebPackagePrefetchHandler(
    base::RepeatingCallback<int(void)> frame_tree_node_id_getter,
    bool report_raw_headers,
    const network::ResourceResponseHead& response,
    network::mojom::URLLoaderPtr network_loader,
    network::mojom::URLLoaderClientRequest network_client_request,
    scoped_refptr<network::SharedURLLoaderFactory> network_loader_factory,
    url::Origin request_initiator,
    const GURL& outer_request_url,
    URLLoaderThrottlesGetter loader_throttles_getter,
    ResourceContext* resource_context,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    network::mojom::URLLoaderClient* forwarding_client)
    : loader_client_binding_(this), forwarding_client_(forwarding_client) {
  network::mojom::URLLoaderClientEndpointsPtr endpoints =
      network::mojom::URLLoaderClientEndpoints::New(
          std::move(network_loader).PassInterface(),
          std::move(network_client_request));
  network::mojom::URLLoaderClientPtr client;
  loader_client_binding_.Bind(mojo::MakeRequest(&client));
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory;
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    url_loader_factory = base::MakeRefCounted<
        SignedExchangeURLLoaderFactoryForNonNetworkService>(
        resource_context, request_context_getter.get());
  } else {
    url_loader_factory = std::move(network_loader_factory);
  }
  web_package_loader_ = std::make_unique<WebPackageLoader>(
      response, std::move(client), std::move(endpoints),
      std::move(request_initiator), network::mojom::kURLLoadOptionNone,
      std::make_unique<SignedExchangeDevToolsProxy>(
          outer_request_url, response, std::move(frame_tree_node_id_getter),
          base::nullopt /* devtools_navigation_token */, report_raw_headers),
      std::move(url_loader_factory), loader_throttles_getter,
      request_context_getter);
}

WebPackagePrefetchHandler::~WebPackagePrefetchHandler() = default;

network::mojom::URLLoaderClientRequest
WebPackagePrefetchHandler::FollowRedirect(
    network::mojom::URLLoaderRequest loader_request) {
  DCHECK(web_package_loader_);
  network::mojom::URLLoaderClientPtr client;
  auto pending_request = mojo::MakeRequest(&client);
  web_package_loader_->ConnectToClient(std::move(client));
  mojo::MakeStrongBinding(std::move(web_package_loader_),
                          std::move(loader_request));
  return pending_request;
}

void WebPackagePrefetchHandler::OnReceiveResponse(
    const network::ResourceResponseHead& head,
    network::mojom::DownloadedTempFilePtr downloaded_file) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const network::ResourceResponseHead& head) {
  forwarding_client_->OnReceiveRedirect(redirect_info, head);
}

void WebPackagePrefetchHandler::OnDataDownloaded(int64_t data_length,
                                                 int64_t encoded_length) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    base::OnceCallback<void()> callback) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  NOTREACHED();
}

void WebPackagePrefetchHandler::OnComplete(
    const network::URLLoaderCompletionStatus& status) {
  forwarding_client_->OnComplete(status);
}

}  // namespace content
