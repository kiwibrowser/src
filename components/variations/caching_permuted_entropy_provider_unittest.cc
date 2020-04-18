// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/caching_permuted_entropy_provider.h"

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

// Size of the low entropy source to use for the permuted entropy provider
// in tests.
const size_t kMaxLowEntropySize = 8000;

// Field trial names used in unit tests.
const char* const kTestTrialNames[] = {"TestTrial", "AnotherTestTrial",
                                       "NewTabButton"};

TEST(CachingPermutedEntropyProviderTest, HasConsistentResults) {
  TestingPrefServiceSimple prefs;
  CachingPermutedEntropyProvider::RegisterPrefs(prefs.registry());
  const int kEntropyValue = 1234;

  // Check that the caching provider returns the same results as the non caching
  // one. Loop over the trial names twice, to test that caching returns the
  // expected results.
  PermutedEntropyProvider provider(kEntropyValue, kMaxLowEntropySize);
  for (size_t i = 0; i < 2 * arraysize(kTestTrialNames); ++i) {
    CachingPermutedEntropyProvider cached_provider(
        &prefs, kEntropyValue, kMaxLowEntropySize);
    const std::string trial_name =
        kTestTrialNames[i % arraysize(kTestTrialNames)];
    EXPECT_DOUBLE_EQ(provider.GetEntropyForTrial(trial_name, 0),
                     cached_provider.GetEntropyForTrial(trial_name, 0));
  }

  // Now, do the same test re-using the same caching provider.
  CachingPermutedEntropyProvider cached_provider(
      &prefs, kEntropyValue, kMaxLowEntropySize);
  for (size_t i = 0; i < 2 * arraysize(kTestTrialNames); ++i) {
    const std::string trial_name =
        kTestTrialNames[i % arraysize(kTestTrialNames)];
    EXPECT_DOUBLE_EQ(provider.GetEntropyForTrial(trial_name, 0),
                     cached_provider.GetEntropyForTrial(trial_name, 0));
  }
}

}  // namespace variations
