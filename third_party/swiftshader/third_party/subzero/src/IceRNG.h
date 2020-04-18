//===- subzero/src/IceRNG.h - Random number generator -----------*- C++ -*-===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Declares a random number generator.
///
//===----------------------------------------------------------------------===//

#ifndef SUBZERO_SRC_ICERNG_H
#define SUBZERO_SRC_ICERNG_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Compiler.h"
#include "IceDefs.h"

#include <cstdint>

namespace Ice {

class RandomNumberGenerator {
  RandomNumberGenerator() = delete;
  RandomNumberGenerator(const RandomNumberGenerator &) = delete;
  RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;

public:
  explicit RandomNumberGenerator(uint64_t Seed, llvm::StringRef Salt = "");
  /// Create a random number generator with: global seed, randomization pass ID
  /// and a salt uint64_t integer.
  /// @param Seed should be a global seed.
  /// @param RandomizationPassID should be one of RandomizationPassesEnum.
  /// @param Salt should be an additional integer input for generating unique
  /// RNG.
  /// The global seed is 64 bits; since it is likely to originate from the
  /// system time, the lower bits are more "valuable" than the upper bits. As
  /// such, we merge the randomization pass ID and the salt into the global seed
  /// by xor'ing them into high bit ranges. We expect the pass ID to fit within
  /// 4 bits, so it gets shifted by 60 to merge into the upper 4 bits. We expect
  /// the salt (usually the function sequence number) to fit within 12 bits, so
  /// it gets shifted by 48 before merging.
  explicit RandomNumberGenerator(uint64_t Seed,
                                 RandomizationPassesEnum RandomizationPassID,
                                 uint64_t Salt = 0);
  uint64_t next(uint64_t Max);

private:
  uint64_t State;
};

/// This class adds additional random number generator utilities. The reason for
/// the wrapper class is that we want to keep the RandomNumberGenerator
/// interface identical to LLVM's.
class RandomNumberGeneratorWrapper {
  RandomNumberGeneratorWrapper() = delete;
  RandomNumberGeneratorWrapper(const RandomNumberGeneratorWrapper &) = delete;
  RandomNumberGeneratorWrapper &
  operator=(const RandomNumberGeneratorWrapper &) = delete;

public:
  uint64_t operator()(uint64_t Max) { return RNG.next(Max); }
  bool getTrueWithProbability(float Probability);
  explicit RandomNumberGeneratorWrapper(RandomNumberGenerator &RNG)
      : RNG(RNG) {}

private:
  RandomNumberGenerator &RNG;
};

/// RandomShuffle is an implementation of std::random_shuffle() that doesn't
/// change across stdlib implementations. Adapted from a sample implementation
/// at cppreference.com.
template <class RandomIt, class RandomFunc>
void RandomShuffle(RandomIt First, RandomIt Last, RandomFunc &&RNG) {
  for (auto i = Last - First - 1; i > 0; --i)
    std::swap(First[i], First[RNG(i + 1)]);
}

} // end of namespace Ice

#endif // SUBZERO_SRC_ICERNG_H
