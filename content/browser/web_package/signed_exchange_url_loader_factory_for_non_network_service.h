// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_URL_LOADER_FACTORY_FOR_NON_NETWORK_SERVICE_H_
#define CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_URL_LOADER_FACTORY_FOR_NON_NETWORK_SERVICE_H_

#include "content/public/common/resource_type.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace content {

class ResourceContext;

// A URLLoaderFactory used for fetching certificate of signed HTTP exchange
// when NetworkService is not enabled.
class SignedExchangeURLLoaderFactoryForNonNetworkService
    : public network::SharedURLLoaderFactory {
 public:
  SignedExchangeURLLoaderFactoryForNonNetworkService(
      ResourceContext* resource_context,
      net::URLRequestContextGetter* url_request_context_getter);

  // SharedURLLoaderFactory:
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader_request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override;

 private:
  ~SignedExchangeURLLoaderFactoryForNonNetworkService() override;

  void GetContextsCallback(ResourceType resource_type,
                           ResourceContext** resource_context_out,
                           net::URLRequestContext** request_context_out);

  ResourceContext* resource_context_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  DISALLOW_COPY_AND_ASSIGN(SignedExchangeURLLoaderFactoryForNonNetworkService);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEB_PACKAGE_SIGNED_EXCHANGE_URL_LOADER_FACTORY_FOR_NON_NETWORK_SERVICE_H_
