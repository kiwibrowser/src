// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_STATS_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_STATS_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "net/base/host_port_pair.h"
#include "net/base/network_change_notifier.h"
#include "net/url_request/url_request.h"

namespace net {
class HttpResponseHeaders;
class ProxyConfig;
class ProxyServer;
}

namespace data_reduction_proxy {

class DataReductionProxyConfig;

class DataReductionProxyBypassStats
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  typedef base::Callback<void(bool /* unreachable */)> UnreachableCallback;

  // Records a data reduction proxy bypass event as a "BlockType" if
  // |bypass_all| is true and as a "BypassType" otherwise. Records the event as
  // "Primary" if |is_primary| is true and "Fallback" otherwise.
  static void RecordDataReductionProxyBypassInfo(
      bool is_primary,
      bool bypass_all,
      const net::ProxyServer& proxy_server,
      DataReductionProxyBypassType bypass_type);

  // For the given response |headers| that are expected to include the data
  // reduction proxy via header, records response code UMA if the data reduction
  // proxy via header is not present.
  static void DetectAndRecordMissingViaHeaderResponseCode(
      bool is_primary,
      const net::HttpResponseHeaders& headers);

  // |config| outlives this class instance. |unreachable_callback| provides a
  // hook to inform the user that the Data Reduction Proxy is unreachable.
  // |config| must not be null.
  DataReductionProxyBypassStats(
      DataReductionProxyConfig* config,
      UnreachableCallback unreachable_callback);

  ~DataReductionProxyBypassStats() override;

  // Performs initialization on the IO thread.
  void InitializeOnIOThread();

  // Callback intended to be called from |DataReductionProxyNetworkDelegate|
  // when a request completes. This method is used to gather bypass stats.
  void OnUrlRequestCompleted(const net::URLRequest* request,
                             bool started,
                             int net_error);

  // Records the last bypass reason to |bypass_type_| and sets
  // |triggering_request_| to true. A triggering request is the first request to
  // cause the current bypass.
  void SetBypassType(DataReductionProxyBypassType type);

  // Visible for testing.
  DataReductionProxyBypassType GetBypassType() const;

  // Given |data_reduction_proxy_enabled|, a |request|, and the
  // |data_reduction_proxy_config| records the number of bypassed bytes for that
  // |request| into UMAs based on bypass type. |data_reduction_proxy_enabled|
  // tells us the state of the Data Reduction Proxy enabling preference.
  void RecordBypassedBytesHistograms(
      const net::URLRequest& request,
      bool data_reduction_proxy_enabled,
      const net::ProxyConfig& data_reduction_proxy_config);

  // Called by |ChromeNetworkDelegate| when a proxy is put into the bad proxy
  // list. Used to track when the data reduction proxy falls back.
  void OnProxyFallback(const net::ProxyServer& bypassed_proxy,
                       int net_error);

  // Unconditionally clears counts of successful requests and net errors when
  // using the Data Reduction Proxy.
  void ClearRequestCounts();

  // Checks if the availability status of the Data Reduction Proxy has changed,
  // and calls |unreachable_callback_| if so.
  void NotifyUnavailabilityIfChanged();

 private:
  friend class DataReductionProxyBypassStatsTest;

  enum BypassedBytesType {
    NOT_BYPASSED = 0,         /* Not bypassed. */
    SSL,                      /* Bypass due to SSL. */
    LOCAL_BYPASS_RULES,       /* Bypass due to client-side bypass rules. */
    PROXY_OVERRIDDEN,         /* Bypass due to a proxy taking precedence. */
    AUDIO_VIDEO,              /* Audio/Video bypass. */
    APPLICATION_OCTET_STREAM, /* "application/octet-stream" content bypass. */
    TRIGGERING_REQUEST,       /* Triggering request bypass. */
    NETWORK_ERROR,            /* Network error. */
    BYPASSED_BYTES_TYPE_MAX   /* This must always be last.*/
  };

  // NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  void RecordBypassedBytes(DataReductionProxyBypassType bypass_type,
                           BypassedBytesType bypassed_bytes_type,
                           int64_t content_length);

  DataReductionProxyConfig* data_reduction_proxy_config_;

  UnreachableCallback unreachable_callback_;

  // The last reason for bypass as determined by
  // MaybeBypassProxyAndPrepareToRetry
  DataReductionProxyBypassType last_bypass_type_;
  // True if the last request triggered the current bypass.
  bool triggering_request_;

  // The following 2 fields are used to determine if data reduction proxy is
  // unreachable. We keep a count of requests which should go through
  // data request proxy, as well as those which actually do. The proxy is
  // unreachable if no successful requests are made through it despite a
  // non-zero number of requests being eligible.

  // Count of successful requests through the data reduction proxy.
  unsigned long successful_requests_through_proxy_count_;

  // Count of network errors encountered when connecting to a data reduction
  // proxy.
  unsigned long proxy_net_errors_count_;

  // Whether or not the data reduction proxy is unavailable.
  bool unavailable_;

  base::ThreadChecker thread_checker_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyBypassStats);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_DATA_REDUCTION_PROXY_BYPASS_STATS_H_
