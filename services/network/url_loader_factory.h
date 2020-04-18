// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_URL_LOADER_FACTORY_H_

#include <memory>
#include <set>

#include "base/containers/unique_ptr_adapters.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace network {

class NetworkContext;
class ResourceSchedulerClient;
class URLLoader;

// This class is an implementation of mojom::URLLoaderFactory that
// creates a mojom::URLLoader.
// A URLLoaderFactory has a pointer to ResourceSchedulerClient. A
// ResourceSchedulerClient is associated with cloned
// NetworkServiceURLLoaderFactories. Roughly one URLLoaderFactory
// is created for one frame in render process, so it means ResourceScheduler
// works on each frame.
// A URLLoaderFactory can be created with null ResourceSchedulerClient, in which
// case requests constructed by the factory will not be throttled.
//
// URLLoaderFactories own all the URLLoaders they were used to create. Once
// there are no live Mojo pipes to a URLLoaderFactory, and all URLLoaders it was
// used to created have been destroyed, it will tell the NetworkContext that
// owns it to destroy it.
class URLLoaderFactory : public mojom::URLLoaderFactory {
 public:
  // NOTE: |context| must outlive this instance.
  URLLoaderFactory(
      NetworkContext* context,
      mojom::URLLoaderFactoryParamsPtr params,
      scoped_refptr<ResourceSchedulerClient> resource_scheduler_client,
      mojom::URLLoaderFactoryRequest request);

  ~URLLoaderFactory() override;

  // mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& url_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  void DestroyURLLoader(URLLoader* url_loader);

  static constexpr int kMaxKeepaliveConnections = 256;
  static constexpr int kMaxKeepaliveConnectionsPerProcess = 20;
  static constexpr int kMaxKeepaliveConnectionsPerProcessForFetchAPI = 10;

 private:
  // If |binding_set_| and |url_loaders_| are both empty, tells the
  // NetworkContext to destroy |this|.
  void DeleteIfNeeded();

  // The NetworkContext that owns |this|.
  NetworkContext* const context_;
  mojom::URLLoaderFactoryParamsPtr params_;
  scoped_refptr<ResourceSchedulerClient> resource_scheduler_client_;

  mojo::BindingSet<mojom::URLLoaderFactory> binding_set_;
  std::set<std::unique_ptr<URLLoader>, base::UniquePtrComparator> url_loaders_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactory);
};

}  // namespace network

#endif  // SERVICES_NETWORK_URL_LOADER_FACTORY_H_
