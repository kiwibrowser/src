// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/proxy_resolver/proxy_resolver_service.h"

#include <utility>

#include "build/build_config.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace proxy_resolver {

ProxyResolverService::ProxyResolverService() = default;

ProxyResolverService::~ProxyResolverService() = default;

std::unique_ptr<service_manager::Service>
ProxyResolverService::CreateService() {
  return std::make_unique<ProxyResolverService>();
}

void ProxyResolverService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      context()->CreateQuitClosure());
  registry_.AddInterface(
      base::Bind(&ProxyResolverService::OnProxyResolverFactoryRequest,
                 base::Unretained(this)));
}

void ProxyResolverService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

void ProxyResolverService::OnProxyResolverFactoryRequest(
    proxy_resolver::mojom::ProxyResolverFactoryRequest request) {
  proxy_resolver_factory_.BindRequest(std::move(request), ref_factory_.get());
}

}  // namespace proxy_resolver
