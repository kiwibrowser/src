// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/certificate_transparency/sth_distributor.h"

#include <map>
#include <string>

#include "base/test/metrics/histogram_tester.h"
#include "components/certificate_transparency/sth_observer.h"
#include "crypto/sha2.h"
#include "net/cert/signed_tree_head.h"
#include "net/test/ct_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace certificate_transparency {

namespace {

// An STHObserver implementation that simply stores all
// observed STHs, keyed by log ID.
class StoringSTHObserver : public STHObserver {
 public:
  void NewSTHObserved(const net::ct::SignedTreeHead& sth) override {
    sths[sth.log_id] = sth;
  }

  std::map<std::string, net::ct::SignedTreeHead> sths;
};

class STHDistributorTest : public ::testing::Test {
 public:
  STHDistributorTest() = default;

  void SetUp() override {
    ASSERT_TRUE(GetSampleSignedTreeHead(&sample_sth_));
    sample_sth_.log_id = net::ct::GetTestPublicKeyId();
  }

 protected:
  STHDistributor distributor_;
  net::ct::SignedTreeHead sample_sth_;
};

// Test that when a new observer is registered, the STHDistributor notifies it
// of all the observed STHs it received so far.
// This test makes sure that all observed STHs are reported to the observer.
TEST_F(STHDistributorTest, NotifiesOfExistingSTHs) {
  // Create an STH that differs from the |sample_sth_| by belonging to a
  // different log.
  const std::string other_log = "another log";
  net::ct::SignedTreeHead second_sth(sample_sth_);
  second_sth.log_id = other_log;

  // Notify |distributor_| of both STHs.
  distributor_.NewSTHObserved(sample_sth_);
  distributor_.NewSTHObserved(second_sth);

  StoringSTHObserver observer;
  distributor_.RegisterObserver(&observer);

  // Check that two STHs from different logs received prior to observer
  // registration were reported to the observer once registered.
  EXPECT_EQ(2u, observer.sths.size());
  EXPECT_EQ(1u, observer.sths.count(other_log));
  distributor_.UnregisterObserver(&observer);
}

// Test that histograms are properly recorded for the STH age when an STH
// from Google's Pilot log is observed.
TEST_F(STHDistributorTest, LogsUMAForPilotSTH) {
  const char kPilotSTHAgeHistogram[] =
      "Net.CertificateTransparency.PilotSTHAge";
  base::HistogramTester histograms;
  histograms.ExpectTotalCount(kPilotSTHAgeHistogram, 0);

  const uint8_t kPilotLogID[] = {
      0xa4, 0xb9, 0x09, 0x90, 0xb4, 0x18, 0x58, 0x14, 0x87, 0xbb, 0x13,
      0xa2, 0xcc, 0x67, 0x70, 0x0a, 0x3c, 0x35, 0x98, 0x04, 0xf9, 0x1b,
      0xdf, 0xb8, 0xe3, 0x77, 0xcd, 0x0e, 0xc8, 0x0d, 0xdc, 0x10};
  sample_sth_.log_id = std::string(reinterpret_cast<const char*>(kPilotLogID),
                                   crypto::kSHA256Length);

  distributor_.NewSTHObserved(sample_sth_);
  histograms.ExpectTotalCount(kPilotSTHAgeHistogram, 1);
}

// Test that the STHDistributor updates, rather than accumulates, STHs
// coming from the same log.
// This is tested by notifying the STHDistributor of an STH, modifying that
// STH, notifying the STHDistributor of the modified STH, then registering
// an observer which should get notified only once, with the modified STH.
TEST_F(STHDistributorTest, UpdatesObservedSTHData) {
  // Observe an initial STH
  StoringSTHObserver observer;
  distributor_.RegisterObserver(&observer);

  distributor_.NewSTHObserved(sample_sth_);

  EXPECT_EQ(1u, observer.sths.size());
  EXPECT_EQ(sample_sth_, observer.sths[net::ct::GetTestPublicKeyId()]);

  // Observe a new STH. "new" simply means that it is a more recently observed
  // SignedTreeHead for the given log ID, not necessarily that it's newer
  // chronologically (the timestamp) or the log state (the tree size).
  // To make sure the more recently observed SignedTreeHead is returned, just
  // modify some fields.
  net::ct::SignedTreeHead new_sth = sample_sth_;
  new_sth.tree_size++;
  new_sth.timestamp -= base::TimeDelta::FromSeconds(3);

  distributor_.NewSTHObserved(new_sth);
  // The STH should have been broadcast to existing observers.
  EXPECT_EQ(1u, observer.sths.size());
  EXPECT_NE(sample_sth_, observer.sths[net::ct::GetTestPublicKeyId()]);
  EXPECT_EQ(new_sth, observer.sths[net::ct::GetTestPublicKeyId()]);

  // Registering a new observer should only receive the most recently observed
  // STH.
  StoringSTHObserver new_observer;
  distributor_.RegisterObserver(&new_observer);
  EXPECT_EQ(1u, new_observer.sths.size());
  EXPECT_NE(sample_sth_, new_observer.sths[net::ct::GetTestPublicKeyId()]);
  EXPECT_EQ(new_sth, new_observer.sths[net::ct::GetTestPublicKeyId()]);

  distributor_.UnregisterObserver(&new_observer);
  distributor_.UnregisterObserver(&observer);
}

}  // namespace

}  // namespace certificate_transparency
