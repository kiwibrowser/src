// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_hints/browser/network_hints_message_filter.h"

#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "components/network_hints/common/network_hints_common.h"
#include "components/network_hints/common/network_hints_messages.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/address_list.h"
#include "net/base/net_errors.h"
#include "net/dns/host_resolver.h"
#include "net/log/net_log_with_source.h"
#include "url/gurl.h"

namespace network_hints {

namespace {

const int kDefaultPort = 80;

class DnsLookupRequest {
 public:
  DnsLookupRequest(net::HostResolver* host_resolver,
                   const std::string& hostname)
      : hostname_(hostname),
        resolver_(host_resolver) {
  }

  // Return underlying network resolver status.
  // net::OK ==> Host was found synchronously.
  // net:ERR_IO_PENDING ==> Network will callback later with result.
  // anything else ==> Host was not found synchronously.
  int Start() {
    net::HostResolver::RequestInfo resolve_info(
        net::HostPortPair(hostname_, kDefaultPort));

    // Make a note that this is a speculative resolve request. This allows
    // separating it from real navigations in the observer's callback, and
    // lets the HostResolver know it can be de-prioritized.
    resolve_info.set_is_speculative(true);
    return resolver_->Resolve(
        resolve_info, net::DEFAULT_PRIORITY, &addresses_,
        base::Bind(&DnsLookupRequest::OnLookupFinished, base::Owned(this)),
        &request_, net::NetLogWithSource());
  }

 private:
  void OnLookupFinished(int result) {
    VLOG(2) << __FUNCTION__ << ": " << hostname_ << ", result=" << result;
  }

  const std::string hostname_;
  net::HostResolver* resolver_;
  std::unique_ptr<net::HostResolver::Request> request_;
  net::AddressList addresses_;

  DISALLOW_COPY_AND_ASSIGN(DnsLookupRequest);
};

}  // namespace

NetworkHintsMessageFilter::NetworkHintsMessageFilter(
    net::HostResolver* host_resolver)
    : content::BrowserMessageFilter(NetworkHintsMsgStart),
      host_resolver_(host_resolver) {
  DCHECK(host_resolver_);
}

NetworkHintsMessageFilter::~NetworkHintsMessageFilter() {
}

bool NetworkHintsMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NetworkHintsMessageFilter, message)
    IPC_MESSAGE_HANDLER(NetworkHintsMsg_DNSPrefetch, OnDnsPrefetch)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void NetworkHintsMessageFilter::OnDnsPrefetch(
    const LookupRequest& lookup_request) {
  DCHECK(host_resolver_);
  for (const std::string& hostname : lookup_request.hostname_list) {
    DnsLookupRequest* request = new DnsLookupRequest(host_resolver_, hostname);
    // Note: DnsLookupRequest will be freed by the base::Owned call when
    // resolving has completed.
    request->Start();
  }
}

}  // namespace network_hints
