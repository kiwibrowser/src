// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/tree_state_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/certificate_transparency/features.h"
#include "net/base/net_errors.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/merkle_tree_leaf.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/signed_tree_head.h"
#include "net/cert/x509_certificate.h"
#include "net/dns/mock_host_resolver.h"
#include "net/log/net_log.h"
#include "net/log/test_net_log.h"
#include "net/test/ct_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using net::ct::SignedCertificateTimestamp;
using net::ct::SignedTreeHead;
using net::ct::GetSampleSignedTreeHead;
using net::ct::GetTestPublicKeyId;
using net::ct::GetTestPublicKey;
using net::ct::kSthRootHashLength;
using net::ct::GetX509CertSCT;

constexpr char kHostname[] = "example.test";
constexpr base::TimeDelta kZeroTTL;

namespace certificate_transparency {

class TreeStateTrackerTest : public ::testing::Test {
  void SetUp() override {
    log_ = net::CTLogVerifier::Create(GetTestPublicKey(), "testlog",
                                      "unresolvable.invalid");

    ASSERT_TRUE(log_);
    ASSERT_EQ(log_->key_id(), GetTestPublicKeyId());

    const std::string der_test_cert(net::ct::GetDerEncodedX509Cert());
    chain_ = net::X509Certificate::CreateFromBytes(der_test_cert.data(),
                                                   der_test_cert.length());
    ASSERT_TRUE(chain_.get());
    GetX509CertSCT(&cert_sct_);
    cert_sct_->origin = SignedCertificateTimestamp::SCT_FROM_OCSP_RESPONSE;
  }

 protected:
  base::MessageLoopForIO message_loop_;
  scoped_refptr<const net::CTLogVerifier> log_;
  net::MockCachingHostResolver host_resolver_;
  std::unique_ptr<TreeStateTracker> tree_tracker_;
  scoped_refptr<net::X509Certificate> chain_;
  scoped_refptr<SignedCertificateTimestamp> cert_sct_;
  net::TestNetLog net_log_;
};

// Test that a new STH & SCT are delegated correctly to a
// SingleTreeTracker instance created by the TreeStateTracker.
// This is verified by looking for a single event on the net_log_
// passed into the TreeStateTracker c'tor.
TEST_F(TreeStateTrackerTest, TestDelegatesCorrectly) {
  std::vector<scoped_refptr<const net::CTLogVerifier>> verifiers;
  verifiers.push_back(log_);

  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(kCTLogAuditing);

  tree_tracker_ =
      std::make_unique<TreeStateTracker>(verifiers, &host_resolver_, &net_log_);

  // Add a cache entry for kHostname that indicates it was looked up over DNS.
  // SingleTreeTracker requires this before it will request an inclusion proof,
  // as otherwise it would reveal to the DNS resolver which server a user
  // visited. If the server was already looked up via DNS though, that that
  // information is already known to the DNS resolver so there is then no harm.
  host_resolver_.GetHostCache()->Set(
      net::HostCache::Key(kHostname, net::ADDRESS_FAMILY_UNSPECIFIED, 0),
      net::HostCache::Entry(net::OK, net::AddressList(),
                            net::HostCache::Entry::SOURCE_DNS),
      base::TimeTicks::Now(), kZeroTTL);

  SignedTreeHead sth;
  GetSampleSignedTreeHead(&sth);
  ASSERT_EQ(log_->key_id(), sth.log_id);
  tree_tracker_->NewSTHObserved(sth);

  ASSERT_EQ(log_->key_id(), cert_sct_->log_id);
  tree_tracker_->OnSCTVerified(kHostname, chain_.get(), cert_sct_.get());
  base::RunLoop().RunUntilIdle();

  net::ct::MerkleTreeLeaf leaf;
  ASSERT_TRUE(GetMerkleTreeLeaf(chain_.get(), cert_sct_.get(), &leaf));

  std::string leaf_hash;
  ASSERT_TRUE(HashMerkleTreeLeaf(leaf, &leaf_hash));
  // There should be one NetLog event.
  EXPECT_EQ(1u, net_log_.GetSize());
}

}  // namespace certificate_transparency
