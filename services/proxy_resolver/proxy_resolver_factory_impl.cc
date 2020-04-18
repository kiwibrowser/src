// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/proxy_resolver/proxy_resolver_factory_impl.h"

#include <string>
#include <utility>

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/net_errors.h"
#include "net/proxy_resolution/mojo_proxy_resolver_v8_tracing_bindings.h"
#include "net/proxy_resolution/proxy_resolver_factory.h"
#include "net/proxy_resolution/proxy_resolver_v8_tracing.h"
#include "services/proxy_resolver/proxy_resolver_impl.h"

namespace proxy_resolver {

class ProxyResolverFactoryImpl::Job {
 public:
  Job(ProxyResolverFactoryImpl* parent,
      const scoped_refptr<net::PacFileData>& pac_script,
      net::ProxyResolverV8TracingFactory* proxy_resolver_factory,
      mojo::InterfaceRequest<mojom::ProxyResolver> request,
      mojom::ProxyResolverFactoryRequestClientPtr client,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~Job();

 private:
  // Mojo error handler.
  void OnConnectionError();

  void OnProxyResolverCreated(int error);

  ProxyResolverFactoryImpl* const parent_;
  std::unique_ptr<net::ProxyResolverV8Tracing> proxy_resolver_impl_;
  mojo::InterfaceRequest<mojom::ProxyResolver> proxy_request_;
  net::ProxyResolverV8TracingFactory* factory_;
  std::unique_ptr<net::ProxyResolverFactory::Request> request_;
  mojom::ProxyResolverFactoryRequestClientPtr client_ptr_;
  std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

ProxyResolverFactoryImpl::Job::Job(
    ProxyResolverFactoryImpl* factory,
    const scoped_refptr<net::PacFileData>& pac_script,
    net::ProxyResolverV8TracingFactory* proxy_resolver_factory,
    mojo::InterfaceRequest<mojom::ProxyResolver> request,
    mojom::ProxyResolverFactoryRequestClientPtr client,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : parent_(factory),
      proxy_request_(std::move(request)),
      factory_(proxy_resolver_factory),
      client_ptr_(std::move(client)),
      service_ref_(std::move(service_ref)) {
  client_ptr_.set_connection_error_handler(
      base::Bind(&ProxyResolverFactoryImpl::Job::OnConnectionError,
                 base::Unretained(this)));
  factory_->CreateProxyResolverV8Tracing(
      pac_script,
      std::make_unique<net::MojoProxyResolverV8TracingBindings<
          mojom::ProxyResolverFactoryRequestClient>>(client_ptr_.get()),
      &proxy_resolver_impl_,
      base::Bind(&ProxyResolverFactoryImpl::Job::OnProxyResolverCreated,
                 base::Unretained(this)),
      &request_);
}

ProxyResolverFactoryImpl::Job::~Job() = default;

void ProxyResolverFactoryImpl::Job::OnConnectionError() {
  client_ptr_->ReportResult(net::ERR_PAC_SCRIPT_TERMINATED);
  parent_->RemoveJob(this);
}

void ProxyResolverFactoryImpl::Job::OnProxyResolverCreated(int error) {
  if (error == net::OK) {
    mojo::MakeStrongBinding(
        std::make_unique<ProxyResolverImpl>(std::move(proxy_resolver_impl_),
                                            std::move(service_ref_)),
        std::move(proxy_request_));
  }
  client_ptr_->ReportResult(error);
  parent_->RemoveJob(this);
}

ProxyResolverFactoryImpl::ProxyResolverFactoryImpl()
    : ProxyResolverFactoryImpl(
          net::ProxyResolverV8TracingFactory::Create()) {}

void ProxyResolverFactoryImpl::BindRequest(
    proxy_resolver::mojom::ProxyResolverFactoryRequest request,
    service_manager::ServiceContextRefFactory* ref_factory) {
  if (binding_set_.empty()) {
    DCHECK(!service_ref_);
    service_ref_ = ref_factory->CreateRef();
  }

  DCHECK(service_ref_.get());
  binding_set_.AddBinding(this, std::move(request));
}

ProxyResolverFactoryImpl::ProxyResolverFactoryImpl(
    std::unique_ptr<net::ProxyResolverV8TracingFactory> proxy_resolver_factory)
    : proxy_resolver_impl_factory_(std::move(proxy_resolver_factory)) {
  binding_set_.set_connection_error_handler(base::Bind(
      &ProxyResolverFactoryImpl::OnConnectionError, base::Unretained(this)));
}

ProxyResolverFactoryImpl::~ProxyResolverFactoryImpl() {}

void ProxyResolverFactoryImpl::CreateResolver(
    const std::string& pac_script,
    mojo::InterfaceRequest<mojom::ProxyResolver> request,
    mojom::ProxyResolverFactoryRequestClientPtr client) {
  DCHECK(service_ref_);

  // The Job will call RemoveJob on |this| when either the create request
  // finishes or |request| or |client| encounters a connection error.
  std::unique_ptr<Job> job = std::make_unique<Job>(
      this, net::PacFileData::FromUTF8(pac_script),
      proxy_resolver_impl_factory_.get(), std::move(request), std::move(client),
      service_ref_->Clone());
  Job* job_ptr = job.get();
  jobs_[job_ptr] = std::move(job);
}

void ProxyResolverFactoryImpl::RemoveJob(Job* job) {
  size_t erased_count = jobs_.erase(job);
  DCHECK_EQ(1U, erased_count);
}

void ProxyResolverFactoryImpl::OnConnectionError() {
  DCHECK(service_ref_);
  if (binding_set_.empty())
    service_ref_.reset();
}

}  // namespace proxy_resolver
