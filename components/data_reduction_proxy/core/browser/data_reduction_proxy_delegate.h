// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_DELEGATE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_DELEGATE_H_

#include <string>

#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "net/base/network_change_notifier.h"
#include "net/base/proxy_delegate.h"
#include "net/proxy_resolution/proxy_retry_info.h"
#include "url/gurl.h"

namespace base {
class TickClock;
}

namespace net {
class NetLog;
class ProxyInfo;
class ProxyServer;
}

namespace data_reduction_proxy {

class DataReductionProxyBypassStats;
class DataReductionProxyConfig;
class DataReductionProxyConfigurator;
class DataReductionProxyEventCreator;
class DataReductionProxyIOData;

class DataReductionProxyDelegate
    : public net::ProxyDelegate,
      public net::NetworkChangeNotifier::IPAddressObserver {
 public:
  // ProxyDelegate instance is owned by io_thread. |auth_handler| and |config|
  // outlives this class instance.
  DataReductionProxyDelegate(DataReductionProxyConfig* config,
                             const DataReductionProxyConfigurator* configurator,
                             DataReductionProxyEventCreator* event_creator,
                             DataReductionProxyBypassStats* bypass_stats,
                             net::NetLog* net_log);

  ~DataReductionProxyDelegate() override;

  // Performs initialization on the IO thread.
  void InitializeOnIOThread(DataReductionProxyIOData* io_data);

  // net::ProxyDelegate implementation:
  void OnResolveProxy(const GURL& url,
                      const std::string& method,
                      const net::ProxyRetryInfoMap& proxy_retry_info,
                      net::ProxyInfo* result) override;
  void OnFallback(const net::ProxyServer& bad_proxy, int net_error) override;

  void SetTickClockForTesting(const base::TickClock* tick_clock);

 protected:
  // Protected so that it can be overridden during testing.
  // Returns true if |proxy_server| supports QUIC.
  virtual bool SupportsQUIC(const net::ProxyServer& proxy_server) const;

  // Availability status of data reduction QUIC proxy.
  // Protected so that the enum values are accessible for testing.
  enum QuicProxyStatus {
    QUIC_PROXY_STATUS_AVAILABLE,
    QUIC_PROXY_NOT_SUPPORTED,
    QUIC_PROXY_STATUS_MARKED_AS_BROKEN,
    QUIC_PROXY_DISABLED_VIA_FIELD_TRIAL,
    QUIC_PROXY_STATUS_BOUNDARY
  };

 private:
  // Records the availability status of data reduction proxy.
  void RecordQuicProxyStatus(QuicProxyStatus status) const;

  // NetworkChangeNotifier::IPAddressObserver:
  void OnIPAddressChanged() override;

  // Checks if the first proxy server in |result| supports QUIC and if so
  // adds an alternative proxy configuration to |result|.
  void GetAlternativeProxy(const GURL& url,
                           const net::ProxyRetryInfoMap& proxy_retry_info,
                           net::ProxyInfo* result) const;

  const DataReductionProxyConfig* config_;
  const DataReductionProxyConfigurator* configurator_;
  DataReductionProxyEventCreator* event_creator_;
  DataReductionProxyBypassStats* bypass_stats_;

  // Tick clock used for obtaining the current time.
  const base::TickClock* tick_clock_;

  // Set to the time when last IP address change event was received, or the time
  // of initialization of |this|, whichever is later.
  base::TimeTicks last_network_change_time_;

  DataReductionProxyIOData* io_data_;

  net::NetLog* net_log_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyDelegate);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_DELEGATE_H_
