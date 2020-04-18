// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/single_tree_tracker.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/certificate_transparency/log_dns_client.h"
#include "crypto/sha2.h"
#include "net/base/hash_value.h"
#include "net/base/net_errors.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/merkle_audit_proof.h"
#include "net/cert/merkle_tree_leaf.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/host_resolver.h"
#include "net/log/net_log.h"

using net::SHA256HashValue;
using net::ct::MerkleAuditProof;
using net::ct::MerkleTreeLeaf;
using net::ct::SignedCertificateTimestamp;
using net::ct::SignedTreeHead;

// Overview of the process for auditing CT log entries
//
// In this file, obsered CT log entries are audited for inclusion in the CT log.
// A pre-requirement for auditing a log entry is having a Signed Tree Head (STH)
// from that log that is 24 hours (MMD period) after the timestamp in the SCT.
// Log entries observed while the client has no STH from that log or an STH that
// is too old start in the PENDING_NEWER_STH state.
//
// Once a fresh-enough STH is obtained, all entries that can be audited using
// this STH move to the PENDING_INCLUSION_PROOF_REQUEST state.
//
// Requests for the entry index and inclusion proof are obtained using a
// LogDnsClient instance - when an inclusion proof for an entry has been
// successfully requested (e.g. it has not been throttled), it moves to the
// INCLUSION_PROOF_REQUESTED state.
//
// Once the inclusion check is done, the entry is removed from
// |pending_entries_|. If the inclusion check has been successful, the entry
// is added to |checked_entries_|.

namespace certificate_transparency {

namespace {

// Measure how often clients encounter very new SCTs, by measuring whether an
// SCT can be checked for inclusion upon first observation.
void LogCanBeCheckedForInclusionToUMA(
    SCTCanBeCheckedForInclusion can_be_checked) {
  UMA_HISTOGRAM_ENUMERATION("Net.CertificateTransparency.CanInclusionCheckSCT",
                            can_be_checked, SCT_CAN_BE_CHECKED_MAX);
}

// Enum indicating the outcome of an inclusion check for a particular log
// entry.
//
// Note: The numeric values are used within a histogram and should not change
// or be re-assigned.
enum LogEntryInclusionCheckResult {
  // Inclusion check succeeded: Proof obtained and validated successfully.
  GOT_VALID_INCLUSION_PROOF = 0,

  // Could not get an inclusion proof.
  FAILED_GETTING_INCLUSION_PROOF = 1,

  // An inclusion proof was obtained but it is invalid.
  GOT_INVALID_INCLUSION_PROOF = 2,

  // The SCT could not be audited because the client's DNS configuration
  // is faulty.
  DNS_QUERY_NOT_POSSIBLE = 3,

  LOG_ENTRY_INCLUSION_CHECK_RESULT_MAX
};

void LogInclusionCheckResult(LogEntryInclusionCheckResult result) {
  UMA_HISTOGRAM_ENUMERATION("Net.CertificateTransparency.InclusionCheckResult",
                            result, LOG_ENTRY_INCLUSION_CHECK_RESULT_MAX);
}

// Calculate the leaf hash of the the entry in the log represented by
// the given |cert| and |sct|. If leaf hash calculation succeeds returns
// true, false otherwise.
bool GetLogEntryLeafHash(const net::X509Certificate* cert,
                         const SignedCertificateTimestamp* sct,
                         SHA256HashValue* leaf_hash) {
  MerkleTreeLeaf leaf;
  if (!GetMerkleTreeLeaf(cert, sct, &leaf))
    return false;

  std::string leaf_hash_str;
  if (!HashMerkleTreeLeaf(leaf, &leaf_hash_str))
    return false;

  memcpy(leaf_hash->data, leaf_hash_str.data(), crypto::kSHA256Length);
  return true;
}

// Audit state of a log entry.
enum AuditState {
  // Entry cannot be audited because a newer STH is needed.
  PENDING_NEWER_STH,
  // A leaf index has been obtained and the entry is now pending request
  // of an inclusion proof.
  PENDING_INCLUSION_PROOF_REQUEST,
  // An inclusion proof for this entry has been requested from the log.
  INCLUSION_PROOF_REQUESTED
};

// Maximal size of the checked entries cache.
size_t kCheckedEntriesCacheSize = 100;

// Maximal size of the pending entries queue.
size_t kPendingEntriesQueueSize = 100;

// Maximum Merge Delay - logs can have individual MMD, but all known logs
// currently have 24 hours MMD and Chrome's CT policy requires an MMD
// that's no greater than that. For simplicity, use 24 hours for all logs.
constexpr base::TimeDelta kMaximumMergeDelay = base::TimeDelta::FromHours(24);

// The log MUST incorporate the a certificate in the tree within the Maximum
// Merge Delay, so an entry can be audited once the timestamp from the SCT +
// MMD has passed.
// Returns true if the timestamp from the STH is newer than SCT timestamp + MMD.
bool IsSCTReadyForAudit(base::Time sth_timestamp, base::Time sct_timestamp) {
  return sct_timestamp + kMaximumMergeDelay < sth_timestamp;
}

std::unique_ptr<base::Value> NetLogEntryAuditingEventCallback(
    const SHA256HashValue* log_entry,
    base::StringPiece log_id,
    bool success,
    net::NetLogCaptureMode capture_mode) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());

  dict->SetString("log_entry",
                  base::HexEncode(log_entry->data, crypto::kSHA256Length));
  dict->SetString("log_id", base::HexEncode(log_id.data(), log_id.size()));
  dict->SetBoolean("success", success);

  return std::move(dict);
}

}  // namespace

// The entry that is being audited.
struct SingleTreeTracker::EntryToAudit {
  base::Time sct_timestamp;
  SHA256HashValue leaf_hash;

  explicit EntryToAudit(base::Time timestamp) : sct_timestamp(timestamp) {}
};

// State of a log entry: its audit state and information necessary to
// validate an inclusion proof. Gets updated as the entry transitions
// between the different audit states.
struct SingleTreeTracker::EntryAuditState {
  // Current phase of inclusion check.
  AuditState state;

  // The audit proof query performed by LogDnsClient.
  // It is null unless a query has been started.
  std::unique_ptr<LogDnsClient::AuditProofQuery> audit_proof_query;

  // The root hash of the tree for which an inclusion proof was requested.
  // The root hash is needed after the inclusion proof is fetched for validating
  // the inclusion proof (each inclusion proof is valid for one particular leaf,
  // denoted by the leaf index, in exactly one particular tree, denoted by the
  // tree size in the proof).
  // To avoid having to re-fetch the inclusion proof if a newer STH is provided
  // to the SingleTreeTracker, the size of the original tree for which the
  // inclusion proof was requested is stored in |proof| and the root hash
  // in |root_hash|.
  std::string root_hash;

  explicit EntryAuditState(AuditState state) : state(state) {}
};

class SingleTreeTracker::NetworkObserver
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  explicit NetworkObserver(SingleTreeTracker* parent) : parent_(parent) {
    net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  }

  ~NetworkObserver() override {
    net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
  }

  // net::NetworkChangeNotifier::NetworkChangeObserver implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override {
    parent_->ResetPendingQueue();
  }

 private:
  SingleTreeTracker* parent_;
};

// Orders entries by the SCT timestamp. In case of tie, which is very unlikely
// as it requires two SCTs issued from a log at exactly the same time, order
// by leaf hash.
bool SingleTreeTracker::OrderByTimestamp::operator()(
    const EntryToAudit& lhs,
    const EntryToAudit& rhs) const {
  return std::tie(lhs.sct_timestamp, lhs.leaf_hash) <
         std::tie(rhs.sct_timestamp, rhs.leaf_hash);
}

SingleTreeTracker::SingleTreeTracker(
    scoped_refptr<const net::CTLogVerifier> ct_log,
    LogDnsClient* dns_client,
    net::HostResolver* host_resolver,
    net::NetLog* net_log)
    : ct_log_(std::move(ct_log)),
      checked_entries_(kCheckedEntriesCacheSize),
      dns_client_(dns_client),
      host_resolver_(host_resolver),
      net_log_(net::NetLogWithSource::Make(
          net_log,
          net::NetLogSourceType::CT_TREE_STATE_TRACKER)),
      network_observer_(new NetworkObserver(this)),
      weak_factory_(this) {
  memory_pressure_listener_.reset(new base::MemoryPressureListener(base::Bind(
      &SingleTreeTracker::OnMemoryPressure, base::Unretained(this))));
}

SingleTreeTracker::~SingleTreeTracker() {
  ResetPendingQueue();
}

void SingleTreeTracker::OnSCTVerified(base::StringPiece hostname,
                                      net::X509Certificate* cert,
                                      const SignedCertificateTimestamp* sct) {
  DCHECK_EQ(ct_log_->key_id(), sct->log_id);

  // Check that a DNS lookup for hostname has already occurred (i.e. the DNS
  // resolver already knows that the user has been accessing that host). If not,
  // the DNS resolver may not know that the user has been accessing that host,
  // but performing an inclusion check would reveal that information so abort to
  // preserve the user's privacy.
  //
  // It's ok to do this now, even though the inclusion check may not happen for
  // some time, because SingleTreeTracker will discard the SCT if the network
  // changes.
  if (!WasLookedUpOverDNS(hostname)) {
    LogCanBeCheckedForInclusionToUMA(NOT_AUDITED_NO_DNS_LOOKUP);
    return;
  }

  EntryToAudit entry(sct->timestamp);
  if (!GetLogEntryLeafHash(cert, sct, &entry.leaf_hash)) {
    LogCanBeCheckedForInclusionToUMA(NOT_AUDITED_INVALID_LEAF_HASH);
    return;
  }

  // Avoid queueing multiple instances of the same entry.
  switch (GetAuditedEntryInclusionStatus(entry)) {
    case SCT_NOT_OBSERVED:
      // No need to record UMA, will be done below.
      break;
    case SCT_INCLUDED_IN_LOG:
      LogCanBeCheckedForInclusionToUMA(NOT_AUDITED_ALREADY_CHECKED);
      return;
    default:
      // Already pending, either due to a newer STH or in the queue.
      LogCanBeCheckedForInclusionToUMA(NOT_AUDITED_ALREADY_PENDING_CHECK);
      return;
  }

  if (pending_entries_.size() >= kPendingEntriesQueueSize) {
    // Queue is full - cannot audit SCT.
    LogCanBeCheckedForInclusionToUMA(NOT_AUDITED_QUEUE_FULL);
    return;
  }

  // If there isn't a valid STH or the STH is not fresh enough to check
  // inclusion against, store the SCT for future checking and return.
  if (verified_sth_.timestamp.is_null() ||
      !IsSCTReadyForAudit(verified_sth_.timestamp, entry.sct_timestamp)) {
    pending_entries_.insert(
        std::make_pair(std::move(entry), EntryAuditState(PENDING_NEWER_STH)));

    if (verified_sth_.timestamp.is_null()) {
      LogCanBeCheckedForInclusionToUMA(VALID_STH_REQUIRED);
    } else {
      LogCanBeCheckedForInclusionToUMA(NEWER_STH_REQUIRED);
    }

    return;
  }

  LogCanBeCheckedForInclusionToUMA(CAN_BE_CHECKED);
  pending_entries_.insert(std::make_pair(
      std::move(entry), EntryAuditState(PENDING_INCLUSION_PROOF_REQUEST)));

  ProcessPendingEntries();
}

void SingleTreeTracker::NewSTHObserved(const SignedTreeHead& sth) {
  DCHECK_EQ(ct_log_->key_id(), sth.log_id);

  if (!ct_log_->VerifySignedTreeHead(sth)) {
    // Sanity check the STH; the caller should have done this
    // already, but being paranoid here.
    // NOTE(eranm): Right now there's no way to get rid of this check here
    // as this is the first object in the chain that has an instance of
    // a CTLogVerifier to verify the STH.
    return;
  }

  // In order to avoid updating |verified_sth_| to an older STH in case
  // an older STH is observed, check that either the observed STH is for
  // a larger tree size or that it is for the same tree size but has
  // a newer timestamp.
  const bool sths_for_same_tree = verified_sth_.tree_size == sth.tree_size;
  const bool received_sth_is_for_larger_tree =
      (verified_sth_.tree_size < sth.tree_size);
  const bool received_sth_is_newer = (sth.timestamp > verified_sth_.timestamp);

  if (!verified_sth_.timestamp.is_null() && !received_sth_is_for_larger_tree &&
      !(sths_for_same_tree && received_sth_is_newer)) {
    // Observed an old STH - do nothing.
    return;
  }

  verified_sth_ = sth;

  // Find the first entry in the PENDING_NEWER_STH state - entries
  // before that should be pending leaf index / inclusion proof, no
  // reason to inspect them.
  auto auditable_entries_begin = std::find_if(
      pending_entries_.begin(), pending_entries_.end(),
      [](std::pair<const EntryToAudit&, const EntryAuditState&> value) {
        return value.second.state == PENDING_NEWER_STH;
      });

  // Find where to stop - this is the first entry whose timestamp + MMD
  // is greater than the STH's timestamp.
  auto auditable_entries_end = std::lower_bound(
      auditable_entries_begin, pending_entries_.end(), sth.timestamp,
      [](std::pair<const EntryToAudit&, const EntryAuditState&> value,
         base::Time sth_timestamp) {
        return IsSCTReadyForAudit(sth_timestamp, value.first.sct_timestamp);
      });

  // Update the state of all entries that can now be checked for inclusion.
  for (auto curr_entry = auditable_entries_begin;
       curr_entry != auditable_entries_end; ++curr_entry) {
    DCHECK_EQ(curr_entry->second.state, PENDING_NEWER_STH);
    curr_entry->second.state = PENDING_INCLUSION_PROOF_REQUEST;
  }

  if (auditable_entries_begin == auditable_entries_end)
    return;

  ProcessPendingEntries();
}

void SingleTreeTracker::ResetPendingQueue() {
  // Move entries out of pending_entries_ prior to deleting them, in case any
  // have inclusion checks in progress. Cancelling those checks would invoke the
  // cancellation callback (ProcessPendingEntries()), which would attempt to
  // access pending_entries_ while it was in the process of being deleted.
  std::map<EntryToAudit, EntryAuditState, OrderByTimestamp> pending_entries;
  pending_entries_.swap(pending_entries);
}

SingleTreeTracker::SCTInclusionStatus
SingleTreeTracker::GetLogEntryInclusionStatus(
    net::X509Certificate* cert,
    const SignedCertificateTimestamp* sct) {
  EntryToAudit entry(sct->timestamp);
  if (!GetLogEntryLeafHash(cert, sct, &entry.leaf_hash))
    return SCT_NOT_OBSERVED;
  return GetAuditedEntryInclusionStatus(entry);
}

void SingleTreeTracker::ProcessPendingEntries() {
  for (auto it = pending_entries_.begin(); it != pending_entries_.end(); ++it) {
    if (it->second.state != PENDING_INCLUSION_PROOF_REQUEST) {
      continue;
    }

    it->second.root_hash =
        std::string(verified_sth_.sha256_root_hash, crypto::kSHA256Length);

    std::string leaf_hash(
        reinterpret_cast<const char*>(it->first.leaf_hash.data),
        crypto::kSHA256Length);
    net::Error result = dns_client_->QueryAuditProof(
        ct_log_->dns_domain(), leaf_hash, verified_sth_.tree_size,
        &(it->second.audit_proof_query),
        base::Bind(&SingleTreeTracker::OnAuditProofObtained,
                   base::Unretained(this), it->first));
    // Handling proofs returned synchronously is not implemeted.
    DCHECK_NE(result, net::OK);
    if (result == net::ERR_IO_PENDING) {
      // Successfully requested an inclusion proof - change entry state
      // and continue to the next one.
      it->second.state = INCLUSION_PROOF_REQUESTED;
    } else if (result == net::ERR_TEMPORARILY_THROTTLED) {
      // Need to use a weak pointer here, as this callback could be triggered
      // when the SingleTreeTracker is deleted (and pending queries are
      // cancelled).
      dns_client_->NotifyWhenNotThrottled(
          base::BindOnce(&SingleTreeTracker::ProcessPendingEntries,
                         weak_factory_.GetWeakPtr()));
      // Exit the loop since all subsequent calls to QueryAuditProof
      // will be throttled.
      break;
    } else if (result == net::ERR_NAME_RESOLUTION_FAILED) {
      LogInclusionCheckResult(DNS_QUERY_NOT_POSSIBLE);
      LogAuditResultToNetLog(it->first, false);
      // Lookup failed due to bad DNS configuration, erase the entry and
      // continue to the next one.
      it = pending_entries_.erase(it);
      // Break here if it's the last entry to avoid |it| being incremented
      // by the for loop.
      if (it == pending_entries_.end())
        break;
    } else {
      // BUG: an invalid argument was provided or an unexpected error
      // was returned from LogDnsClient.
      DCHECK_EQ(result, net::ERR_INVALID_ARGUMENT);
      NOTREACHED();
    }
  }
}

SingleTreeTracker::SCTInclusionStatus
SingleTreeTracker::GetAuditedEntryInclusionStatus(const EntryToAudit& entry) {
  const auto checked_entries_iterator = checked_entries_.Get(entry.leaf_hash);
  if (checked_entries_iterator != checked_entries_.end()) {
    return SCT_INCLUDED_IN_LOG;
  }

  auto pending_iterator = pending_entries_.find(entry);
  if (pending_iterator == pending_entries_.end()) {
    return SCT_NOT_OBSERVED;
  }

  switch (pending_iterator->second.state) {
    case PENDING_NEWER_STH:
      return SCT_PENDING_NEWER_STH;
    case PENDING_INCLUSION_PROOF_REQUEST:
    case INCLUSION_PROOF_REQUESTED:
      return SCT_PENDING_INCLUSION_CHECK;
  }

  NOTREACHED();
  return SCT_NOT_OBSERVED;
}

void SingleTreeTracker::OnAuditProofObtained(const EntryToAudit& entry,
                                             int net_error) {
  auto it = pending_entries_.find(entry);
  // The entry may not be present if it was evacuated due to low memory
  // pressure.
  if (it == pending_entries_.end())
    return;

  DCHECK_EQ(it->second.state, INCLUSION_PROOF_REQUESTED);

  if (net_error != net::OK) {
    // TODO(eranm): Should failures be cached? For now, they are not.
    LogInclusionCheckResult(FAILED_GETTING_INCLUSION_PROOF);
    LogAuditResultToNetLog(entry, false);
    pending_entries_.erase(it);
    return;
  }

  std::string leaf_hash(reinterpret_cast<const char*>(entry.leaf_hash.data),
                        crypto::kSHA256Length);

  bool verified =
      ct_log_->VerifyAuditProof(it->second.audit_proof_query->GetProof(),
                                it->second.root_hash, leaf_hash);
  LogAuditResultToNetLog(entry, verified);

  if (!verified) {
    LogInclusionCheckResult(GOT_INVALID_INCLUSION_PROOF);
  } else {
    LogInclusionCheckResult(GOT_VALID_INCLUSION_PROOF);
    checked_entries_.Put(entry.leaf_hash, EntryAuditResult());
  }

  pending_entries_.erase(it);
}

void SingleTreeTracker::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  switch (memory_pressure_level) {
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
      break;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL:
      ResetPendingQueue();
      // Fall through to clearing the other cache.
      FALLTHROUGH;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
      checked_entries_.Clear();
      break;
  }
}

void SingleTreeTracker::LogAuditResultToNetLog(const EntryToAudit& entry,
                                               bool success) {
  net::NetLogParametersCallback net_log_callback =
      base::Bind(&NetLogEntryAuditingEventCallback, &entry.leaf_hash,
                 ct_log_->key_id(), success);

  net_log_.AddEvent(net::NetLogEventType::CT_LOG_ENTRY_AUDITED,
                    net_log_callback);
}

bool SingleTreeTracker::WasLookedUpOverDNS(base::StringPiece hostname) const {
  net::HostCache::Entry::Source source;
  net::HostCache::EntryStaleness staleness;
  return host_resolver_->HasCached(hostname, &source, &staleness) &&
         source == net::HostCache::Entry::SOURCE_DNS &&
         staleness.network_changes == 0;
}

}  // namespace certificate_transparency
