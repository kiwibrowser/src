// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_APPCACHE_APPCACHE_SUBRESOURCE_URL_FACTORY_H_
#define CONTENT_BROWSER_APPCACHE_APPCACHE_SUBRESOURCE_URL_FACTORY_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace content {

class AppCacheHost;
class AppCacheJob;
class AppCacheRequestHandler;
class AppCacheServiceImpl;
class URLLoaderFactoryGetter;

// Implements the URLLoaderFactory mojom for AppCache subresource requests.
class CONTENT_EXPORT AppCacheSubresourceURLFactory
    : public network::mojom::URLLoaderFactory {
 public:
  ~AppCacheSubresourceURLFactory() override;

  // Factory function to create an instance of the factory.
  // 1. The |factory_getter| parameter is used to query the network service
  //    to pass network requests to.
  // 2. The |host| parameter contains the appcache host instance. This is used
  //    to create the AppCacheRequestHandler instances for handling subresource
  //    requests.
  static void CreateURLLoaderFactory(
      URLLoaderFactoryGetter* factory_getter,
      base::WeakPtr<AppCacheHost> host,
      network::mojom::URLLoaderFactoryPtr* loader_factory);

  // network::mojom::URLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest url_loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;

  base::WeakPtr<AppCacheSubresourceURLFactory> GetWeakPtr();

 private:
  friend class AppCacheNetworkServiceBrowserTest;

  // TODO(michaeln): Declare SubresourceLoader here and add unittests.

  AppCacheSubresourceURLFactory(URLLoaderFactoryGetter* factory_getter,
                                base::WeakPtr<AppCacheHost> host);
  void OnConnectionError();

  mojo::BindingSet<network::mojom::URLLoaderFactory> bindings_;
  scoped_refptr<URLLoaderFactoryGetter> default_url_loader_factory_getter_;
  base::WeakPtr<AppCacheHost> appcache_host_;
  base::WeakPtrFactory<AppCacheSubresourceURLFactory> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(AppCacheSubresourceURLFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_APPCACHE_APPCACHE_URL_LOADER_FACTORY_H_
