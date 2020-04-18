// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_STALE_HOST_RESOLVER_H_
#define COMPONENTS_CRONET_STALE_HOST_RESOLVER_H_

#include <unordered_set>

#include "net/dns/host_resolver.h"
#include "net/dns/host_resolver_impl.h"

namespace cronet {

// A HostResolver that wraps a HostResolverImpl and uses it to make requests,
// but "impatiently" returns stale data (if available and usable) after a delay,
// to reduce DNS latency at the expense of accuracy.
class StaleHostResolver : public net::HostResolver {
 public:
  struct StaleOptions {
    StaleOptions();

    // How long to wait before returning stale data, if available.
    base::TimeDelta delay;

    // If positive, how long stale data can be past the expiration time before
    // it's considered unusable. If zero or negative, stale data can be used
    // indefinitely.
    base::TimeDelta max_expired_time;

    // If set, stale data from previous networks is usable; if clear, it's not.
    //
    // If the other network had a working, correct DNS setup, this can increase
    // the availability of useful stale results.
    //
    // If the other network had a broken (e.g. hijacked for captive portal) DNS
    // setup, this will instead end up returning useless results.
    bool allow_other_network;

    // If positive, the maximum number of times a stale entry can be used. If
    // zero, there is no limit.
    int max_stale_uses;
  };

  // Creates a StaleHostResolver that uses |inner_resolver| for actual
  // resolution, but potentially returns stale data according to
  // |stale_options|.
  StaleHostResolver(std::unique_ptr<net::HostResolverImpl> inner_resolver,
                    const StaleOptions& stale_options);

  ~StaleHostResolver() override;

  // HostResolver implementation:

  // Resolves as a regular HostResolver, but if stale data is available and
  // usable (according to the options passed to the constructor), and fresh data
  // is not returned before the specified delay, returns the stale data instead.
  //
  // If stale data is returned, the StaleHostResolver allows the underlying
  // request to continue in order to repopulate the cache.
  int Resolve(const RequestInfo& info,
              net::RequestPriority priority,
              net::AddressList* addresses,
              const net::CompletionCallback& callback,
              std::unique_ptr<Request>* out_req,
              const net::NetLogWithSource& net_log) override;

  // The remaining public methods pass through to the inner resolver:

  int ResolveFromCache(const RequestInfo& info,
                       net::AddressList* addresses,
                       const net::NetLogWithSource& net_log) override;
  int ResolveStaleFromCache(
      const RequestInfo& info,
      net::AddressList* addresses,
      net::HostCache::EntryStaleness* stale_info,
      const net::NetLogWithSource& source_net_log) override;
  void SetDnsClientEnabled(bool enabled) override;
  net::HostCache* GetHostCache() override;
  bool HasCached(base::StringPiece hostname,
                 net::HostCache::Entry::Source* source_out,
                 net::HostCache::EntryStaleness* stale_out) const override;
  std::unique_ptr<base::Value> GetDnsConfigAsValue() const override;

 private:
  class RequestImpl;

  // Called from |Request| when a request is complete and can be destroyed.
  void OnRequestComplete(Request* request);

  // The underlying HostResolverImpl that will be used to make cache and network
  // requests.
  std::unique_ptr<net::HostResolverImpl> inner_resolver_;

  // Options that govern when a stale response can or can't be returned.
  StaleOptions options_;

  DISALLOW_COPY_AND_ASSIGN(StaleHostResolver);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_STALE_HOST_RESOLVER_H_
