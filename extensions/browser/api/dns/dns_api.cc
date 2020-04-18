// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/dns/dns_api.h"

#include "base/bind.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_context.h"
#include "extensions/browser/api/dns/host_resolver_wrapper.h"
#include "extensions/common/api/dns.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_with_source.h"

using content::BrowserThread;
using extensions::api::dns::ResolveCallbackResolveInfo;

namespace Resolve = extensions::api::dns::Resolve;

namespace extensions {

DnsResolveFunction::DnsResolveFunction()
    : resource_context_(), response_(false), addresses_(new net::AddressList) {}

DnsResolveFunction::~DnsResolveFunction() {}

ExtensionFunction::ResponseAction DnsResolveFunction::Run() {
  std::unique_ptr<Resolve::Params> params(Resolve::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  hostname_ = params->hostname;
  resource_context_ = browser_context()->GetResourceContext();

  bool result = BrowserThread::PostTask(
      BrowserThread::IO,
      FROM_HERE,
      base::Bind(&DnsResolveFunction::WorkOnIOThread, this));
  DCHECK(result);
  return RespondLater();
}

void DnsResolveFunction::WorkOnIOThread() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  net::HostResolver* host_resolver =
      HostResolverWrapper::GetInstance()->GetHostResolver(resource_context_);
  DCHECK(host_resolver);

  // Yes, we are passing zero as the port. There are some interesting but not
  // presently relevant reasons why HostResolver asks for the port of the
  // hostname you'd like to resolve, even though it doesn't use that value in
  // determining its answer.
  net::HostPortPair host_port_pair(hostname_, 0);

  net::HostResolver::RequestInfo request_info(host_port_pair);
  int resolve_result = host_resolver->Resolve(
      request_info, net::DEFAULT_PRIORITY, addresses_.get(),
      base::Bind(&DnsResolveFunction::OnLookupFinished, this), &request_,
      net::NetLogWithSource());

  // Balanced in OnLookupFinished.
  AddRef();

  if (resolve_result != net::ERR_IO_PENDING)
    OnLookupFinished(resolve_result);
}

void DnsResolveFunction::RespondOnUIThread(
    std::unique_ptr<base::ListValue> results) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  Respond(response_ ? ArgumentList(std::move(results))
                    : Error(kUnknownErrorDoNotUse));
}

void DnsResolveFunction::OnLookupFinished(int resolve_result) {
  auto resolve_info = std::make_unique<ResolveCallbackResolveInfo>();
  resolve_info->result_code = resolve_result;
  if (resolve_result == net::OK) {
    DCHECK(!addresses_->empty());
    resolve_info->address.reset(
        new std::string(addresses_->front().ToStringWithoutPort()));
  }
  response_ = true;

  bool post_task_result = BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&DnsResolveFunction::RespondOnUIThread, this,
                     Resolve::Results::Create(*resolve_info)));
  DCHECK(post_task_result);

  Release();  // Added in WorkOnIOThread().
}

}  // namespace extensions
