// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/single_tree_tracker.h"

#include <string>
#include <utility>

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/test/metrics/histogram_tester.h"
#include "components/base32/base32.h"
#include "components/certificate_transparency/log_dns_client.h"
#include "components/certificate_transparency/mock_log_dns_traffic.h"
#include "crypto/sha2.h"
#include "net/base/network_change_notifier.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/merkle_tree_leaf.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_tree_head.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/dns_client.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log.h"
#include "net/log/test_net_log.h"
#include "net/log/test_net_log_util.h"
#include "net/test/ct_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::ct::SignedCertificateTimestamp;
using net::ct::SignedTreeHead;
using net::ct::GetSampleSignedTreeHead;
using net::ct::GetTestPublicKeyId;
using net::ct::GetTestPublicKey;
using net::ct::kSthRootHashLength;
using net::ct::GetX509CertSCT;

namespace certificate_transparency {

namespace {

const char kHostname[] = "example.test";
const char kCanCheckForInclusionHistogramName[] =
    "Net.CertificateTransparency.CanInclusionCheckSCT";
const char kInclusionCheckResultHistogramName[] =
    "Net.CertificateTransparency.InclusionCheckResult";

const char kDNSRequestSuffix[] = "dns.example.com";

// These tests use a 0 time-to-live for HostCache entries, so all entries will
// be stale. This is fine because SingleTreeTracker considers stale entries to
// still be evidence that a DNS lookup was performed for a given hostname.
// Ignoring stale entries could be exploited, as an attacker could set their
// website's DNS record to have a very short TTL in order to avoid having
// inclusion checks performed on the SCTs they provide.
constexpr base::TimeDelta kZeroTTL;

constexpr base::TimeDelta kMoreThanMMD = base::TimeDelta::FromHours(25);

bool GetOldSignedTreeHead(SignedTreeHead* sth) {
  sth->version = SignedTreeHead::V1;
  sth->timestamp = base::Time::UnixEpoch() +
                   base::TimeDelta::FromMilliseconds(INT64_C(1348589665525));
  sth->tree_size = 12u;

  const uint8_t kOldSTHRootHash[] = {
      0x18, 0x04, 0x1b, 0xd4, 0x66, 0x50, 0x83, 0x00, 0x1f, 0xba, 0x8c,
      0x54, 0x11, 0xd2, 0xd7, 0x48, 0xe8, 0xab, 0xbf, 0xdc, 0xdf, 0xd9,
      0x21, 0x8c, 0xb0, 0x2b, 0x68, 0xa7, 0x8e, 0x7d, 0x4c, 0x23};
  memcpy(sth->sha256_root_hash, kOldSTHRootHash, kSthRootHashLength);

  sth->log_id = GetTestPublicKeyId();

  const uint8_t kOldSTHSignatureData[] = {
      0x04, 0x03, 0x00, 0x47, 0x30, 0x45, 0x02, 0x20, 0x15, 0x7b, 0x23,
      0x42, 0xa2, 0x5f, 0x88, 0xc9, 0x0b, 0x30, 0xa6, 0xb4, 0x49, 0x50,
      0xb3, 0xab, 0xf5, 0x25, 0xfe, 0x27, 0xf0, 0x3f, 0x9a, 0xbf, 0xc1,
      0x16, 0x5a, 0x7a, 0xc0, 0x62, 0x2b, 0xbb, 0x02, 0x21, 0x00, 0xe6,
      0x57, 0xa3, 0xfe, 0xfc, 0x5a, 0x82, 0x9b, 0x29, 0x46, 0x15, 0x1d,
      0xbc, 0xfd, 0x9e, 0x87, 0x7f, 0xd0, 0x00, 0x5d, 0x62, 0x4f, 0x9a,
      0x1a, 0x9f, 0x20, 0x79, 0xd0, 0xc1, 0x34, 0x2e, 0x08};
  base::StringPiece sp(reinterpret_cast<const char*>(kOldSTHSignatureData),
                       sizeof(kOldSTHSignatureData));
  return DecodeDigitallySigned(&sp, &(sth->signature)) && sp.empty();
}

scoped_refptr<SignedCertificateTimestamp> GetSCT() {
  scoped_refptr<SignedCertificateTimestamp> sct;

  // TODO(eranm): Move setting of the origin field to ct_test_util.cc
  GetX509CertSCT(&sct);
  sct->origin = SignedCertificateTimestamp::SCT_FROM_OCSP_RESPONSE;
  return sct;
}

std::string LeafHash(const net::X509Certificate* cert,
                     const SignedCertificateTimestamp* sct) {
  net::ct::MerkleTreeLeaf leaf;
  if (!GetMerkleTreeLeaf(cert, sct, &leaf))
    return std::string();

  std::string leaf_hash;
  if (!HashMerkleTreeLeaf(leaf, &leaf_hash))
    return std::string();

  return leaf_hash;
}

std::string Base32LeafHash(const net::X509Certificate* cert,
                           const SignedCertificateTimestamp* sct) {
  std::string leaf_hash = LeafHash(cert, sct);
  if (leaf_hash.empty())
    return std::string();

  return base32::Base32Encode(leaf_hash,
                              base32::Base32EncodePolicy::OMIT_PADDING);
}

// Fills in |sth| for a tree of size 2, where the root hash is a hash of
// the test SCT (from GetX509CertSCT) and another entry,
// whose hash is '0a' 32 times.
bool GetSignedTreeHeadForTreeOfSize2(SignedTreeHead* sth) {
  sth->version = SignedTreeHead::V1;
  // Timestamp is after the timestamp of the test SCT (GetX509CertSCT)
  // to indicate it can be audited using this STH.
  sth->timestamp = base::Time::UnixEpoch() +
                   base::TimeDelta::FromMilliseconds(INT64_C(1365354256089));
  sth->tree_size = 2;
  // Root hash is:
  // HASH (0x01 || HASH(log entry made of test SCT) || HASH(0x0a * 32))
  // The proof provided by FillVectorWithValidAuditProofForTreeOfSize2 would
  // validate with this root hash for the log entry made of the test SCT +
  // cert.
  const uint8_t kRootHash[] = {0x16, 0x80, 0xbd, 0x5a, 0x1b, 0xc1, 0xb6, 0xcf,
                               0x1b, 0x7e, 0x77, 0x41, 0xeb, 0xed, 0x86, 0x8b,
                               0x73, 0x81, 0x87, 0xf5, 0xab, 0x93, 0x6d, 0xb2,
                               0x0a, 0x79, 0x0d, 0x9e, 0x40, 0x55, 0xc3, 0xe6};
  memcpy(sth->sha256_root_hash, reinterpret_cast<const char*>(kRootHash),
         kSthRootHashLength);

  sth->log_id = GetTestPublicKeyId();

  // valid signature over the STH, using the test log key at:
  // https://github.com/google/certificate-transparency/blob/master/test/testdata/ct-server-key.pem
  const uint8_t kTreeHeadSignatureData[] = {
      0x04, 0x03, 0x00, 0x46, 0x30, 0x44, 0x02, 0x20, 0x25, 0xa1, 0x9d,
      0x7b, 0xf6, 0xe6, 0xfc, 0x47, 0xa7, 0x2d, 0xef, 0x6b, 0xf4, 0x84,
      0x71, 0xb7, 0x7b, 0x7e, 0xd4, 0x4c, 0x7a, 0x5c, 0x4f, 0x9a, 0xb7,
      0x04, 0x71, 0x6e, 0xd0, 0xa8, 0x0f, 0x53, 0x02, 0x20, 0x27, 0xe5,
      0xed, 0x7d, 0xc3, 0x5d, 0x4c, 0xf0, 0x67, 0x35, 0x5d, 0x8a, 0x10,
      0xae, 0x25, 0x87, 0x1a, 0xef, 0xea, 0xd2, 0xf7, 0xe3, 0x73, 0x2f,
      0x07, 0xb3, 0x4b, 0xea, 0x5b, 0xdd, 0x81, 0x2d};

  base::StringPiece sp(reinterpret_cast<const char*>(kTreeHeadSignatureData),
                       sizeof(kTreeHeadSignatureData));
  return DecodeDigitallySigned(&sp, &sth->signature);
}

void FillVectorWithValidAuditProofForTreeOfSize2(
    std::vector<std::string>* out_proof) {
  std::string node(crypto::kSHA256Length, '\0');
  for (size_t i = 0; i < crypto::kSHA256Length; ++i) {
    node[i] = static_cast<char>(0x0a);
  }
  out_proof->push_back(node);
}

void AddCacheEntry(net::HostCache* cache,
                   const std::string& hostname,
                   net::HostCache::Entry::Source source,
                   base::TimeDelta ttl) {
  cache->Set(net::HostCache::Key(hostname, net::ADDRESS_FAMILY_UNSPECIFIED, 0),
             net::HostCache::Entry(net::OK, net::AddressList(), source),
             base::TimeTicks::Now(), ttl);
}

}  // namespace

class SingleTreeTrackerTest : public ::testing::Test {
  void SetUp() override {
    log_ = net::CTLogVerifier::Create(GetTestPublicKey(), "testlog",
                                      kDNSRequestSuffix);

    ASSERT_TRUE(log_);
    ASSERT_EQ(log_->key_id(), GetTestPublicKeyId());

    const std::string der_test_cert(net::ct::GetDerEncodedX509Cert());
    chain_ = net::X509Certificate::CreateFromBytes(der_test_cert.data(),
                                                   der_test_cert.length());
    ASSERT_TRUE(chain_.get());
    GetX509CertSCT(&cert_sct_);
    cert_sct_->origin = SignedCertificateTimestamp::SCT_FROM_OCSP_RESPONSE;

    net_change_notifier_ =
        base::WrapUnique(net::NetworkChangeNotifier::CreateMock());
    mock_dns_.InitializeDnsConfig();
  }

 protected:
  void CreateTreeTracker() {
    log_dns_client_ = std::make_unique<LogDnsClient>(
        mock_dns_.CreateDnsClient(), net_log_with_source_, 1);

    tree_tracker_ = std::make_unique<SingleTreeTracker>(
        log_, log_dns_client_.get(), &host_resolver_, &net_log_);
  }

  void CreateTreeTrackerWithDefaultDnsExpectation() {
    // Default to throttling requests as it means observed log entries will
    // be frozen in a pending state, simplifying testing of the
    // SingleTreeTracker.
    ASSERT_TRUE(ExpectLeafIndexRequestAndThrottle(chain_, cert_sct_));
    CreateTreeTracker();
  }

  // Configured the |mock_dns_| to expect a request for the leaf index
  // and have th mock DNS client throttle it.
  bool ExpectLeafIndexRequestAndThrottle(
      const scoped_refptr<net::X509Certificate>& chain,
      const scoped_refptr<SignedCertificateTimestamp>& sct) {
    return mock_dns_.ExpectRequestAndSocketError(
        Base32LeafHash(chain.get(), sct.get()) + ".hash." + kDNSRequestSuffix,
        net::Error::ERR_TEMPORARILY_THROTTLED);
  }

  bool MatchAuditingResultInNetLog(net::TestNetLog& net_log,
                                   std::string expected_leaf_hash,
                                   bool expected_success) {
    net::TestNetLogEntry::List entries;

    net_log.GetEntries(&entries);
    if (entries.size() == 0)
      return false;

    size_t pos = net::ExpectLogContainsSomewhere(
        entries, 0, net::NetLogEventType::CT_LOG_ENTRY_AUDITED,
        net::NetLogEventPhase::NONE);

    const net::TestNetLogEntry& logged_entry = entries[pos];

    std::string logged_log_id, logged_leaf_hash;
    if (!logged_entry.GetStringValue("log_id", &logged_log_id) ||
        !logged_entry.GetStringValue("log_entry", &logged_leaf_hash))
      return false;

    if (base::HexEncode(GetTestPublicKeyId().data(),
                        GetTestPublicKeyId().size()) != logged_log_id)
      return false;

    if (base::HexEncode(expected_leaf_hash.data(), expected_leaf_hash.size()) !=
        logged_leaf_hash)
      return false;

    bool logged_success;
    if (!logged_entry.GetBooleanValue("success", &logged_success))
      return false;

    return logged_success == expected_success;
  }

  base::MessageLoopForIO message_loop_;
  MockLogDnsTraffic mock_dns_;
  scoped_refptr<const net::CTLogVerifier> log_;
  std::unique_ptr<net::NetworkChangeNotifier> net_change_notifier_;
  std::unique_ptr<LogDnsClient> log_dns_client_;
  net::MockCachingHostResolver host_resolver_;
  std::unique_ptr<SingleTreeTracker> tree_tracker_;
  scoped_refptr<net::X509Certificate> chain_;
  scoped_refptr<SignedCertificateTimestamp> cert_sct_;
  net::TestNetLog net_log_;
  net::NetLogWithSource net_log_with_source_;
};

// Test that an SCT is discarded if the HostResolver cache does not indicate
// that the hostname lookup was done using DNS. To perform an inclusion check
// in this case could compromise privacy, as the DNS resolver would learn that
// the user had visited that host.
TEST_F(SingleTreeTrackerTest, DiscardsSCTWhenHostnameNotLookedUpUsingDNS) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_UNKNOWN, kZeroTTL);

  base::HistogramTester histograms;
  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Make sure the SCT status is the same as if there's no STH for this log.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // The status for this SCT should still be 'not observed'.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Exactly one value should be logged, indicating that the SCT could not be
  // checked for inclusion because of no prior DNS lookup for this hostname.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 4, 1);

  // Nothing should be logged in the result histogram or net log since an
  // inclusion check wasn't performed.
  histograms.ExpectTotalCount(kInclusionCheckResultHistogramName, 0);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that an SCT is discarded if the hostname it was obtained from is an IP
// literal. To perform an inclusion check in this case could compromise privacy,
// as the DNS resolver would learn that the user had visited that host.
TEST_F(SingleTreeTrackerTest, DiscardsSCTWhenHostnameIsIPLiteral) {
  CreateTreeTrackerWithDefaultDnsExpectation();

  base::HistogramTester histograms;
  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Make sure the SCT status is the same as if there's no STH for this log.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  tree_tracker_->OnSCTVerified("::1", chain_.get(), cert_sct_.get());

  // The status for this SCT should still be 'not observed'.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Exactly one value should be logged, indicating that the SCT could not be
  // checked for inclusion because of no prior DNS lookup for this hostname
  // (because it's an IP literal).
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 4, 1);

  // Nothing should be logged in the result histogram or net log since an
  // inclusion check wasn't performed.
  histograms.ExpectTotalCount(kInclusionCheckResultHistogramName, 0);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that an SCT is discarded if the network has changed since the hostname
// lookup was performed. To perform an inclusion check in this case could
// compromise privacy, as the current DNS resolver would learn that the user had
// visited that host (it would not already know this already because the
// hostname lookup was performed on a different network, using a different DNS
// resolver).
TEST_F(SingleTreeTrackerTest,
       DiscardsSCTWhenHostnameLookedUpUsingDNSOnDiffNetwork) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  // Simulate network change.
  host_resolver_.GetHostCache()->OnNetworkChange();

  base::HistogramTester histograms;
  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Make sure the SCT status is the same as if there's no STH for this log.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // The status for this SCT should still be 'not observed'.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Exactly one value should be logged, indicating that the SCT could not be
  // checked for inclusion because of no prior DNS lookup for this hostname on
  // the current network.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 4, 1);

  // Nothing should be logged in the result histogram or net log since an
  // inclusion check wasn't performed.
  histograms.ExpectTotalCount(kInclusionCheckResultHistogramName, 0);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that an SCT is classified as pending for a newer STH if the
// SingleTreeTracker has not seen any STHs so far.
TEST_F(SingleTreeTrackerTest, CorrectlyClassifiesUnobservedSCTNoSTH) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  base::HistogramTester histograms;
  // First make sure the SCT has not been observed at all.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // Since no STH was provided to the tree_tracker_ the status should be that
  // the SCT is pending a newer STH.
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Expect logging of a value indicating a valid STH is required.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 0, 1);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that an SCT is classified as pending an inclusion check if the
// SingleTreeTracker has a fresh-enough STH to check inclusion against.
TEST_F(SingleTreeTrackerTest, CorrectlyClassifiesUnobservedSCTWithRecentSTH) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  base::HistogramTester histograms;
  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Make sure the SCT status is the same as if there's no STH for
  // this log.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // The status for this SCT should be 'pending inclusion check' since the STH
  // provided at the beginning of the test is newer than the SCT.
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Exactly one value should be logged, indicating the SCT can be checked for
  // inclusion, as |tree_tracker_| did have a valid STH when it was notified
  // of a new SCT.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 2, 1);
  // Nothing should be logged in the result histogram since inclusion check
  // didn't finish.
  histograms.ExpectTotalCount(kInclusionCheckResultHistogramName, 0);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that the SingleTreeTracker correctly queues verified SCTs for inclusion
// checking such that, upon receiving a fresh STH, it changes the SCT's status
// from pending newer STH to pending inclusion check.
TEST_F(SingleTreeTrackerTest, CorrectlyUpdatesSCTStatusOnNewSTH) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  base::HistogramTester histograms;
  // Report an observed SCT and make sure it's in the pending newer STH
  // state.
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  histograms.ExpectTotalCount(kCanCheckForInclusionHistogramName, 1);

  // Provide with a fresh STH
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Test that its status has changed.
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  // Check that no additional UMA was logged for this case as the histogram is
  // only supposed to measure the state of newly-observed SCTs, not pending
  // ones.
  histograms.ExpectTotalCount(kCanCheckForInclusionHistogramName, 1);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that the SingleTreeTracker does not change an SCT's status if an STH
// from the log it was issued by is observed, but that STH is too old to check
// inclusion against.
TEST_F(SingleTreeTrackerTest, DoesNotUpdatesSCTStatusOnOldSTH) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  // Notify of an SCT and make sure it's in the 'pending newer STH' state.
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Provide an old STH for the same log.
  SignedTreeHead sth;
  GetOldSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  // Make sure the SCT's state hasn't changed.
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that the SingleTreeTracker correctly logs that an SCT is pending a new
// STH, when it has a valid STH,  but the observed SCT is not covered by the
// STH.
TEST_F(SingleTreeTrackerTest, LogsUMAForNewSCTAndOldSTH) {
  CreateTreeTrackerWithDefaultDnsExpectation();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  base::HistogramTester histograms;
  // Provide an old STH for the same log.
  SignedTreeHead sth;
  GetOldSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);

  histograms.ExpectTotalCount(kCanCheckForInclusionHistogramName, 0);

  // Notify of an SCT and make sure it's in the 'pending newer STH' state.
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // Exactly one value should be logged, indicating the SCT cannot be checked
  // for inclusion as the STH is too old.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 1, 1);
  EXPECT_EQ(0u, net_log_.GetSize());
}

// Test that an entry transitions to the "not found" state if the LogDnsClient
// fails to get a leaf index.
TEST_F(SingleTreeTrackerTest, TestEntryNotPendingAfterLeafIndexFetchFailure) {
  ASSERT_TRUE(mock_dns_.ExpectRequestAndSocketError(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      net::Error::ERR_FAILED));

  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Provide with a fresh STH
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  // There should have been one NetLog event, logged with failure.
  EXPECT_TRUE(MatchAuditingResultInNetLog(
      net_log_, LeafHash(chain_.get(), cert_sct_.get()), false));
}

// Test that an entry transitions to the "not found" state if the LogDnsClient
// succeeds to get a leaf index but fails to get an inclusion proof.
TEST_F(SingleTreeTrackerTest, TestEntryNotPendingAfterInclusionCheckFailure) {
  // Return 12 as the index of this leaf.
  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      12));
  // Expect a request for an inclusion proof for leaf #12 in a tree of size
  // 21, which is the size of the tree in the STH returned by
  // GetSampleSignedTreeHead.
  ASSERT_TRUE(mock_dns_.ExpectRequestAndSocketError(
      std::string("0.12.21.tree.") + kDNSRequestSuffix,
      net::Error::ERR_FAILED));

  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Provide with a fresh STH
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  tree_tracker_->NewSTHObserved(sth);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  // There should have been one NetLog event, logged with failure.
  EXPECT_TRUE(MatchAuditingResultInNetLog(
      net_log_, LeafHash(chain_.get(), cert_sct_.get()), false));
}

// Test that an entry transitions to the "included" state if the LogDnsClient
// succeeds to get a leaf index and an inclusion proof.
TEST_F(SingleTreeTrackerTest, TestEntryIncludedAfterInclusionCheckSuccess) {
  std::vector<std::string> audit_proof;
  FillVectorWithValidAuditProofForTreeOfSize2(&audit_proof);

  // Return 0 as the index for this leaf, so the proof provided
  // later on would verify.
  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      0));
  // The STH (later on) is for a tree of size 2 and the entry has index 0
  // in the tree, so expect an inclusion proof for entry 0 in a tree
  // of size 2 (0.0.2).
  ASSERT_TRUE(mock_dns_.ExpectAuditProofRequestAndResponse(
      std::string("0.0.2.tree.") + kDNSRequestSuffix, audit_proof.begin(),
      audit_proof.begin() + 1));

  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Provide with a fresh STH, which is for a tree of size 2.
  SignedTreeHead sth;
  ASSERT_TRUE(GetSignedTreeHeadForTreeOfSize2(&sth));
  ASSERT_TRUE(log_->VerifySignedTreeHead(sth));

  tree_tracker_->NewSTHObserved(sth);
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(
      SingleTreeTracker::SCT_INCLUDED_IN_LOG,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  // There should have been one NetLog event, with success logged.
  EXPECT_TRUE(MatchAuditingResultInNetLog(
      net_log_, LeafHash(chain_.get(), cert_sct_.get()), true));
}

// Tests that inclusion checks are aborted and SCTs discarded if under critical
// memory pressure.
TEST_F(SingleTreeTrackerTest,
       TestInclusionCheckCancelledIfUnderMemoryPressure) {
  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  EXPECT_EQ(
      SingleTreeTracker::SCT_PENDING_NEWER_STH,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Provide with a fresh STH, which is for a tree of size 2.
  SignedTreeHead sth;
  ASSERT_TRUE(GetSignedTreeHeadForTreeOfSize2(&sth));
  ASSERT_TRUE(log_->VerifySignedTreeHead(sth));

  // Make the first event that is processed a critical memory pressure
  // notification. This should be handled before the response to the first DNS
  // request, so no requests after the first one should be sent (the leaf index
  // request).
  base::MemoryPressureListener::NotifyMemoryPressure(
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);

  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      0));

  tree_tracker_->NewSTHObserved(sth);
  base::RunLoop().RunUntilIdle();

  // Expect the SCT to have been discarded.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
}

// Test that pending entries transition states correctly according to the
// STHs provided:
// * Start without an STH.
// * Add a collection of entries with mixed timestamps (i.e. SCTs not added
//   in the order of their timestamps).
// * Provide an STH that covers some of the entries, test these are audited.
// * Provide another STH that covers more of the entries, test that the entries
//   already audited are not audited again and that those that need to be
//   audited are audited, while those that are not covered by that STH are
//   not audited.
TEST_F(SingleTreeTrackerTest, TestMultipleEntriesTransitionStateCorrectly) {
  SignedTreeHead old_sth;
  GetOldSignedTreeHead(&old_sth);

  SignedTreeHead new_sth;
  GetSampleSignedTreeHead(&new_sth);

  base::TimeDelta kLessThanMMD = base::TimeDelta::FromHours(23);

  // Assert the gap between the two timestamps is big enough so that
  // all assumptions below on which SCT can be audited with the
  // new STH are true.
  ASSERT_LT(old_sth.timestamp + (kMoreThanMMD * 2), new_sth.timestamp);

  // Oldest SCT - auditable by the old and new STHs.
  scoped_refptr<SignedCertificateTimestamp> oldest_sct(GetSCT());
  oldest_sct->timestamp = old_sth.timestamp - kMoreThanMMD;

  // SCT that's older than the old STH's timestamp but by less than the MMD,
  // so not auditable by old STH.
  scoped_refptr<SignedCertificateTimestamp> not_auditable_by_old_sth_sct(
      GetSCT());
  not_auditable_by_old_sth_sct->timestamp = old_sth.timestamp - kLessThanMMD;

  // SCT that's newer than the old STH's timestamp so is only auditable by
  // the new STH.
  scoped_refptr<SignedCertificateTimestamp> newer_than_old_sth_sct(GetSCT());
  newer_than_old_sth_sct->timestamp = old_sth.timestamp + kLessThanMMD;

  // SCT that's older than the new STH's timestamp but by less than the MMD,
  // so isn't auditable by the new STH.
  scoped_refptr<SignedCertificateTimestamp> not_auditable_by_new_sth_sct(
      GetSCT());
  not_auditable_by_new_sth_sct->timestamp = new_sth.timestamp - kLessThanMMD;

  // SCT that's newer than the new STH's timestamp so isn't auditable by the
  // the new STH.
  scoped_refptr<SignedCertificateTimestamp> newer_than_new_sth_sct(GetSCT());
  newer_than_new_sth_sct->timestamp = new_sth.timestamp + kLessThanMMD;

  // Set up DNS expectations based on inclusion proof request order.
  ASSERT_TRUE(ExpectLeafIndexRequestAndThrottle(chain_, oldest_sct));
  ASSERT_TRUE(
      ExpectLeafIndexRequestAndThrottle(chain_, not_auditable_by_old_sth_sct));
  ASSERT_TRUE(
      ExpectLeafIndexRequestAndThrottle(chain_, newer_than_old_sth_sct));
  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  // Add SCTs in mixed order.
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(),
                               newer_than_new_sth_sct.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), oldest_sct.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(),
                               not_auditable_by_new_sth_sct.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(),
                               newer_than_old_sth_sct.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(),
                               not_auditable_by_old_sth_sct.get());

  // Ensure all are in the PENDING_NEWER_STH state.
  for (const auto& sct :
       {oldest_sct, not_auditable_by_old_sth_sct, newer_than_old_sth_sct,
        not_auditable_by_new_sth_sct, newer_than_new_sth_sct}) {
    ASSERT_EQ(
        SingleTreeTracker::SCT_PENDING_NEWER_STH,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()))
        << "SCT age: " << sct->timestamp;
  }

  // Provide the old STH, ensure only the oldest one is auditable.
  tree_tracker_->NewSTHObserved(old_sth);
  // Ensure all but the oldest are in the PENDING_NEWER_STH state.
  ASSERT_EQ(SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
            tree_tracker_->GetLogEntryInclusionStatus(chain_.get(),
                                                      oldest_sct.get()));

  for (const auto& sct :
       {not_auditable_by_old_sth_sct, newer_than_old_sth_sct,
        not_auditable_by_new_sth_sct, newer_than_new_sth_sct}) {
    ASSERT_EQ(
        SingleTreeTracker::SCT_PENDING_NEWER_STH,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()))
        << "SCT age: " << sct->timestamp;
  }

  // Provide the newer one, ensure two more are auditable but the
  // rest aren't.
  tree_tracker_->NewSTHObserved(new_sth);

  for (const auto& sct :
       {not_auditable_by_old_sth_sct, newer_than_old_sth_sct}) {
    ASSERT_EQ(
        SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()))
        << "SCT age: " << sct->timestamp;
  }

  for (const auto& sct :
       {not_auditable_by_new_sth_sct, newer_than_new_sth_sct}) {
    ASSERT_EQ(
        SingleTreeTracker::SCT_PENDING_NEWER_STH,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()))
        << "SCT age: " << sct->timestamp;
  }
}

// Test that if a request for an entry is throttled, it remains in a
// pending state.

// Test that if several entries are throttled, when the LogDnsClient notifies
// of un-throttling all entries are handled.
TEST_F(SingleTreeTrackerTest, TestThrottledEntryGetsHandledAfterUnthrottling) {
  std::vector<std::string> audit_proof;
  FillVectorWithValidAuditProofForTreeOfSize2(&audit_proof);

  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      0));
  ASSERT_TRUE(mock_dns_.ExpectAuditProofRequestAndResponse(
      std::string("0.0.2.tree.") + kDNSRequestSuffix, audit_proof.begin(),
      audit_proof.begin() + 1));

  scoped_refptr<SignedCertificateTimestamp> second_sct(GetSCT());
  second_sct->timestamp -= base::TimeDelta::FromHours(1);

  // Process request for |second_sct|
  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), second_sct.get()) + ".hash." +
          kDNSRequestSuffix,
      1));
  ASSERT_TRUE(mock_dns_.ExpectAuditProofRequestAndResponse(
      std::string("0.1.2.tree.") + kDNSRequestSuffix, audit_proof.begin(),
      audit_proof.begin() + 1));

  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  SignedTreeHead sth;
  ASSERT_TRUE(GetSignedTreeHeadForTreeOfSize2(&sth));
  tree_tracker_->NewSTHObserved(sth);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), second_sct.get());

  // Both entries should be in the pending state, the first because the
  // LogDnsClient did not invoke the callback yet, the second one because
  // the LogDnsClient is "busy" with the first entry and so would throttle.
  ASSERT_EQ(
      SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));
  ASSERT_EQ(SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
            tree_tracker_->GetLogEntryInclusionStatus(chain_.get(),
                                                      second_sct.get()));

  // Process pending DNS queries so later assertions are on handling
  // of the entries based on replies received.
  base::RunLoop().RunUntilIdle();

  // Check that the first sct is included in the log.
  ASSERT_EQ(
      SingleTreeTracker::SCT_INCLUDED_IN_LOG,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Check that the second SCT got an invalid proof and is not included, rather
  // than being in the pending state.
  ASSERT_EQ(SingleTreeTracker::SCT_NOT_OBSERVED,
            tree_tracker_->GetLogEntryInclusionStatus(chain_.get(),
                                                      second_sct.get()));
}

// Test that proof fetching failure due to DNS config errors is handled
// correctly:
//   (1) Entry removed from pending queue.
//   (2) UMA logged
TEST_F(SingleTreeTrackerTest,
       TestProofLookupDueToBadDNSConfigHandledCorrectly) {
  base::HistogramTester histograms;
  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);

  // Clear existing DNS configuration, so that the DnsClient created
  // by the MockLogDnsTraffic has no valid DnsConfig.
  net_change_notifier_.reset();
  net_change_notifier_ =
      base::WrapUnique(net::NetworkChangeNotifier::CreateMock());
  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  tree_tracker_->NewSTHObserved(sth);
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());

  // Make sure the SCT status indicates the entry has been removed from
  // the SingleTreeTracker's internal queue as the DNS lookup failed
  // synchronously.
  EXPECT_EQ(
      SingleTreeTracker::SCT_NOT_OBSERVED,
      tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), cert_sct_.get()));

  // Exactly one value should be logged, indicating the SCT can be checked for
  // inclusion, as |tree_tracker_| did have a valid STH when it was notified
  // of a new SCT.
  histograms.ExpectUniqueSample(kCanCheckForInclusionHistogramName, 2, 1);
  // Failure due to DNS configuration should be logged in the result histogram.
  histograms.ExpectUniqueSample(kInclusionCheckResultHistogramName, 3, 1);
}

// Test that entries are no longer pending after a network state
// change.
TEST_F(SingleTreeTrackerTest, DiscardsPendingEntriesAfterNetworkChange) {
  // Setup expectations for 2 SCTs to pass inclusion checking.
  // However, the first should be cancelled half way through (when the network
  // change occurs) and the second should be throttled (and then cancelled) so,
  // by the end of test, neither should actually have passed the checks.
  std::vector<std::string> audit_proof;
  FillVectorWithValidAuditProofForTreeOfSize2(&audit_proof);

  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), cert_sct_.get()) + ".hash." +
          kDNSRequestSuffix,
      0));
  ASSERT_TRUE(mock_dns_.ExpectAuditProofRequestAndResponse(
      std::string("0.0.2.tree.") + kDNSRequestSuffix, audit_proof.begin(),
      audit_proof.begin() + 1));

  scoped_refptr<SignedCertificateTimestamp> second_sct(GetSCT());
  second_sct->timestamp -= base::TimeDelta::FromHours(1);

  ASSERT_TRUE(mock_dns_.ExpectLeafIndexRequestAndResponse(
      Base32LeafHash(chain_.get(), second_sct.get()) + ".hash." +
          kDNSRequestSuffix,
      1));
  ASSERT_TRUE(mock_dns_.ExpectAuditProofRequestAndResponse(
      std::string("0.1.2.tree.") + kDNSRequestSuffix, audit_proof.begin(),
      audit_proof.begin() + 1));

  CreateTreeTracker();
  AddCacheEntry(host_resolver_.GetHostCache(), kHostname,
                net::HostCache::Entry::SOURCE_DNS, kZeroTTL);

  // Provide an STH to the tree_tracker_.
  SignedTreeHead sth;
  GetSignedTreeHeadForTreeOfSize2(&sth);
  tree_tracker_->NewSTHObserved(sth);

  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), second_sct.get());

  for (auto sct : {cert_sct_, second_sct}) {
    EXPECT_EQ(
        SingleTreeTracker::SCT_PENDING_INCLUSION_CHECK,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()));
  }

  net_change_notifier_->NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  base::RunLoop().RunUntilIdle();

  for (auto sct : {cert_sct_, second_sct}) {
    EXPECT_EQ(
        SingleTreeTracker::SCT_NOT_OBSERVED,
        tree_tracker_->GetLogEntryInclusionStatus(chain_.get(), sct.get()));
  }
}

}  // namespace certificate_transparency
