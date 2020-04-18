// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_DNS_PROBE_SERVICE_H_
#define CHROME_BROWSER_NET_DNS_PROBE_SERVICE_H_

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "chrome/browser/net/dns_probe_runner.h"
#include "components/error_page/common/net_error_info.h"
#include "net/base/network_change_notifier.h"

namespace net {
class DnsClient;
}

namespace chrome_browser_net {

// Probes the system and public DNS servers to determine the (probable) cause
// of a recent DNS-related page load error.  Coalesces multiple probe requests
// (perhaps from multiple tabs) and caches the results.
//
// Uses a single DNS attempt per config, and doesn't randomize source ports.
class DnsProbeService : public net::NetworkChangeNotifier::DNSObserver {
 public:
  typedef base::Callback<void(error_page::DnsProbeStatus result)>
      ProbeCallback;

  DnsProbeService();
  ~DnsProbeService() override;

  virtual void ProbeDns(const ProbeCallback& callback);

  // NetworkChangeNotifier::DNSObserver implementation:
  void OnDNSChanged() override;
  void OnInitialDNSConfigRead() override;

  void SetSystemClientForTesting(std::unique_ptr<net::DnsClient> system_client);
  void SetPublicClientForTesting(std::unique_ptr<net::DnsClient> public_client);
  void ClearCachedResultForTesting();

 private:
  enum State {
    STATE_NO_RESULT,
    STATE_PROBE_RUNNING,
    STATE_RESULT_CACHED,
  };

  void SetSystemClientToCurrentConfig();
  void SetPublicClientToGooglePublicDns();

  // Starts a probe (runs system and public probes).
  void StartProbes();
  void OnProbeComplete();
  // Calls all |pending_callbacks_| with the |cached_result_|.
  void CallCallbacks();
  // Clears a cached probe result.
  void ClearCachedResult();

  bool CachedResultIsExpired() const;

  State state_;
  std::vector<ProbeCallback> pending_callbacks_;
  base::Time probe_start_time_;
  error_page::DnsProbeStatus cached_result_;

  // DnsProbeRunners for the system DNS configuration and a public DNS server.
  DnsProbeRunner system_runner_;
  DnsProbeRunner public_runner_;

  DISALLOW_COPY_AND_ASSIGN(DnsProbeService);
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_DNS_PROBE_SERVICE_H_
