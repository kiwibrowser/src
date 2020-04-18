//===- subzero/src/IceRNG.cpp - PRNG implementation -----------------------===//
//
//                        The Subzero Code Generator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Implements the random number generator.
///
//===----------------------------------------------------------------------===//

#include "IceRNG.h"

#include <climits>
#include <ctime>

namespace Ice {

namespace {
constexpr unsigned MAX = 2147483647;
} // end of anonymous namespace

// TODO(wala,stichnot): Switch to RNG implementation from LLVM or C++11.
//
// TODO(wala,stichnot): Make it possible to replay the RNG sequence in a
// subsequent run, for reproducing a bug. Print the seed in a comment in the
// asm output. Embed the seed in the binary via metadata that an attacker can't
// introspect.
RandomNumberGenerator::RandomNumberGenerator(uint64_t Seed, llvm::StringRef)
    : State(Seed) {}

RandomNumberGenerator::RandomNumberGenerator(
    uint64_t Seed, RandomizationPassesEnum RandomizationPassID, uint64_t Salt) {
  constexpr unsigned NumBitsGlobalSeed = CHAR_BIT * sizeof(State);
  constexpr unsigned NumBitsPassID = 4;
  constexpr unsigned NumBitsSalt = 12;
  static_assert(RPE_num < (1 << NumBitsPassID), "NumBitsPassID too small");
  State = Seed ^ ((uint64_t)RandomizationPassID
                  << (NumBitsGlobalSeed - NumBitsPassID)) ^
          (Salt << (NumBitsGlobalSeed - NumBitsPassID - NumBitsSalt));
}
uint64_t RandomNumberGenerator::next(uint64_t Max) {
  // Lewis, Goodman, and Miller (1969)
  State = (16807 * State) % MAX;
  return State % Max;
}

bool RandomNumberGeneratorWrapper::getTrueWithProbability(float Probability) {
  return RNG.next(MAX) < Probability * MAX;
}

} // end of namespace Ice
