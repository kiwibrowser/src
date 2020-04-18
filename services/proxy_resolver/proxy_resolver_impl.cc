// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/proxy_resolver/proxy_resolver_impl.h"

#include <utility>

#include "base/macros.h"
#include "net/base/net_errors.h"
#include "net/proxy_resolution/mojo_proxy_resolver_v8_tracing_bindings.h"
#include "net/proxy_resolution/pac_file_data.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/proxy_resolution/proxy_resolver_v8_tracing.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace proxy_resolver {

class ProxyResolverImpl::Job {
 public:
  Job(mojom::ProxyResolverRequestClientPtr client,
      ProxyResolverImpl* resolver,
      const GURL& url);
  ~Job();

  void Start();

 private:
  // Mojo error handler. This is invoked in response to the client
  // disconnecting, indicating cancellation.
  void OnConnectionError();

  void GetProxyDone(int error);

  ProxyResolverImpl* resolver_;

  mojom::ProxyResolverRequestClientPtr client_;
  net::ProxyInfo result_;
  GURL url_;
  std::unique_ptr<net::ProxyResolver::Request> request_;
  bool done_;

  DISALLOW_COPY_AND_ASSIGN(Job);
};

ProxyResolverImpl::ProxyResolverImpl(
    std::unique_ptr<net::ProxyResolverV8Tracing> resolver,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : resolver_(std::move(resolver)), service_ref_(std::move(service_ref)) {}

ProxyResolverImpl::~ProxyResolverImpl() {}

void ProxyResolverImpl::GetProxyForUrl(
    const GURL& url,
    mojom::ProxyResolverRequestClientPtr client) {
  DVLOG(1) << "GetProxyForUrl(" << url << ")";
  std::unique_ptr<Job> job =
      std::make_unique<Job>(std::move(client), this, url);
  Job* job_ptr = job.get();
  resolve_jobs_[job_ptr] = std::move(job);
  job_ptr->Start();
}

void ProxyResolverImpl::DeleteJob(Job* job) {
  size_t erased_count = resolve_jobs_.erase(job);
  DCHECK_EQ(1U, erased_count);
}

ProxyResolverImpl::Job::Job(mojom::ProxyResolverRequestClientPtr client,
                            ProxyResolverImpl* resolver,
                            const GURL& url)
    : resolver_(resolver),
      client_(std::move(client)),
      url_(url),
      done_(false) {}

ProxyResolverImpl::Job::~Job() {}

void ProxyResolverImpl::Job::Start() {
  resolver_->resolver_->GetProxyForURL(
      url_, &result_, base::Bind(&Job::GetProxyDone, base::Unretained(this)),
      &request_,
      std::make_unique<net::MojoProxyResolverV8TracingBindings<
          mojom::ProxyResolverRequestClient>>(client_.get()));
  client_.set_connection_error_handler(base::Bind(
      &ProxyResolverImpl::Job::OnConnectionError, base::Unretained(this)));
}

void ProxyResolverImpl::Job::GetProxyDone(int error) {
  done_ = true;
  DVLOG(1) << "GetProxyForUrl(" << url_ << ") finished with error " << error
           << ". " << result_.proxy_list().size() << " Proxies returned:";
  for (const auto& proxy : result_.proxy_list().GetAll()) {
    DVLOG(1) << proxy.ToURI();
  }
  if (error == net::OK)
    client_->ReportResult(error, result_);
  else
    client_->ReportResult(error, net::ProxyInfo());

  resolver_->DeleteJob(this);
}

void ProxyResolverImpl::Job::OnConnectionError() {
  resolver_->DeleteJob(this);
}

}  // namespace proxy_resolver
