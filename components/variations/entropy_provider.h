// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VARIATIONS_ENTROPY_PROVIDER_H_
#define COMPONENTS_VARIATIONS_ENTROPY_PROVIDER_H_

#include <stddef.h>
#include <stdint.h>

#include <functional>
#include <random>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"

namespace variations {

// Internals of entropy_provider.cc exposed for testing.
namespace internal {

// A functor that generates random numbers based on a seed, using the Mersenne
// Twister algorithm. Suitable for use with std::random_shuffle().
struct SeededRandGenerator {
  explicit SeededRandGenerator(uint32_t seed);
  ~SeededRandGenerator();

  // Returns a random number in range [0, range).
  uint32_t operator()(uint32_t range);

  std::mt19937 mersenne_twister_;
};

// Fills |mapping| to create a bijection of values in the range of
// [0, |mapping.size()|), permuted based on |randomization_seed|.
void PermuteMappingUsingRandomizationSeed(uint32_t randomization_seed,
                                          std::vector<uint16_t>* mapping);

}  // namespace internal

// SHA1EntropyProvider is an entropy provider suitable for high entropy sources.
// It works by taking the first 64 bits of the SHA1 hash of the entropy source
// concatenated with the trial name, or randomization seed and using that for
// the final entropy value.
class SHA1EntropyProvider : public base::FieldTrial::EntropyProvider {
 public:
  // Creates a SHA1EntropyProvider with the given |entropy_source|, which
  // should contain a large amount of entropy - for example, a textual
  // representation of a persistent randomly-generated 128-bit value.
  explicit SHA1EntropyProvider(const std::string& entropy_source);
  ~SHA1EntropyProvider() override;

  // base::FieldTrial::EntropyProvider implementation:
  double GetEntropyForTrial(const std::string& trial_name,
                            uint32_t randomization_seed) const override;

 private:
  std::string entropy_source_;

  DISALLOW_COPY_AND_ASSIGN(SHA1EntropyProvider);
};

// PermutedEntropyProvider is an entropy provider suitable for low entropy
// sources (below 16 bits). It uses the field trial name or randomization seed
// to generate a permutation of a mapping array from an initial entropy value to
// a new value.  Note: This provider's performance is O(2^n), where n is the
// number of bits in the entropy source.
class PermutedEntropyProvider : public base::FieldTrial::EntropyProvider {
 public:
  // Creates a PermutedEntropyProvider with the given |low_entropy_source|,
  // which should have a value in the range of [0, low_entropy_source_max).
  PermutedEntropyProvider(uint16_t low_entropy_source,
                          size_t low_entropy_source_max);
  ~PermutedEntropyProvider() override;

  // base::FieldTrial::EntropyProvider implementation:
  double GetEntropyForTrial(const std::string& trial_name,
                            uint32_t randomization_seed) const override;

 protected:
  // Performs the permutation algorithm and returns the permuted value that
  // corresponds to |low_entropy_source_|.
  virtual uint16_t GetPermutedValue(uint32_t randomization_seed) const;

 private:
  uint16_t low_entropy_source_;
  size_t low_entropy_source_max_;

  DISALLOW_COPY_AND_ASSIGN(PermutedEntropyProvider);
};

}  // namespace variations

#endif  // COMPONENTS_VARIATIONS_ENTROPY_PROVIDER_H_
