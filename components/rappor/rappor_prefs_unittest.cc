// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_prefs.h"

#include <stdint.h>

#include "base/base64.h"
#include "base/macros.h"
#include "base/test/metrics/histogram_tester.h"
#include "components/prefs/testing_pref_service.h"
#include "components/rappor/byte_vector_utils.h"
#include "components/rappor/proto/rappor_metric.pb.h"
#include "components/rappor/rappor_pref_names.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

namespace internal {

namespace {

// Convert a secret to base 64 and store it in preferences.
void StoreSecret(const std::string& secret,
                 TestingPrefServiceSimple* test_prefs) {
  std::string secret_base64;
  base::Base64Encode(secret, &secret_base64);
  test_prefs->SetString(prefs::kRapporSecret, secret_base64);
}

// Verify that the current value of the secret pref matches the loaded secret.
void ExpectConsistentSecret(const TestingPrefServiceSimple& test_prefs,
                            const std::string& loaded_secret) {
  std::string pref = test_prefs.GetString(prefs::kRapporSecret);
  std::string decoded_pref;
  EXPECT_TRUE(base::Base64Decode(pref, &decoded_pref));
  EXPECT_EQ(loaded_secret, decoded_pref);
}

}  // namespace

class RapporPrefsTest : public testing::Test {
 public:
  RapporPrefsTest() {
    RegisterPrefs(test_prefs_.registry());
  }

 protected:
  base::HistogramTester tester_;
  TestingPrefServiceSimple test_prefs_;

  DISALLOW_COPY_AND_ASSIGN(RapporPrefsTest);
};

TEST_F(RapporPrefsTest, EmptyCohort) {
  test_prefs_.ClearPref(prefs::kRapporCohortSeed);
  // Loaded cohort should have been rerolled into a valid number.
  int32_t cohort = LoadCohort(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadCohortHistogramName, LOAD_EMPTY_VALUE, 1);
  EXPECT_GE(cohort, 0);
  EXPECT_LT(cohort, RapporParameters::kMaxCohorts);
  // The preferences should be consistent with the loaded value.
  int32_t pref = test_prefs_.GetInteger(prefs::kRapporCohortSeed);
  EXPECT_EQ(pref, cohort);
}

TEST_F(RapporPrefsTest, LoadCohort) {
  test_prefs_.SetInteger(prefs::kRapporCohortSeed, 1);
  // Loading the valid cohort should just retrieve it.
  int32_t cohort = LoadCohort(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadCohortHistogramName, LOAD_SUCCESS, 1);
  EXPECT_EQ(1, cohort);
  // The preferences should be consistent with the loaded value.
  int32_t pref = test_prefs_.GetInteger(prefs::kRapporCohortSeed);
  EXPECT_EQ(pref, cohort);
}

TEST_F(RapporPrefsTest, CorruptCohort) {
  // Set an invalid cohort value in the preference.
  test_prefs_.SetInteger(prefs::kRapporCohortSeed, -10);
  // Loaded cohort should have been rerolled into a valid number.
  int32_t cohort = LoadCohort(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadCohortHistogramName, LOAD_CORRUPT_VALUE, 1);
  EXPECT_GE(cohort, 0);
  EXPECT_LT(cohort, RapporParameters::kMaxCohorts);
  // The preferences should be consistent with the loaded value.
  int32_t pref = test_prefs_.GetInteger(prefs::kRapporCohortSeed);
  EXPECT_EQ(pref, cohort);
}

TEST_F(RapporPrefsTest, EmptySecret) {
  test_prefs_.ClearPref(prefs::kRapporSecret);
  // Loaded secret should be rerolled from empty.
  std::string secret2 = LoadSecret(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadSecretHistogramName, LOAD_EMPTY_VALUE, 1);
  EXPECT_EQ(HmacByteVectorGenerator::kEntropyInputSize, secret2.size());
  // The stored preference should also be updated.
  ExpectConsistentSecret(test_prefs_, secret2);
}

TEST_F(RapporPrefsTest, LoadSecret) {
  std::string secret1 = HmacByteVectorGenerator::GenerateEntropyInput();
  StoreSecret(secret1, &test_prefs_);
  // Secret should load successfully.
  std::string secret2 = LoadSecret(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadSecretHistogramName, LOAD_SUCCESS, 1);
  EXPECT_EQ(secret1, secret2);
  // The stored preference should also be unchanged.
  ExpectConsistentSecret(test_prefs_, secret2);
}

TEST_F(RapporPrefsTest, CorruptSecret) {
  // Store an invalid secret in the preferences that won't decode as base64.
  test_prefs_.SetString(prefs::kRapporSecret, "!!INVALID!!");
  // We should have rerolled a new secret.
  std::string secret2 = LoadSecret(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadSecretHistogramName, LOAD_CORRUPT_VALUE, 1);
  EXPECT_EQ(HmacByteVectorGenerator::kEntropyInputSize, secret2.size());
  // The stored preference should also be updated.
  ExpectConsistentSecret(test_prefs_, secret2);
}

TEST_F(RapporPrefsTest, DecodableCorruptSecret) {
  // Store an invalid secret in the preferences that will decode as base64.
  std::string secret1 = "!!INVALID!!";
  StoreSecret(secret1, &test_prefs_);
  // We should have rerolled a new secret.
  std::string secret2 = LoadSecret(&test_prefs_);
  tester_.ExpectUniqueSample(kLoadSecretHistogramName, LOAD_CORRUPT_VALUE, 1);
  EXPECT_NE(secret1, secret2);
  EXPECT_EQ(HmacByteVectorGenerator::kEntropyInputSize, secret2.size());
  // The stored preference should also be updated.
  ExpectConsistentSecret(test_prefs_, secret2);
}

}  // namespace internal

}  // namespace rappor
