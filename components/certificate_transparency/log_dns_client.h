// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_LOG_DNS_CLIENT_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_LOG_DNS_CLIENT_H_

#include <stdint.h>

#include <list>

#include "base/callback.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_piece.h"
#include "net/base/completion_callback.h"
#include "net/base/net_errors.h"
#include "net/base/network_change_notifier.h"
#include "net/log/net_log_with_source.h"

namespace net {
class DnsClient;
namespace ct {
struct MerkleAuditProof;
}  // namespace ct
}  // namespace net

namespace certificate_transparency {

// Queries Certificate Transparency (CT) log servers via DNS.
// All queries are performed asynchronously.
// For more information, see
// https://github.com/google/certificate-transparency-rfcs/blob/master/dns/draft-ct-over-dns.md.
// It must be created and deleted on the same thread. It is not thread-safe.
class LogDnsClient : public net::NetworkChangeNotifier::DNSObserver {
 public:
  class AuditProofQuery {
   public:
    virtual ~AuditProofQuery() = default;
    virtual const net::ct::MerkleAuditProof& GetProof() const = 0;
  };

  // Creates a log client that will take ownership of |dns_client| and use it
  // to perform DNS queries. Queries will be logged to |net_log|.
  // The |dns_client| does not need to be configured first - this will be done
  // automatically as needed.
  // A limit can be set on the number of concurrent DNS queries by providing a
  // positive value for |max_concurrent_queries|. Queries that would exceed this
  // limit will fail with net::TEMPORARILY_THROTTLED. Setting this to 0 will
  // disable this limit.
  LogDnsClient(std::unique_ptr<net::DnsClient> dns_client,
               const net::NetLogWithSource& net_log,
               size_t max_concurrent_queries);
  // Must be deleted on the same thread that it was created on.
  ~LogDnsClient() override;

  // Called by NetworkChangeNotifier when the DNS config changes.
  // The DnsClient's config will be updated in response.
  void OnDNSChanged() override;

  // Called by NetworkChangeNotifier when the DNS config is first read.
  // The DnsClient's config will be updated in response.
  void OnInitialDNSConfigRead() override;

  // Registers a callback to be invoked when the number of concurrent queries
  // falls below the limit defined by |max_concurrent_queries| (passed to the
  // constructor of LogDnsClient). This callback will fire once and then be
  // unregistered. Should only be used if QueryAuditProof() returns
  // net::ERR_TEMPORARILY_THROTTLED.
  // The callback will be run on the same thread that created the LogDnsClient.
  void NotifyWhenNotThrottled(base::OnceClosure callback);

  // Queries a CT log to retrieve an audit proof for the leaf with |leaf_hash|.
  // The log is identified by |domain_for_log|, which is the DNS name used as a
  // suffix for all queries.
  // The |leaf_hash| is the SHA-256 Merkle leaf hash (see RFC6962, section 2.1).
  // The size of the CT log tree, for which the proof is requested, must be
  // provided in |tree_size|.
  // A handle to the query will be placed in |out_query|. The audit proof can be
  // obtained from that once the query completes. Deleting this handle before
  // the query completes will cancel it. It must not outlive the LogDnsClient.
  // If the proof cannot be obtained synchronously, this method will return
  // net::ERR_IO_PENDING and invoke |callback| once the query is complete.
  // The callback will be run on the same thread that created the LogDnsClient.
  // Returns:
  // - net::OK if the query was successful.
  // - net::ERR_IO_PENDING if the query was successfully started and is
  //   continuing asynchronously.
  // - net::ERR_TEMPORARILY_THROTTLED if the maximum number of concurrent
  //   queries are already in progress. Try again later.
  // - net::ERR_NAME_RESOLUTION_FAILED if DNS queries are not possible.
  //   Check that the DnsConfig returned by NetworkChangeNotifier is valid.
  // - net::ERR_INVALID_ARGUMENT if an argument is invalid, e.g. |leaf_hash| is
  //   not a SHA-256 hash.
  net::Error QueryAuditProof(base::StringPiece domain_for_log,
                             std::string leaf_hash,
                             uint64_t tree_size,
                             std::unique_ptr<AuditProofQuery>* out_query,
                             const net::CompletionCallback& callback);

 private:
  // Invoked when an audit proof query completes.
  // |callback| is the user-provided callback that should be notified.
  // |net_error| is a net::Error indicating success or failure.
  void QueryAuditProofComplete(const net::CompletionCallback& callback,
                               int net_error);

  // Invoked when an audit proof query is cancelled.
  void QueryAuditProofCancelled();

  // Returns true if the maximum number of queries are currently in-flight.
  // If the maximum number of in-flight queries is set to 0, this will always
  // return false.
  bool HasMaxQueriesInFlight() const;

  // Updates the |dns_client_| config using NetworkChangeNotifier.
  void UpdateDnsConfig();

  // Used to perform DNS queries.
  std::unique_ptr<net::DnsClient> dns_client_;
  // Passed to the DNS client for logging.
  net::NetLogWithSource net_log_;
  // The number of queries that are currently in-flight.
  size_t in_flight_queries_;
  // The maximum number of queries that can be in-flight at one time.
  size_t max_in_flight_queries_;
  // Callbacks to invoke when the number of in-flight queries is at its limit.
  std::list<base::OnceClosure> not_throttled_callbacks_;

  DISALLOW_COPY_AND_ASSIGN(LogDnsClient);
};

}  // namespace certificate_transparency
#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_LOG_DNS_CLIENT_H_
