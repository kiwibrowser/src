// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/cors/cors_url_loader_factory.h"

#include "services/network/cors/cors_url_loader.h"
#include "services/network/public/cpp/features.h"

namespace network {

namespace cors {

CORSURLLoaderFactory::CORSURLLoaderFactory(
    std::unique_ptr<mojom::URLLoaderFactory> network_loader_factory)
    : network_loader_factory_(std::move(network_loader_factory)) {}

CORSURLLoaderFactory::CORSURLLoaderFactory(
    std::unique_ptr<mojom::URLLoaderFactory> network_loader_factory,
    const base::RepeatingCallback<void(int)>& preflight_finalizer)
    : network_loader_factory_(std::move(network_loader_factory)),
      preflight_finalizer_(preflight_finalizer) {}

CORSURLLoaderFactory::~CORSURLLoaderFactory() = default;

void CORSURLLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  if (base::FeatureList::IsEnabled(features::kOutOfBlinkCORS)) {
    loader_bindings_.AddBinding(
        std::make_unique<CORSURLLoader>(
            routing_id, request_id, options, resource_request,
            std::move(client), traffic_annotation,
            network_loader_factory_.get(), preflight_finalizer_),
        std::move(request));
  } else {
    network_loader_factory_->CreateLoaderAndStart(
        std::move(request), routing_id, request_id, options, resource_request,
        std::move(client), traffic_annotation);
  }
}

void CORSURLLoaderFactory::Clone(mojom::URLLoaderFactoryRequest request) {
  // The cloned factories stop working when this factory is destructed.
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace cors

}  // namespace network
