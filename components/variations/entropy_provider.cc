// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/entropy_provider.h"

#include <algorithm>
#include <limits>
#include <vector>

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_byteorder.h"
#include "components/variations/hashing.h"

namespace variations {

namespace internal {

SeededRandGenerator::SeededRandGenerator(uint32_t seed)
    : mersenne_twister_(seed) {}

SeededRandGenerator::~SeededRandGenerator() {
}

uint32_t SeededRandGenerator::operator()(uint32_t range) {
  // Based on base::RandGenerator().
  DCHECK_GT(range, 0u);

  // We must discard random results above this number, as they would
  // make the random generator non-uniform (consider e.g. if
  // MAX_UINT64 was 7 and |range| was 5, then a result of 1 would be twice
  // as likely as a result of 3 or 4).
  uint32_t max_acceptable_value =
      (std::numeric_limits<uint32_t>::max() / range) * range - 1;

  uint32_t value;
  do {
    value = mersenne_twister_();
  } while (value > max_acceptable_value);

  return value % range;
}

void PermuteMappingUsingRandomizationSeed(uint32_t randomization_seed,
                                          std::vector<uint16_t>* mapping) {
  for (size_t i = 0; i < mapping->size(); ++i)
    (*mapping)[i] = static_cast<uint16_t>(i);

  SeededRandGenerator generator(randomization_seed);

  // Do a deterministic random shuffle of the mapping using |generator|.
  //
  // Note: This logic is identical to the following call with libstdc++ and VS:
  //
  //   std::random_shuffle(mapping->begin(), mapping->end(), generator);
  //
  // However, this is not guaranteed by the spec and some implementations (e.g.
  // libc++) use a different algorithm. To ensure results are consistent
  // regardless of the compiler toolchain used, use our own version.
  for (size_t i = 1; i < mapping->size(); ++i) {
    // Pick an element in mapping[:i+1] with which to exchange mapping[i].
    size_t j = generator(i + 1);
    std::swap((*mapping)[i], (*mapping)[j]);
  }
}

}  // namespace internal

SHA1EntropyProvider::SHA1EntropyProvider(const std::string& entropy_source)
    : entropy_source_(entropy_source) {
}

SHA1EntropyProvider::~SHA1EntropyProvider() {
}

double SHA1EntropyProvider::GetEntropyForTrial(
    const std::string& trial_name,
    uint32_t randomization_seed) const {
  // Given enough input entropy, SHA-1 will produce a uniformly random spread
  // in its output space. In this case, the input entropy that is used is the
  // combination of the original |entropy_source_| and the |trial_name|.
  //
  // Note: If |entropy_source_| has very low entropy, such as 13 bits or less,
  // it has been observed that this method does not result in a uniform
  // distribution given the same |trial_name|. When using such a low entropy
  // source, PermutedEntropyProvider should be used instead.
  std::string input(entropy_source_);
  input.append(randomization_seed == 0 ? trial_name : base::UintToString(
                                                          randomization_seed));

  unsigned char sha1_hash[base::kSHA1Length];
  base::SHA1HashBytes(reinterpret_cast<const unsigned char*>(input.c_str()),
                      input.size(),
                      sha1_hash);

  uint64_t bits;
  static_assert(sizeof(bits) < sizeof(sha1_hash), "more data required");
  memcpy(&bits, sha1_hash, sizeof(bits));
  bits = base::ByteSwapToLE64(bits);

  return base::BitsToOpenEndedUnitInterval(bits);
}

PermutedEntropyProvider::PermutedEntropyProvider(uint16_t low_entropy_source,
                                                 size_t low_entropy_source_max)
    : low_entropy_source_(low_entropy_source),
      low_entropy_source_max_(low_entropy_source_max) {
  DCHECK_LT(low_entropy_source, low_entropy_source_max);
  DCHECK_LE(low_entropy_source_max, std::numeric_limits<uint16_t>::max());
}

PermutedEntropyProvider::~PermutedEntropyProvider() {
}

double PermutedEntropyProvider::GetEntropyForTrial(
    const std::string& trial_name,
    uint32_t randomization_seed) const {
  if (randomization_seed == 0)
    randomization_seed = HashName(trial_name);

  return GetPermutedValue(randomization_seed) /
         static_cast<double>(low_entropy_source_max_);
}

uint16_t PermutedEntropyProvider::GetPermutedValue(
    uint32_t randomization_seed) const {
  std::vector<uint16_t> mapping(low_entropy_source_max_);
  internal::PermuteMappingUsingRandomizationSeed(randomization_seed, &mapping);
  return mapping[low_entropy_source_];
}

}  // namespace variations
