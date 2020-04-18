// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CERTIFICATE_TRANSPARENCY_SINGLE_TREE_TRACKER_H_
#define COMPONENTS_CERTIFICATE_TRANSPARENCY_SINGLE_TREE_TRACKER_H_

#include <map>
#include <memory>
#include <string>

#include "base/containers/mru_cache.h"
#include "base/memory/memory_pressure_monitor.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/certificate_transparency/sth_observer.h"
#include "net/base/hash_value.h"
#include "net/base/network_change_notifier.h"
#include "net/cert/ct_verifier.h"
#include "net/cert/signed_tree_head.h"
#include "net/log/net_log_with_source.h"

namespace net {

class CTLogVerifier;
class HostResolver;
class X509Certificate;

namespace ct {

struct MerkleAuditProof;
struct SignedCertificateTimestamp;

}  // namespace ct

}  // namespace net

namespace certificate_transparency {

class LogDnsClient;

// Enum indicating whether an SCT can be checked for inclusion and if not,
// the reason it cannot.
//
// Note: The numeric values are used within a histogram and should not change
// or be re-assigned.
enum SCTCanBeCheckedForInclusion {
  // If the SingleTreeTracker does not have a valid STH, then a valid STH is
  // first required to evaluate whether the SCT can be checked for inclusion
  // or not.
  VALID_STH_REQUIRED = 0,

  // If the STH does not cover the SCT (the timestamp in the SCT is greater than
  // MMD + timestamp in the STH), then a newer STH is needed.
  NEWER_STH_REQUIRED = 1,

  // When an SCT is observed, if the SingleTreeTracker instance has a valid STH
  // and the STH covers the SCT (the timestamp in the SCT is less than MMD +
  // timestamp in the STH), then it can be checked for inclusion.
  CAN_BE_CHECKED = 2,

  // This SCT was not audited because the queue of pending entries was
  // full.
  NOT_AUDITED_QUEUE_FULL = 3,

  // This SCT was not audited because no DNS lookup was done when first
  // visiting the website that supplied it. It could compromise the user's
  // privacy to do an inclusion check over DNS in this scenario.
  NOT_AUDITED_NO_DNS_LOOKUP = 4,

  // This SCT was not audited, because it was recently checked and the cached
  // inclusion result can be used.
  NOT_AUDITED_ALREADY_CHECKED = 5,

  // This SCT is already pending an audit check, and thus can be
  // de-duplicated.
  NOT_AUDITED_ALREADY_PENDING_CHECK = 6,

  // This SCT was not audited, as an Entry Leaf Hash could not be calculated.
  NOT_AUDITED_INVALID_LEAF_HASH = 7,

  SCT_CAN_BE_CHECKED_MAX
};

// Tracks the state of an individual Certificate Transparency Log's Merkle Tree.
// A CT Log constantly issues Signed Tree Heads, for which every older STH must
// be incorporated into the current/newer STH. As new certificates are logged,
// new SCTs are produced, and eventually, those SCTs are incorporated into the
// log and a new STH is produced, with there being an inclusion proof between
// the SCTs and the new STH, and a consistency proof between the old STH and the
// new STH.
// This class receives STHs provided by/observed by the embedder, with the
// assumption that STHs have been checked for consistency already. As SCTs are
// observed, their status is checked against the latest STH to ensure they were
// properly logged. If an SCT is newer than the latest STH, then this class
// verifies that when an STH is observed that should have incorporated those
// SCTs, the SCTs (and their corresponding entries) are present in the log.
//
// To accomplish this, this class needs to be notified of when new SCTs are
// observed (which it does by implementing net::CTVerifier::Observer) and when
// new STHs are observed (which it does by implementing STHObserver).
// Once connected to sources providing that data, the status for a given SCT
// can be queried by calling GetLogEntryInclusionCheck.
class SingleTreeTracker : public net::CTVerifier::Observer, public STHObserver {
 public:
  enum SCTInclusionStatus {
    // SCT was not observed by this class and is not currently pending
    // inclusion check. As there's no evidence the SCT this status relates
    // to is verified (it was never observed via OnSCTVerified), nothing
    // is done with it.
    SCT_NOT_OBSERVED,

    // SCT was observed but the STH known to this class is not old
    // enough to check for inclusion, so a newer STH is needed first.
    SCT_PENDING_NEWER_STH,

    // SCT is known and there's a new-enough STH to check inclusion against.
    // It's in the process of being checked for inclusion.
    SCT_PENDING_INCLUSION_CHECK,

    // Inclusion check succeeded.
    SCT_INCLUDED_IN_LOG,
  };

  // Tracks new STHs and SCTs received that were issued by |ct_log|.
  // The |dns_client| will be used to obtain inclusion proofs for these SCTs,
  // where possible. It is not optional.
  // The |host_resolver| will be used to ensure that DNS requests performed to
  // obtain inclusion proofs do not compromise the user's privacy. It is not
  // optional. It is assumed that it caches DNS lookups.
  SingleTreeTracker(scoped_refptr<const net::CTLogVerifier> ct_log,
                    LogDnsClient* dns_client,
                    net::HostResolver* host_resolver,
                    net::NetLog* net_log);
  ~SingleTreeTracker() override;

  // net::ct::CTVerifier::Observer implementation.
  // TODO(eranm): Extract CTVerifier::Observer to SCTObserver
  // Enqueues |sct| for later inclusion checking of the given |cert|, so long as
  // both of the following are true:
  // a) The latest STH known for this log is older than |sct.timestamp| +
  //    Maximum Merge Delay.
  // b) The |hostname| for which this certificate was issued has previously been
  //    resolved to an IP address using a DNS lookup, and the network has not
  //    changed since. This ensures that performing an inclusion check over DNS
  //    will not leak information to the DNS resolver.
  // Should only be called with SCTs issued by the log this instance tracks.
  // Hostname may be an IP literal, but a DNS lookup will not have been
  // performed in this case so inclusion checking will not be performed.
  // TODO(eranm): Make sure not to perform any synchronous, blocking operation
  // here as this callback is invoked during certificate validation.
  void OnSCTVerified(base::StringPiece hostname,
                     net::X509Certificate* cert,
                     const net::ct::SignedCertificateTimestamp* sct) override;

  // STHObserver implementation.
  // After verification of the signature over the |sth|, uses this
  // STH for future inclusion checks.
  // Must only be called for STHs issued by the log this instance tracks.
  void NewSTHObserved(const net::ct::SignedTreeHead& sth) override;

  // Returns the status of a given log entry that is assembled from
  // |cert| and |sct|. If |cert| and |sct| were not previously observed,
  // |sct| is not an SCT for |cert| or |sct| is not for this log,
  // SCT_NOT_OBSERVED will be returned.
  SCTInclusionStatus GetLogEntryInclusionStatus(
      net::X509Certificate* cert,
      const net::ct::SignedCertificateTimestamp* sct);

 private:
  struct EntryToAudit;
  struct EntryAuditState;
  struct EntryAuditResult {};
  class NetworkObserver;
  friend class NetworkObserver;

  // Less-than comparator that sorts EntryToAudits based on the SCT timestamp,
  // with smaller (older) SCTs appearing less than larger (newer) SCTs.
  struct OrderByTimestamp {
    bool operator()(const EntryToAudit& lhs, const EntryToAudit& rhs) const;
  };

  // Requests an inclusion proof for each of the entries in |pending_entries_|
  // until throttled by the LogDnsClient.
  void ProcessPendingEntries();

  // Returns the inclusion status of the given |entry|, similar to
  // GetLogEntryInclusionStatus(). The |entry| is an internal representation of
  // a certificate + SCT combination.
  SCTInclusionStatus GetAuditedEntryInclusionStatus(const EntryToAudit& entry);

  // Processes the result of obtaining an audit proof for |entry|.
  // * If an audit proof was successfully obtained and validated,
  //   updates |checked_entries_| so that future calls to
  //   GetLogEntryInclusionStatus() will indicate the entry's
  //   inclusion.
  // * If there was a failure to obtain or validate an inclusion
  //   proof, removes |entry| from the queue of entries to validate.
  //   Future calls to GetLogEntryInclusionStatus() will indicate the entry
  //   has not been observed.
  void OnAuditProofObtained(const EntryToAudit& entry, int net_error);

  // Discards all entries pending inclusion check on network change.
  // That is done to prevent the client looking up inclusion proofs for
  // certificates received from one network, on another network, thus
  // leaking state between networks.
  void ResetPendingQueue();

  // Clears entries to reduce memory overhead.
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  void LogAuditResultToNetLog(const EntryToAudit& entry, bool success);

  // Returns true if |hostname| has previously been looked up using DNS, and the
  // network has not changed since.
  bool WasLookedUpOverDNS(base::StringPiece hostname) const;

  // Holds the latest STH fetched and verified for this log.
  net::ct::SignedTreeHead verified_sth_;

  // The log being tracked.
  scoped_refptr<const net::CTLogVerifier> ct_log_;

  // Log entries waiting to be checked for inclusion, or being checked for
  // inclusion, and their state.
  std::map<EntryToAudit, EntryAuditState, OrderByTimestamp> pending_entries_;

  // A cache of leaf hashes identifying entries which were checked for
  // inclusion (the key is the Leaf Hash of the log entry).
  // NOTE: The current implementation does not cache failures, so the presence
  // of an entry in |checked_entries_| indicates success.
  // To extend support for caching failures, a success indicator should be
  // added to the EntryAuditResult struct.
  base::MRUCache<net::SHA256HashValue, EntryAuditResult> checked_entries_;

  LogDnsClient* dns_client_;

  net::HostResolver* host_resolver_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  net::NetLogWithSource net_log_;

  std::unique_ptr<NetworkObserver> network_observer_;

  base::WeakPtrFactory<SingleTreeTracker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SingleTreeTracker);
};

}  // namespace certificate_transparency

#endif  // COMPONENTS_CERTIFICATE_TRANSPARENCY_SINGLE_TREE_TRACKER_H_
