// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_RAPPOR_BYTE_VECTOR_UTILS_H_
#define COMPONENTS_RAPPOR_BYTE_VECTOR_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "components/rappor/public/rappor_parameters.h"
#include "crypto/hmac.h"

namespace rappor {

// A vector of 8-bit integers used to store a set of binary bits.
typedef std::vector<uint8_t> ByteVector;

// Converts the lowest |size| bytes of |value| into a ByteVector.
void Uint64ToByteVector(uint64_t value, size_t size, ByteVector* output);

// Computes a bitwise AND of byte vectors and stores the result in rhs.
// Returns rhs for chaining.
ByteVector* ByteVectorAnd(const ByteVector& lhs, ByteVector* rhs);

// Computes a bitwise OR of byte vectors and stores the result in rhs.
// Returns rhs for chaining.
ByteVector* ByteVectorOr(const ByteVector& lhs, ByteVector* rhs);

// Merges the contents of lhs and rhs  vectors according to a mask vector.
// The i-th bit of the result vector will be the i-th bit of either the lhs
// or rhs vector, based on the i-th bit of the mask vector.
// Equivalent to (lhs & ~mask) | (rhs & mask).  Stores the result in rhs.
// Returns rhs for chaining.
ByteVector* ByteVectorMerge(const ByteVector& mask,
                            const ByteVector& lhs,
                            ByteVector* rhs);

// Counts the number of bits set in the byte vector.
int CountBits(const ByteVector& vector);

// A utility object for generating random binary data with different
// likelihood of bits being true, using entropy from crypto::RandBytes().
class ByteVectorGenerator {
 public:
  explicit ByteVectorGenerator(size_t byte_count);

  virtual ~ByteVectorGenerator();

  // Generates a random byte vector where the bits are independent random
  // variables which are true with the given |probability|.
  ByteVector GetWeightedRandomByteVector(Probability probability);

 protected:
  // Size of vectors to be generated.
  size_t byte_count() const { return byte_count_; }

  // Generates a random vector of bytes from a uniform distribution.
  virtual ByteVector GetRandomByteVector();

 private:
  size_t byte_count_;

  DISALLOW_COPY_AND_ASSIGN(ByteVectorGenerator);
};

// A ByteVectorGenerator that uses a pseudo-random function to generate a
// deterministically random bits.  This class only implements a single request
// from HMAC_DRBG and streams up to 2^19 bits from that request.
// Ref: http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
// We're using our own PRNG instead of crypto::RandBytes because we need to
// generate a repeatable sequence of bits from the same seed. Conservatively,
// we're choosing to use HMAC_DRBG here, as it is one of the best studied
// and standardized ways of generating deterministic, unpredictable sequences
// based on a secret seed.
class HmacByteVectorGenerator : public ByteVectorGenerator {
 public:
  // Constructor takes the size of the vector to generate, along with a
  // |entropy_input| and |personalization_string| to seed the pseudo-random
  // number generator.  The string parameters are treated as byte arrays.
  HmacByteVectorGenerator(size_t byte_count,
                          const std::string& entropy_input,
                          base::StringPiece personalization_string);

  ~HmacByteVectorGenerator() override;

  // Generates a random string suitable for passing to the constructor as
  // |entropy_input|.
  static std::string GenerateEntropyInput();

  // Key size required for 128-bit security strength (including nonce).
  static const size_t kEntropyInputSize;

 protected:
  // Generate byte vector generator that streams from the next request instead
  // of the current one.  For testing against NIST test vectors only.
  explicit HmacByteVectorGenerator(const HmacByteVectorGenerator& prev_request);

  // ByteVector implementation:
  ByteVector GetRandomByteVector() override;

 private:
  // HMAC initalized with the value of "Key" HMAC_DRBG_Initialize.
  crypto::HMAC hmac_;

  // The "V" value from HMAC_DRBG.
  ByteVector value_;

  // Total number of bytes streamed from the HMAC_DRBG Generate Process.
  size_t generated_bytes_;

  DISALLOW_ASSIGN(HmacByteVectorGenerator);
};

}  // namespace rappor

#endif  // COMPONENTS_RAPPOR_BYTE_VECTOR_UTILS_H_
