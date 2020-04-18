// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/url_loader_factory_impl.h"

#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader.mojom.h"

namespace content {

URLLoaderFactoryImpl::URLLoaderFactoryImpl(
    scoped_refptr<ResourceRequesterInfo> requester_info)
    : requester_info_(std::move(requester_info)) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK((requester_info_->IsRenderer() && requester_info_->filter()) ||
         requester_info_->IsNavigationPreload() ||
         requester_info_->IsCertificateFetcherForSignedExchange());
}

URLLoaderFactoryImpl::~URLLoaderFactoryImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void URLLoaderFactoryImpl::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& url_request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  ResourceDispatcherHostImpl* rdh = ResourceDispatcherHostImpl::Get();
  rdh->OnRequestResourceWithMojo(
      requester_info_.get(), routing_id, request_id, options, url_request,
      std::move(request), std::move(client),
      static_cast<net::NetworkTrafficAnnotationTag>(traffic_annotation));
}

void URLLoaderFactoryImpl::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  // The cloned factories stop working when this factory is destructed.
  bindings_.AddBinding(this, std::move(request));
}

}  // namespace content
