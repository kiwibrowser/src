// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DNS_DNS_API_H_
#define EXTENSIONS_BROWSER_API_DNS_DNS_API_H_

#include <string>

#include "extensions/browser/extension_function.h"
#include "net/base/address_list.h"
#include "net/base/completion_callback.h"
#include "net/dns/host_resolver.h"

namespace content {
class ResourceContext;
}

namespace extensions {

class DnsResolveFunction : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("dns.resolve", DNS_RESOLVE)

  DnsResolveFunction();

 protected:
  ~DnsResolveFunction() override;

  // UIThreadExtensionFunction:
  ResponseAction Run() override;

 private:
  void WorkOnIOThread();
  void RespondOnUIThread(std::unique_ptr<base::ListValue> results);

  void OnLookupFinished(int result);

  std::string hostname_;

  // Not owned.
  content::ResourceContext* resource_context_;

  bool response_;  // The value sent in SendResponse().

  std::unique_ptr<net::HostResolver::Request> request_;
  std::unique_ptr<net::AddressList> addresses_;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DNS_DNS_API_H_
