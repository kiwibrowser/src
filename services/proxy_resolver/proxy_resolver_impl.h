// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_IMPL_H_
#define SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_IMPL_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/proxy_resolution/proxy_resolver.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"

namespace net {
class ProxyResolverV8Tracing;
}  // namespace net

namespace service_manager {
class ServiceContextRef;
}  // namespace service_manager

namespace proxy_resolver {

class ProxyResolverImpl : public mojom::ProxyResolver {
 public:
  ProxyResolverImpl(
      std::unique_ptr<net::ProxyResolverV8Tracing> resolver,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  ~ProxyResolverImpl() override;

 private:
  class Job;

  // mojom::ProxyResolver overrides.
  void GetProxyForUrl(const GURL& url,
                      mojom::ProxyResolverRequestClientPtr client) override;

  void DeleteJob(Job* job);

  std::unique_ptr<net::ProxyResolverV8Tracing> resolver_;
  std::map<Job*, std::unique_ptr<Job>> resolve_jobs_;
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(ProxyResolverImpl);
};

}  // namespace proxy_resolver

#endif  // SERVICES_PROXY_RESOLVER_PROXY_RESOLVER_IMPL_H_
