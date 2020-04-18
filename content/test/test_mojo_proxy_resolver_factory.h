// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_
#define CONTENT_TEST_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace content {

// MojoProxyResolverFactory that runs PAC scripts in-process, for tests.
class TestMojoProxyResolverFactory
    : public proxy_resolver::mojom::ProxyResolverFactory {
 public:
  TestMojoProxyResolverFactory();
  ~TestMojoProxyResolverFactory() override;

  // Returns true if CreateResolver was called.
  bool resolver_created() const { return resolver_created_; }

  proxy_resolver::mojom::ProxyResolverFactoryPtr CreateFactoryInterface();

  // Overridden from interfaces::ProxyResolverFactory:
  void CreateResolver(
      const std::string& pac_script,
      proxy_resolver::mojom::ProxyResolverRequest req,
      proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client)
      override;

 private:
  service_manager::ServiceContextRefFactory service_ref_factory_;
  proxy_resolver::ProxyResolverFactoryImpl proxy_resolver_factory_impl_;

  proxy_resolver::mojom::ProxyResolverFactoryPtr factory_;

  mojo::Binding<ProxyResolverFactory> binding_;

  bool resolver_created_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestMojoProxyResolverFactory);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_MOJO_PROXY_RESOLVER_FACTORY_H_
