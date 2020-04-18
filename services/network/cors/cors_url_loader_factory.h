// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_CORS_CORS_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_CORS_CORS_URL_LOADER_FACTORY_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding_set.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace network {

struct ResourceRequest;

namespace cors {

// A factory class to create a URLLoader that supports CORS.
// This class takes a network::mojom::URLLoaderFactory instance in the
// constructor and owns it to make network requests for CORS preflight, and
// actual network request.
class COMPONENT_EXPORT(NETWORK_SERVICE) CORSURLLoaderFactory final
    : public mojom::URLLoaderFactory {
 public:
  explicit CORSURLLoaderFactory(
      std::unique_ptr<mojom::URLLoaderFactory> network_loader_factory);
  // TODO(yhirano): Remove |preflight_finalizer| when the network service is
  // fully enabled.
  CORSURLLoaderFactory(
      std::unique_ptr<mojom::URLLoaderFactory> network_loader_factory,
      const base::RepeatingCallback<void(int)>& preflight_finalizer);
  ~CORSURLLoaderFactory() override;

 private:
  // Implements mojom::URLLoaderFactory.
  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(mojom::URLLoaderFactoryRequest request) override;

  mojo::BindingSet<mojom::URLLoaderFactory> bindings_;

  std::unique_ptr<mojom::URLLoaderFactory> network_loader_factory_;

  base::RepeatingCallback<void(int)> preflight_finalizer_;

  // The factory owns the CORSURLLoader it creates.
  mojo::StrongBindingSet<mojom::URLLoader> loader_bindings_;

  DISALLOW_COPY_AND_ASSIGN(CORSURLLoaderFactory);
};

}  // namespace cors

}  // namespace network

#endif  // SERVICES_NETWORK_CORS_CORS_URL_LOADER_FACTORY_H_
