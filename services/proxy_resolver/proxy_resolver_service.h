// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_SERVICE_H_
#define SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_SERVICE_H_

#include <memory>
#include <string>

#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace proxy_resolver {

class ProxyResolverService : public service_manager::Service {
 public:
  ProxyResolverService();
  ~ProxyResolverService() override;

  // Factory method for creating the service.
  static std::unique_ptr<service_manager::Service> CreateService();

  // Lifescycle events that occur after the service has started to spinup.
  void OnStart() override;
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override;

 private:
  void OnProxyResolverFactoryRequest(
      proxy_resolver::mojom::ProxyResolverFactoryRequest request);

  ProxyResolverFactoryImpl proxy_resolver_factory_;

  service_manager::BinderRegistry registry_;

  // State needed to manage service lifecycle and lifecycle of bound clients.
  // Should be last in the list, just like a WeakPtrFactory.
  std::unique_ptr<service_manager::ServiceContextRefFactory> ref_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverService);
};

}  // namespace proxy_resolver

#endif  // SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_SERVICE_H_
