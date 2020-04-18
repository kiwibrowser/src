// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NETWORK_HINTS_BROWSER_NETWORK_HINTS_MESSAGE_FILTER_H_
#define COMPONENTS_NETWORK_HINTS_BROWSER_NETWORK_HINTS_MESSAGE_FILTER_H_

#include "base/macros.h"
#include "content/public/browser/browser_message_filter.h"

namespace net {
class HostResolver;
}

namespace network_hints {
struct LookupRequest;

// Simple browser-side handler for DNS prefetch requests.
// Passes prefetch requests to the provided net::HostResolver.
// Each renderer process requires its own filter.
class NetworkHintsMessageFilter : public content::BrowserMessageFilter {
 public:
  explicit NetworkHintsMessageFilter(net::HostResolver* host_resolver);

  // content::BrowserMessageFilter implementation:
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ~NetworkHintsMessageFilter() override;

  void OnDnsPrefetch(const LookupRequest& lookup_request);

  net::HostResolver* host_resolver_;

  DISALLOW_COPY_AND_ASSIGN(NetworkHintsMessageFilter);
};

}  // namespace network_hints

#endif  // COMPONENTS_NETWORK_HINTS_BROWSER_NETWORK_HINTS_MESSAGE_FILTER_H_
