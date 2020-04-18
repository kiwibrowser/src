// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/web_package/signed_exchange_url_loader_factory_for_non_network_service.h"

#include "base/feature_list.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/browser/loader/url_loader_factory_impl.h"
#include "content/browser/web_package/signed_exchange_utils.h"
#include "content/public/common/content_features.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/cpp/features.h"

namespace content {

SignedExchangeURLLoaderFactoryForNonNetworkService::
    SignedExchangeURLLoaderFactoryForNonNetworkService(
        ResourceContext* resource_context,
        net::URLRequestContextGetter* url_request_context_getter)
    : resource_context_(resource_context),
      url_request_context_getter_(url_request_context_getter) {
  DCHECK(!base::FeatureList::IsEnabled(network::features::kNetworkService));
  DCHECK(signed_exchange_utils::IsSignedExchangeHandlingEnabled());
}

SignedExchangeURLLoaderFactoryForNonNetworkService::
    ~SignedExchangeURLLoaderFactoryForNonNetworkService() = default;

void SignedExchangeURLLoaderFactoryForNonNetworkService::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest loader_request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (!url_request_context_getter_->GetURLRequestContext()) {
    // The context has been destroyed.
    return;
  }
  std::unique_ptr<URLLoaderFactoryImpl> url_loader_factory =
      std::make_unique<URLLoaderFactoryImpl>(
          ResourceRequesterInfo::CreateForCertificateFetcherForSignedExchange(
              base::BindRepeating(
                  &SignedExchangeURLLoaderFactoryForNonNetworkService::
                      GetContextsCallback,
                  base::Unretained(this))));
  url_loader_factory->CreateLoaderAndStart(
      std::move(loader_request), routing_id, request_id, options, request,
      std::move(client), traffic_annotation);
}

void SignedExchangeURLLoaderFactoryForNonNetworkService::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  NOTREACHED();
}

std::unique_ptr<network::SharedURLLoaderFactoryInfo>
SignedExchangeURLLoaderFactoryForNonNetworkService::Clone() {
  NOTREACHED();
  return nullptr;
}

void SignedExchangeURLLoaderFactoryForNonNetworkService::GetContextsCallback(
    ResourceType resource_type,
    ResourceContext** resource_context_out,
    net::URLRequestContext** request_context_out) {
  DCHECK(url_request_context_getter_->GetURLRequestContext());
  *resource_context_out = resource_context_;
  *request_context_out = url_request_context_getter_->GetURLRequestContext();
}

}  // namespace content
