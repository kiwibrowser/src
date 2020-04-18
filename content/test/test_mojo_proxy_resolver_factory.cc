// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/test_mojo_proxy_resolver_factory.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

TestMojoProxyResolverFactory::TestMojoProxyResolverFactory()
    : service_ref_factory_(base::DoNothing()), binding_(this) {
  proxy_resolver_factory_impl_.BindRequest(mojo::MakeRequest(&factory_),
                                           &service_ref_factory_);
}

TestMojoProxyResolverFactory::~TestMojoProxyResolverFactory() = default;

void TestMojoProxyResolverFactory::CreateResolver(
    const std::string& pac_script,
    proxy_resolver::mojom::ProxyResolverRequest req,
    proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client) {
  resolver_created_ = true;
  factory_->CreateResolver(pac_script, std::move(req), std::move(client));
}

proxy_resolver::mojom::ProxyResolverFactoryPtr
TestMojoProxyResolverFactory::CreateFactoryInterface() {
  DCHECK(!binding_.is_bound());
  proxy_resolver::mojom::ProxyResolverFactoryPtr mojo_factory;
  binding_.Bind(mojo::MakeRequest(&mojo_factory));
  return mojo_factory;
}

}  // namespace content
