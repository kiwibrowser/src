// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/entropy_provider.h"

#include <stddef.h>
#include <stdint.h>

#include <cmath>
#include <limits>
#include <memory>
#include <numeric>

#include "base/guid.h"
#include "base/macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "components/variations/hashing.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace variations {

namespace {

// Size of the low entropy source to use for the permuted entropy provider
// in tests.
const size_t kMaxLowEntropySize = 8000;

// Field trial names used in unit tests.
const char* const kTestTrialNames[] = { "TestTrial", "AnotherTestTrial",
                                        "NewTabButton" };

// Computes the Chi-Square statistic for |values| assuming they follow a uniform
// distribution, where each entry has expected value |expected_value|.
//
// The Chi-Square statistic is defined as Sum((O-E)^2/E) where O is the observed
// value and E is the expected value.
double ComputeChiSquare(const std::vector<int>& values,
                        double expected_value) {
  double sum = 0;
  for (size_t i = 0; i < values.size(); ++i) {
    const double delta = values[i] - expected_value;
    sum += (delta * delta) / expected_value;
  }
  return sum;
}

// Computes SHA1-based entropy for the given |trial_name| based on
// |entropy_source|
double GenerateSHA1Entropy(const std::string& entropy_source,
                           const std::string& trial_name) {
  SHA1EntropyProvider sha1_provider(entropy_source);
  return sha1_provider.GetEntropyForTrial(trial_name, 0);
}

// Generates permutation-based entropy for the given |trial_name| based on
// |entropy_source| which must be in the range [0, entropy_max).
double GeneratePermutedEntropy(uint16_t entropy_source,
                               size_t entropy_max,
                               const std::string& trial_name) {
  PermutedEntropyProvider permuted_provider(entropy_source, entropy_max);
  return permuted_provider.GetEntropyForTrial(trial_name, 0);
}

// Helper interface for testing used to generate entropy values for a given
// field trial. Unlike EntropyProvider, which keeps the low/high entropy source
// value constant and generates entropy for different trial names, instances
// of TrialEntropyGenerator keep the trial name constant and generate low/high
// entropy source values internally to produce each output entropy value.
class TrialEntropyGenerator {
 public:
  virtual ~TrialEntropyGenerator() {}
  virtual double GenerateEntropyValue() const = 0;
};

// An TrialEntropyGenerator that uses the SHA1EntropyProvider with the high
// entropy source (random GUID with 128 bits of entropy + 13 additional bits of
// entropy corresponding to a low entropy source).
class SHA1EntropyGenerator : public TrialEntropyGenerator {
 public:
  explicit SHA1EntropyGenerator(const std::string& trial_name)
      : trial_name_(trial_name) {
  }

  ~SHA1EntropyGenerator() override {}

  double GenerateEntropyValue() const override {
    // Use a random GUID + 13 additional bits of entropy to match how the
    // SHA1EntropyProvider is used in metrics_service.cc.
    const int low_entropy_source =
        static_cast<uint16_t>(base::RandInt(0, kMaxLowEntropySize - 1));
    const std::string high_entropy_source =
        base::GenerateGUID() + base::IntToString(low_entropy_source);
    return GenerateSHA1Entropy(high_entropy_source, trial_name_);
  }

 private:
  std::string trial_name_;

  DISALLOW_COPY_AND_ASSIGN(SHA1EntropyGenerator);
};

// An TrialEntropyGenerator that uses the permuted entropy provider algorithm,
// using 13-bit low entropy source values.
class PermutedEntropyGenerator : public TrialEntropyGenerator {
 public:
  explicit PermutedEntropyGenerator(const std::string& trial_name)
      : mapping_(kMaxLowEntropySize) {
    // Note: Given a trial name, the computed mapping will be the same.
    // As a performance optimization, pre-compute the mapping once per trial
    // name and index into it for each entropy value.
    const uint32_t randomization_seed = HashName(trial_name);
    internal::PermuteMappingUsingRandomizationSeed(randomization_seed,
                                                   &mapping_);
  }

  ~PermutedEntropyGenerator() override {}

  double GenerateEntropyValue() const override {
    const int low_entropy_source =
        static_cast<uint16_t>(base::RandInt(0, kMaxLowEntropySize - 1));
    return mapping_[low_entropy_source] /
           static_cast<double>(kMaxLowEntropySize);
  }

 private:
  std::vector<uint16_t> mapping_;

  DISALLOW_COPY_AND_ASSIGN(PermutedEntropyGenerator);
};

// Tests uniformity of a given |entropy_generator| using the Chi-Square Goodness
// of Fit Test.
void PerformEntropyUniformityTest(
    const std::string& trial_name,
    const TrialEntropyGenerator& entropy_generator) {
  // Number of buckets in the simulated field trials.
  const size_t kBucketCount = 20;
  // Max number of iterations to perform before giving up and failing.
  const size_t kMaxIterationCount = 100000;
  // The number of iterations to perform before each time the statistical
  // significance of the results is checked.
  const size_t kCheckIterationCount = 10000;
  // This is the Chi-Square threshold from the Chi-Square statistic table for
  // 19 degrees of freedom (based on |kBucketCount|) with a 99.9% confidence
  // level. See: http://www.medcalc.org/manual/chi-square-table.php
  const double kChiSquareThreshold = 43.82;

  std::vector<int> distribution(kBucketCount);

  for (size_t i = 1; i <= kMaxIterationCount; ++i) {
    const double entropy_value = entropy_generator.GenerateEntropyValue();
    const size_t bucket = static_cast<size_t>(kBucketCount * entropy_value);
    ASSERT_LT(bucket, kBucketCount);
    distribution[bucket] += 1;

    // After |kCheckIterationCount| iterations, compute the Chi-Square
    // statistic of the distribution. If the resulting statistic is greater
    // than |kChiSquareThreshold|, we can conclude with 99.9% confidence
    // that the observed samples do not follow a uniform distribution.
    //
    // However, since 99.9% would still result in a false negative every
    // 1000 runs of the test, do not treat it as a failure (else the test
    // will be flaky). Instead, perform additional iterations to determine
    // if the distribution will converge, up to |kMaxIterationCount|.
    if ((i % kCheckIterationCount) == 0) {
      const double expected_value_per_bucket =
          static_cast<double>(i) / kBucketCount;
      const double chi_square =
          ComputeChiSquare(distribution, expected_value_per_bucket);
      if (chi_square < kChiSquareThreshold)
        break;

      // If |i == kMaxIterationCount|, the Chi-Square statistic did not
      // converge after |kMaxIterationCount|.
      EXPECT_NE(i, kMaxIterationCount) << "Failed for trial " <<
          trial_name << " with chi_square = " << chi_square <<
          " after " << kMaxIterationCount << " iterations.";
    }
  }
}

}  // namespace

TEST(EntropyProviderTest, UseOneTimeRandomizationSHA1) {
  // Simply asserts that two trials using one-time randomization
  // that have different names, normally generate different results.
  //
  // Note that depending on the one-time random initialization, they
  // _might_ actually give the same result, but we know that given
  // the particular client_id we use for unit tests they won't.
  base::FieldTrialList field_trial_list(
      std::make_unique<SHA1EntropyProvider>("client_id"));
  const int kNoExpirationYear = base::FieldTrialList::kNoExpirationYear;
  scoped_refptr<base::FieldTrial> trials[] = {
      base::FieldTrialList::FactoryGetFieldTrial(
          "one", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, nullptr),
      base::FieldTrialList::FactoryGetFieldTrial(
          "two", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, nullptr),
  };

  for (size_t i = 0; i < arraysize(trials); ++i) {
    for (int j = 0; j < 100; ++j)
      trials[i]->AppendGroup(std::string(), 1);
  }

  // The trials are most likely to give different results since they have
  // different names.
  EXPECT_NE(trials[0]->group(), trials[1]->group());
  EXPECT_NE(trials[0]->group_name(), trials[1]->group_name());
}

TEST(EntropyProviderTest, UseOneTimeRandomizationPermuted) {
  // Simply asserts that two trials using one-time randomization
  // that have different names, normally generate different results.
  //
  // Note that depending on the one-time random initialization, they
  // _might_ actually give the same result, but we know that given
  // the particular client_id we use for unit tests they won't.
  base::FieldTrialList field_trial_list(
      std::make_unique<PermutedEntropyProvider>(1234, kMaxLowEntropySize));
  const int kNoExpirationYear = base::FieldTrialList::kNoExpirationYear;
  scoped_refptr<base::FieldTrial> trials[] = {
      base::FieldTrialList::FactoryGetFieldTrial(
          "one", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, nullptr),
      base::FieldTrialList::FactoryGetFieldTrial(
          "two", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, nullptr),
  };

  for (size_t i = 0; i < arraysize(trials); ++i) {
    for (int j = 0; j < 100; ++j)
      trials[i]->AppendGroup(std::string(), 1);
  }

  // The trials are most likely to give different results since they have
  // different names.
  EXPECT_NE(trials[0]->group(), trials[1]->group());
  EXPECT_NE(trials[0]->group_name(), trials[1]->group_name());
}

TEST(EntropyProviderTest, UseOneTimeRandomizationWithCustomSeedPermuted) {
  // Ensures that two trials with different names but the same custom seed used
  // for one time randomization produce the same group assignments.
  base::FieldTrialList field_trial_list(
      std::make_unique<PermutedEntropyProvider>(1234, kMaxLowEntropySize));
  const int kNoExpirationYear = base::FieldTrialList::kNoExpirationYear;
  const uint32_t kCustomSeed = 9001;
  scoped_refptr<base::FieldTrial> trials[] = {
      base::FieldTrialList::FactoryGetFieldTrialWithRandomizationSeed(
          "one", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, kCustomSeed, nullptr, nullptr),
      base::FieldTrialList::FactoryGetFieldTrialWithRandomizationSeed(
          "two", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, kCustomSeed, nullptr, nullptr),
  };

  for (size_t i = 0; i < arraysize(trials); ++i) {
    for (int j = 0; j < 100; ++j)
      trials[i]->AppendGroup(std::string(), 1);
  }

  // Normally, these trials should produce different groups, but if the same
  // custom seed is used, they should produce the same group assignment.
  EXPECT_EQ(trials[0]->group(), trials[1]->group());
  EXPECT_EQ(trials[0]->group_name(), trials[1]->group_name());
}

TEST(EntropyProviderTest, UseOneTimeRandomizationWithCustomSeedSHA1) {
  // Ensures that two trials with different names but the same custom seed used
  // for one time randomization produce the same group assignments.
  base::FieldTrialList field_trial_list(
      std::make_unique<SHA1EntropyProvider>("client_id"));
  const int kNoExpirationYear = base::FieldTrialList::kNoExpirationYear;
  const uint32_t kCustomSeed = 9001;
  scoped_refptr<base::FieldTrial> trials[] = {
      base::FieldTrialList::FactoryGetFieldTrialWithRandomizationSeed(
          "one", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, kCustomSeed, nullptr, nullptr),
      base::FieldTrialList::FactoryGetFieldTrialWithRandomizationSeed(
          "two", 100, "default", kNoExpirationYear, 1, 1,
          base::FieldTrial::ONE_TIME_RANDOMIZED, kCustomSeed, nullptr, nullptr),
  };

  for (size_t i = 0; i < arraysize(trials); ++i) {
    for (int j = 0; j < 100; ++j)
      trials[i]->AppendGroup(std::string(), 1);
  }

  // Normally, these trials should produce different groups, but if the same
  // custom seed is used, they should produce the same group assignment.
  EXPECT_EQ(trials[0]->group(), trials[1]->group());
  EXPECT_EQ(trials[0]->group_name(), trials[1]->group_name());
}

TEST(EntropyProviderTest, SHA1Entropy) {
  const double results[] = { GenerateSHA1Entropy("hi", "1"),
                             GenerateSHA1Entropy("there", "1") };

  EXPECT_NE(results[0], results[1]);
  for (size_t i = 0; i < arraysize(results); ++i) {
    EXPECT_LE(0.0, results[i]);
    EXPECT_GT(1.0, results[i]);
  }

  EXPECT_EQ(GenerateSHA1Entropy("yo", "1"),
            GenerateSHA1Entropy("yo", "1"));
  EXPECT_NE(GenerateSHA1Entropy("yo", "something"),
            GenerateSHA1Entropy("yo", "else"));
}

TEST(EntropyProviderTest, PermutedEntropy) {
  const double results[] = {
      GeneratePermutedEntropy(1234, kMaxLowEntropySize, "1"),
      GeneratePermutedEntropy(4321, kMaxLowEntropySize, "1") };

  EXPECT_NE(results[0], results[1]);
  for (size_t i = 0; i < arraysize(results); ++i) {
    EXPECT_LE(0.0, results[i]);
    EXPECT_GT(1.0, results[i]);
  }

  EXPECT_EQ(GeneratePermutedEntropy(1234, kMaxLowEntropySize, "1"),
            GeneratePermutedEntropy(1234, kMaxLowEntropySize, "1"));
  EXPECT_NE(GeneratePermutedEntropy(1234, kMaxLowEntropySize, "something"),
            GeneratePermutedEntropy(1234, kMaxLowEntropySize, "else"));
}

TEST(EntropyProviderTest, PermutedEntropyProviderResults) {
  // Verifies that PermutedEntropyProvider produces expected results. This
  // ensures that the results are the same between platforms and ensures that
  // changes to the implementation do not regress this accidentally.

  EXPECT_DOUBLE_EQ(2194 / static_cast<double>(kMaxLowEntropySize),
                   GeneratePermutedEntropy(1234, kMaxLowEntropySize, "XYZ"));
  EXPECT_DOUBLE_EQ(5676 / static_cast<double>(kMaxLowEntropySize),
                   GeneratePermutedEntropy(1, kMaxLowEntropySize, "Test"));
  EXPECT_DOUBLE_EQ(1151 / static_cast<double>(kMaxLowEntropySize),
                   GeneratePermutedEntropy(5000, kMaxLowEntropySize, "Foo"));
}

TEST(EntropyProviderTest, SHA1EntropyIsUniform) {
  for (size_t i = 0; i < arraysize(kTestTrialNames); ++i) {
    SHA1EntropyGenerator entropy_generator(kTestTrialNames[i]);
    PerformEntropyUniformityTest(kTestTrialNames[i], entropy_generator);
  }
}

TEST(EntropyProviderTest, PermutedEntropyIsUniform) {
  for (size_t i = 0; i < arraysize(kTestTrialNames); ++i) {
    PermutedEntropyGenerator entropy_generator(kTestTrialNames[i]);
    PerformEntropyUniformityTest(kTestTrialNames[i], entropy_generator);
  }
}

TEST(EntropyProviderTest, SeededRandGeneratorIsUniform) {
  // Verifies that SeededRandGenerator has a uniform distribution.
  //
  // Mirrors RandUtilTest.RandGeneratorIsUniform in base/rand_util_unittest.cc.

  const uint32_t kTopOfRange =
      (std::numeric_limits<uint32_t>::max() / 4ULL) * 3ULL;
  const uint32_t kExpectedAverage = kTopOfRange / 2ULL;
  const uint32_t kAllowedVariance = kExpectedAverage / 50ULL;  // +/- 2%
  const int kMinAttempts = 1000;
  const int kMaxAttempts = 1000000;

  for (size_t i = 0; i < arraysize(kTestTrialNames); ++i) {
    const uint32_t seed = HashName(kTestTrialNames[i]);
    internal::SeededRandGenerator rand_generator(seed);

    double cumulative_average = 0.0;
    int count = 0;
    while (count < kMaxAttempts) {
      uint32_t value = rand_generator(kTopOfRange);
      cumulative_average = (count * cumulative_average + value) / (count + 1);

      // Don't quit too quickly for things to start converging, or we may have
      // a false positive.
      if (count > kMinAttempts &&
          kExpectedAverage - kAllowedVariance < cumulative_average &&
          cumulative_average < kExpectedAverage + kAllowedVariance) {
        break;
      }

      ++count;
    }

    ASSERT_LT(count, kMaxAttempts) << "Expected average was " <<
        kExpectedAverage << ", average ended at " << cumulative_average <<
        ", for trial " << kTestTrialNames[i];
  }
}

}  // namespace variations
