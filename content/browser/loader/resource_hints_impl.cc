// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/memory/ref_counted.h"
#include "base/trace_event/trace_event.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/public/browser/resource_hints.h"
#include "net/base/address_list.h"
#include "net/base/load_flags.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_info.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_transaction_factory.h"
#include "net/log/net_log_with_source.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace content {

namespace {

class RequestHolder {
 public:
  std::unique_ptr<net::HostResolver::Request>* GetRequest() {
    return &request_;
  }

 private:
  std::unique_ptr<net::HostResolver::Request> request_;
};

void OnResolveComplete(std::unique_ptr<RequestHolder> request_holder,
                       std::unique_ptr<net::AddressList> addresses,
                       const net::CompletionCallback& callback,
                       int result) {
  // Plumb the resolution result into the callback if future consumers want
  // that information.
  callback.Run(result);
}

}  // namespace

void PreconnectUrl(net::URLRequestContextGetter* getter,
                   const GURL& url,
                   const GURL& site_for_cookies,
                   int count,
                   bool allow_credentials) {
  DCHECK(ResourceDispatcherHostImpl::Get()
             ->io_thread_task_runner()
             ->BelongsToCurrentThread());
  DCHECK(getter);
  TRACE_EVENT2("net", "PreconnectUrl", "url", url.spec(), "count", count);

  net::URLRequestContext* request_context = getter->GetURLRequestContext();
  if (!request_context)
    return;

  net::HttpTransactionFactory* factory =
      request_context->http_transaction_factory();
  net::HttpNetworkSession* session = factory->GetSession();

  std::string user_agent;
  if (request_context->http_user_agent_settings())
    user_agent = request_context->http_user_agent_settings()->GetUserAgent();
  net::HttpRequestInfo request_info;
  request_info.url = url;
  request_info.method = "GET";
  request_info.extra_headers.SetHeader(net::HttpRequestHeaders::kUserAgent,
                                       user_agent);

  net::NetworkDelegate* delegate = request_context->network_delegate();
  if (delegate->CanEnablePrivacyMode(url, site_for_cookies))
    request_info.privacy_mode = net::PRIVACY_MODE_ENABLED;

  // TODO(yoav): Fix this layering violation, since when credentials are not
  // allowed we should turn on a flag indicating that, rather then turn on
  // private mode, even if lower layers would treat both the same.
  if (!allow_credentials) {
    request_info.privacy_mode = net::PRIVACY_MODE_ENABLED;
    request_info.load_flags = net::LOAD_DO_NOT_SEND_COOKIES |
                              net::LOAD_DO_NOT_SAVE_COOKIES |
                              net::LOAD_DO_NOT_SEND_AUTH_DATA;
  }

  net::HttpStreamFactory* http_stream_factory = session->http_stream_factory();
  http_stream_factory->PreconnectStreams(count, request_info);
}

int PreresolveUrl(net::URLRequestContextGetter* getter,
                  const GURL& url,
                  const net::CompletionCallback& callback) {
  DCHECK(ResourceDispatcherHostImpl::Get()
             ->io_thread_task_runner()
             ->BelongsToCurrentThread());
  DCHECK(getter);
  TRACE_EVENT1("net", "PreresolveUrl", "url", url.spec());

  net::URLRequestContext* request_context = getter->GetURLRequestContext();
  if (!request_context)
    return net::ERR_CONTEXT_SHUT_DOWN;

  auto request_holder = std::make_unique<RequestHolder>();
  auto addresses = std::make_unique<net::AddressList>();

  // Save raw pointers before the unique_ptr is invalidated by base::Passed.
  net::AddressList* raw_addresses = addresses.get();
  std::unique_ptr<net::HostResolver::Request>* out_request =
      request_holder->GetRequest();

  net::HostResolver* resolver = request_context->host_resolver();
  net::HostResolver::RequestInfo resolve_info(net::HostPortPair::FromURL(url));
  resolve_info.set_is_speculative(true);
  return resolver->Resolve(
      resolve_info, net::IDLE, raw_addresses,
      base::Bind(&OnResolveComplete, base::Passed(&request_holder),
                 base::Passed(&addresses), callback),
      out_request, net::NetLogWithSource());
}

}  // namespace content
