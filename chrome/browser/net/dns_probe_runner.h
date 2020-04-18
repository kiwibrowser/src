// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_DNS_PROBE_RUNNER_H_
#define CHROME_BROWSER_NET_DNS_PROBE_RUNNER_H_

#include <memory>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace net {
class DnsClient;
class DnsResponse;
class DnsTransaction;
}

namespace chrome_browser_net {

// Runs DNS probes using a single DnsClient and evaluates the responses.
// (Currently requests A records for google.com and expects at least one IP
// address in the response.)
// Used by DnsProbeService to probe the system and public DNS configurations.
class DnsProbeRunner {
 public:
  static const char kKnownGoodHostname[];

  // Used in histograms; add new entries at the bottom, and don't remove any.
  enum Result {
    UNKNOWN,
    CORRECT,     // Response contains at least one A record.
    INCORRECT,   // Response claimed success but included no A records.
    FAILING,     // Response included an error or was malformed.
    UNREACHABLE  // No response received (timeout, network unreachable, etc.).
  };

  DnsProbeRunner();
  ~DnsProbeRunner();

  // Sets the DnsClient that will be used for DNS probes sent by this runner.
  // Must be called before RunProbe; can be called repeatedly, including during
  // a probe.  It will not affect an in-flight probe, if one is running.
  void SetClient(std::unique_ptr<net::DnsClient> client);

  // Starts a probe using the client specified with SetClient, which must have
  // been called before RunProbe.  |callback| will be called asynchronously
  // when the result is ready, even if it is ready synchronously.  Must not
  // be called again until the callback is called, but may be called during the
  // callback.
  void RunProbe(const base::Closure& callback);

  // Returns true if a probe is running.  Guaranteed to return true after
  // RunProbe returns, and false during and after the callback.
  bool IsRunning() const;

  // Returns the result of the last probe.
  Result result() const { return result_; }

 private:
  void OnTransactionComplete(net::DnsTransaction* transaction,
                             int net_error,
                             const net::DnsResponse* response);
  void CallCallback();

  std::unique_ptr<net::DnsClient> client_;

  // The callback passed to |RunProbe|.  Cleared right before calling the
  // callback.
  base::Closure callback_;

  // The transaction started in |RunProbe| for the DNS probe.  Reset once the
  // results have been examined.
  std::unique_ptr<net::DnsTransaction> transaction_;

  Result result_;

  base::WeakPtrFactory<DnsProbeRunner> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DnsProbeRunner);
};

}  // namespace chrome_browser_net

#endif  // CHROME_BROWSER_NET_DNS_PROBE_RUNNER_H_
