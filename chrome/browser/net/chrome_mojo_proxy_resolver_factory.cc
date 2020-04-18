// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

ChromeMojoProxyResolverFactory::ChromeMojoProxyResolverFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

ChromeMojoProxyResolverFactory::~ChromeMojoProxyResolverFactory() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

proxy_resolver::mojom::ProxyResolverFactoryPtr
ChromeMojoProxyResolverFactory::CreateWithStrongBinding() {
  proxy_resolver::mojom::ProxyResolverFactoryPtr proxy_resolver_factory;
  mojo::MakeStrongBinding(std::make_unique<ChromeMojoProxyResolverFactory>(),
                          mojo::MakeRequest(&proxy_resolver_factory));
  return proxy_resolver_factory;
}

void ChromeMojoProxyResolverFactory::CreateResolver(
    const std::string& pac_script,
    proxy_resolver::mojom::ProxyResolverRequest req,
    proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Bind a ProxyResolverFactory backed by the proxy resolver service, have it
  // create a ProxyResolverFactory and then destroy the factory, to avoid
  // keeping the service alive after all resolvers have been destroyed.
  proxy_resolver::mojom::ProxyResolverFactoryPtr resolver_factory;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(proxy_resolver::mojom::kProxyResolverServiceName,
                      mojo::MakeRequest(&resolver_factory));
  resolver_factory->CreateResolver(pac_script, std::move(req),
                                   std::move(client));
}
