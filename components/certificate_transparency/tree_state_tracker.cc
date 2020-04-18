// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/tree_state_tracker.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "components/certificate_transparency/features.h"
#include "components/certificate_transparency/log_dns_client.h"
#include "components/certificate_transparency/single_tree_tracker.h"
#include "net/base/network_change_notifier.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_tree_head.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/dns_client.h"
#include "net/dns/dns_config_service.h"
#include "net/log/net_log.h"

using net::X509Certificate;
using net::CTLogVerifier;
using net::ct::SignedCertificateTimestamp;
using net::ct::SignedTreeHead;

namespace {
const size_t kMaxConcurrentDnsQueries = 1;
}

namespace certificate_transparency {

TreeStateTracker::TreeStateTracker(
    std::vector<scoped_refptr<const CTLogVerifier>> ct_logs,
    net::HostResolver* host_resolver,
    net::NetLog* net_log) {
  std::unique_ptr<net::DnsClient> dns_client =
      net::DnsClient::CreateClient(net_log);
  dns_client_ = std::make_unique<LogDnsClient>(
      std::move(dns_client),
      net::NetLogWithSource::Make(net_log,
                                  net::NetLogSourceType::CT_TREE_STATE_TRACKER),
      kMaxConcurrentDnsQueries);

  for (const auto& log : ct_logs) {
    tree_trackers_[log->key_id()].reset(
        new SingleTreeTracker(log, dns_client_.get(), host_resolver, net_log));
  }
}

TreeStateTracker::~TreeStateTracker() {}

void TreeStateTracker::OnSCTVerified(base::StringPiece hostname,
                                     X509Certificate* cert,
                                     const SignedCertificateTimestamp* sct) {
  auto it = tree_trackers_.find(sct->log_id);
  // Ignore if the SCT is from an unknown log.
  if (it == tree_trackers_.end())
    return;

  it->second->OnSCTVerified(hostname, cert, sct);
}

void TreeStateTracker::NewSTHObserved(const SignedTreeHead& sth) {
  auto it = tree_trackers_.find(sth.log_id);
  // Is the STH from a known log? Since STHs can be provided from external
  // sources for logs not yet recognized by this client, return, rather than
  // DCHECK.
  if (it == tree_trackers_.end())
    return;

  it->second->NewSTHObserved(sth);
}

}  // namespace certificate_transparency
