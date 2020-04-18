// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/shared_worker/shared_worker_script_loader_factory.h"

#include <memory>
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_provider_host.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/browser/shared_worker/shared_worker_script_loader.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_provider_type.mojom.h"

namespace content {

SharedWorkerScriptLoaderFactory::SharedWorkerScriptLoaderFactory(
    ServiceWorkerContextWrapper* context,
    base::WeakPtr<ServiceWorkerProviderHost> service_worker_provider_host,
    ResourceContext* resource_context,
    scoped_refptr<network::SharedURLLoaderFactory> loader_factory)
    : service_worker_provider_host_(service_worker_provider_host),
      resource_context_(resource_context),
      loader_factory_(std::move(loader_factory)) {
  DCHECK(ServiceWorkerUtils::IsServicificationEnabled());
  DCHECK_EQ(service_worker_provider_host_->provider_type(),
            blink::mojom::ServiceWorkerProviderType::kForSharedWorker);
}

SharedWorkerScriptLoaderFactory::~SharedWorkerScriptLoaderFactory() {}

void SharedWorkerScriptLoaderFactory::CreateLoaderAndStart(
    network::mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const network::ResourceRequest& resource_request,
    network::mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // Handle only the main script (RESOURCE_TYPE_SHARED_WORKER). Import scripts
  // should go to the network loader or controller.
  if (resource_request.resource_type != RESOURCE_TYPE_SHARED_WORKER) {
    mojo::ReportBadMessage(
        "SharedWorkerScriptLoaderFactory should only get requests for shared "
        "worker scripts");
    return;
  }

  // Create a SharedWorkerScriptLoader to load the script.
  mojo::MakeStrongBinding(
      std::make_unique<SharedWorkerScriptLoader>(
          routing_id, request_id, options, resource_request, std::move(client),
          service_worker_provider_host_, resource_context_, loader_factory_,
          traffic_annotation),
      std::move(request));
}

void SharedWorkerScriptLoaderFactory::Clone(
    network::mojom::URLLoaderFactoryRequest request) {
  // This method is required to support synchronous requests, which shared
  // worker script requests are not.
  NOTIMPLEMENTED();
}

}  // namespace content
