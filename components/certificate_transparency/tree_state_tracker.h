// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_TREE_STATE_TRACKER_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_TREE_STATE_TRACKER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/ref_counted.h"
#include "components/certificate_transparency/sth_observer.h"
#include "net/cert/ct_verifier.h"

namespace net {
class NetLog;
class CTLogVerifier;
class HostResolver;
class X509Certificate;

namespace ct {
struct SignedCertificateTimestamp;
struct SignedTreeHead;
}  // namespace ct

}  // namespace net

namespace certificate_transparency {
class LogDnsClient;
class SingleTreeTracker;

// This class receives notifications of new Signed Tree Heads (STHs) and
// verified Signed Certificate Timestamps (SCTs) and delegates them to
// the SingleTreeTracker tracking the CT log they relate to.
// TODO(eranm): Export the inclusion check status of SCTs+Certs so it can
// be used in the DevTools Security panel, for example. See
// https://crbug.com/506227#c22.
class TreeStateTracker : public net::CTVerifier::Observer, public STHObserver {
 public:
  // Tracks the state of the logs provided in |ct_logs|. An instance of this
  // class only tracks the logs provided in the constructor. The implementation
  // is based on the assumption that the list of recognized logs does not change
  // during the object's life time.
  // Observed STHs from logs not in this list will be simply ignored.
  TreeStateTracker(std::vector<scoped_refptr<const net::CTLogVerifier>> ct_logs,
                   net::HostResolver* host_resolver,
                   net::NetLog* net_log);
  ~TreeStateTracker() override;

  // net::ct::CTVerifier::Observer implementation.
  // Delegates to the tree tracker corresponding to the log that issued the SCT.
  void OnSCTVerified(base::StringPiece hostname,
                     net::X509Certificate* cert,
                     const net::ct::SignedCertificateTimestamp* sct) override;

  // STHObserver implementation.
  // Delegates to the tree tracker corresponding to the log that issued the STH.
  void NewSTHObserved(const net::ct::SignedTreeHead& sth) override;

 private:
  // A Log DNS client for fetching inclusion proof and leaf indices from
  // DNS front-end of CT logs.
  // Shared between all SingleTreeTrackers, for rate-limiting across all
  // trackers. Must be deleted after the tree trackers.
  std::unique_ptr<LogDnsClient> dns_client_;

  // Holds the SingleTreeTracker for each log
  std::map<std::string, std::unique_ptr<SingleTreeTracker>> tree_trackers_;

  DISALLOW_COPY_AND_ASSIGN(TreeStateTracker);
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_TREE_STATE_TRACKER_H_
