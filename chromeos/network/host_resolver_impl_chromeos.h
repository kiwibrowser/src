// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_HOST_RESOLVER_IMPL_CHROMEOS_H_
#define CHROMEOS_NETWORK_HOST_RESOLVER_IMPL_CHROMEOS_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "chromeos/chromeos_export.h"
#include "net/dns/host_resolver_impl.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace chromeos {
class NetworkStateHandler;
}

namespace chromeos {

// HostResolverImplChromeOS overrides HostResolverImpl::Resolve in order to
// provide the correct IP addresses for localhost using the chromeos
// NetworkHandler interface. ('hostname' only returns 'localhost' on cros).

class CHROMEOS_EXPORT HostResolverImplChromeOS : public net::HostResolverImpl {
 public:
  // ChromeOS specific implementation of HostResolver::CreateSystemResolver.
  // Assumes NetworkHandler has been initialized.
  // This is expected to be constructed on the same thread that Resolve() is
  // called from, i.e. the IO thread, which is presumed to differ from the
  // thread that NetworkStateHandler is called on, i.e. the UI thread.
  static std::unique_ptr<net::HostResolver> CreateSystemResolver(
      const Options& options,
      net::NetLog* net_log);

  // Creates a host resolver instance for testing.
  static std::unique_ptr<net::HostResolver> CreateHostResolverForTest(
      scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner,
      NetworkStateHandler* network_state_handler);

  ~HostResolverImplChromeOS() override;

  // HostResolverImpl
  int Resolve(const RequestInfo& info,
              net::RequestPriority priority,
              net::AddressList* addresses,
              const net::CompletionCallback& callback,
              std::unique_ptr<Request>* out_req,
              const net::NetLogWithSource& source_net_log) override;

 private:
  friend class net::HostResolver;
  class NetworkObserver;

  HostResolverImplChromeOS(
      scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner,
      NetworkStateHandler* network_state_handler,
      const Options& options,
      net::NetLog* net_log);

  void SetIPAddresses(const std::string& ipv4_address,
                      const std::string& ipv6_address);

  bool ResolveLocalIPAddress(const RequestInfo& info,
                             net::AddressList* addresses);

  std::string ipv4_address_;
  std::string ipv6_address_;
  base::ThreadChecker thread_checker_;
  scoped_refptr<base::SingleThreadTaskRunner> network_handler_task_runner_;
  base::WeakPtrFactory<HostResolverImplChromeOS> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HostResolverImplChromeOS);
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_HOST_RESOLVER_IMPL_CHROMEOS_H_
