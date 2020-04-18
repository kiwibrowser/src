// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/byte_vector_utils.h"

#include <algorithm>
#include <string>

#include "base/logging.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/random.h"

namespace rappor {

namespace {

// Reinterpets a ByteVector as a StringPiece.
base::StringPiece ByteVectorAsStringPiece(const ByteVector& lhs) {
  return base::StringPiece(reinterpret_cast<const char *>(&lhs[0]), lhs.size());
}

// Concatenates parameters together as a string.
std::string Concat(const ByteVector& value, char c, base::StringPiece data) {
  std::string result(value.begin(), value.end());
  result += c;
  data.AppendToString(&result);
  return result;
}

// Performs the operation: K = HMAC(K, data)
// The input "K" is passed by initializing |hmac| with it.
// The output "K" is returned by initializing |result| with it.
// Returns false on an error.
bool HMAC_Rotate(const crypto::HMAC& hmac,
                 const std::string& data,
                 crypto::HMAC* result) {
  ByteVector key(hmac.DigestLength());
  if (!hmac.Sign(data, &key[0], key.size()))
    return false;
  return result->Init(ByteVectorAsStringPiece(key));
}

// Performs the operation: V = HMAC(K, V)
// The input "K" is passed by initializing |hmac| with it.
// "V" is read from and written to |value|.
// Returns false on an error.
bool HMAC_Rehash(const crypto::HMAC& hmac, ByteVector* value) {
  return hmac.Sign(ByteVectorAsStringPiece(*value),
                   &(*value)[0], value->size());
}

// Implements (Key, V) = HMAC_DRBG_Update(provided_data, Key, V)
// See: http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
// "V" is read from and written to |value|.
// The input "Key" is passed by initializing |hmac1| with it.
// The output "Key" is returned by initializing |out_hmac| with it.
// Returns false on an error.
bool HMAC_DRBG_Update(base::StringPiece provided_data,
                      const crypto::HMAC& hmac1,
                      ByteVector* value,
                      crypto::HMAC* out_hmac) {
  // HMAC_DRBG Update Process
  crypto::HMAC temp_hmac(crypto::HMAC::SHA256);
  crypto::HMAC* hmac2 = provided_data.size() > 0 ? &temp_hmac : out_hmac;
  // 1. K = HMAC(K, V || 0x00 || provided_data)
  if (!HMAC_Rotate(hmac1, Concat(*value, 0x00, provided_data), hmac2))
    return false;
  // 2. V = HMAC(K, V)
  if (!HMAC_Rehash(*hmac2, value))
    return false;
  // 3. If (provided_data = Null), then return K and V.
  if (hmac2 == out_hmac)
    return true;
  // 4. K = HMAC(K, V || 0x01 || provided_data)
  if (!HMAC_Rotate(*hmac2, Concat(*value, 0x01, provided_data), out_hmac))
    return false;
  // 5. V = HMAC(K, V)
  return HMAC_Rehash(*out_hmac, value);
}

}  // namespace

void Uint64ToByteVector(uint64_t value, size_t size, ByteVector* output) {
  DCHECK_LE(size, 8u);
  DCHECK_EQ(size, output->size());
  for (size_t i = 0; i < size; i++) {
    // Get the value of the i-th smallest byte and copy it to the byte vector.
    uint64_t shift = i * 8;
    uint64_t byte_mask = static_cast<uint64_t>(0xff) << shift;
    (*output)[i] = (value & byte_mask) >> shift;
  }
}

ByteVector* ByteVectorAnd(const ByteVector& lhs, ByteVector* rhs) {
  DCHECK_EQ(lhs.size(), rhs->size());
  for (size_t i = 0; i < lhs.size(); ++i) {
    (*rhs)[i] = lhs[i] & (*rhs)[i];
  }
  return rhs;
}

ByteVector* ByteVectorOr(const ByteVector& lhs, ByteVector* rhs) {
  DCHECK_EQ(lhs.size(), rhs->size());
  for (size_t i = 0; i < lhs.size(); ++i) {
    (*rhs)[i] = lhs[i] | (*rhs)[i];
  }
  return rhs;
}

ByteVector* ByteVectorMerge(const ByteVector& mask,
                            const ByteVector& lhs,
                            ByteVector* rhs) {
  DCHECK_EQ(lhs.size(), rhs->size());
  for (size_t i = 0; i < lhs.size(); ++i) {
    (*rhs)[i] = (lhs[i] & ~mask[i]) | ((*rhs)[i] & mask[i]);
  }
  return rhs;
}

int CountBits(const ByteVector& vector) {
  int bit_count = 0;
  for (size_t i = 0; i < vector.size(); ++i) {
    uint8_t byte = vector[i];
    for (int j = 0; j < 8 ; ++j) {
      if (byte & (1 << j))
        bit_count++;
    }
  }
  return bit_count;
}

ByteVectorGenerator::ByteVectorGenerator(size_t byte_count)
    : byte_count_(byte_count) {}

ByteVectorGenerator::~ByteVectorGenerator() {}

ByteVector ByteVectorGenerator::GetRandomByteVector() {
  ByteVector bytes(byte_count_);
  crypto::RandBytes(&bytes[0], bytes.size());
  return bytes;
}

ByteVector ByteVectorGenerator::GetWeightedRandomByteVector(
    Probability probability) {
  switch (probability) {
    case PROBABILITY_100:
      return ByteVector(byte_count_, 0xff);
    case PROBABILITY_75: {
      ByteVector bytes = GetRandomByteVector();
      return *ByteVectorOr(GetRandomByteVector(), &bytes);
    }
    case PROBABILITY_50:
      return GetRandomByteVector();
    case PROBABILITY_25: {
      ByteVector bytes = GetRandomByteVector();
      return *ByteVectorAnd(GetRandomByteVector(), &bytes);
    }
    case PROBABILITY_0:
      return ByteVector(byte_count_);
  }
  NOTREACHED();
  return ByteVector(byte_count_);
}

HmacByteVectorGenerator::HmacByteVectorGenerator(
    size_t byte_count,
    const std::string& entropy_input,
    base::StringPiece personalization_string)
    : ByteVectorGenerator(byte_count),
      hmac_(crypto::HMAC::SHA256),
      value_(hmac_.DigestLength(), 0x01),
      generated_bytes_(0) {
  // HMAC_DRBG Instantiate Process
  // See: http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
  // 1. seed_material = entropy_input + nonce + personalization_string
  // Note: We are using the 8.6.7 interpretation, where the entropy_input and
  // nonce are acquired at the same time from the same source.
  DCHECK_EQ(kEntropyInputSize, entropy_input.size());
  std::string seed_material(entropy_input);
  personalization_string.AppendToString(&seed_material);
  // 2. Key = 0x00 00...00
  crypto::HMAC hmac1(crypto::HMAC::SHA256);
  if (!hmac1.Init(std::string(hmac_.DigestLength(), 0x00)))
    NOTREACHED();
  // 3. V = 0x01 01...01
  // (value_ in initializer list)

  // 4. (Key, V) = HMAC_DRBG_Update(seed_material, Key, V)
  if (!HMAC_DRBG_Update(seed_material, hmac1, &value_, &hmac_))
    NOTREACHED();
}

HmacByteVectorGenerator::~HmacByteVectorGenerator() {}

HmacByteVectorGenerator::HmacByteVectorGenerator(
    const HmacByteVectorGenerator& prev_request)
    : ByteVectorGenerator(prev_request.byte_count()),
      hmac_(crypto::HMAC::SHA256),
      value_(prev_request.value_),
      generated_bytes_(0) {
  if (!HMAC_DRBG_Update("", prev_request.hmac_, &value_, &hmac_))
    NOTREACHED();
}

// HMAC_DRBG requires entropy input to be security_strength bits long,
// and nonce to be at least 1/2 security_strength bits long.  We
// generate them both as a single "extra strong" entropy input.
// max_security_strength for SHA256 is 256 bits.
// See: http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
const size_t HmacByteVectorGenerator::kEntropyInputSize = (256 / 8) * 3 / 2;

// static
std::string HmacByteVectorGenerator::GenerateEntropyInput() {
  return base::RandBytesAsString(kEntropyInputSize);
}

ByteVector HmacByteVectorGenerator::GetRandomByteVector() {
  // Streams bytes from HMAC_DRBG_Generate
  // See: http://csrc.nist.gov/publications/nistpubs/800-90A/SP800-90A.pdf
  const size_t digest_length = hmac_.DigestLength();
  DCHECK_EQ(value_.size(), digest_length);
  ByteVector bytes(byte_count());
  uint8_t* data = &bytes[0];
  size_t bytes_to_go = byte_count();
  while (bytes_to_go > 0) {
    size_t requested_byte_in_digest = generated_bytes_ % digest_length;
    if (requested_byte_in_digest == 0) {
      // Do step 4.1 of the HMAC_DRBG Generate Process for more bits.
      // V = HMAC(Key, V)
      if (!HMAC_Rehash(hmac_, &value_))
        NOTREACHED();
    }
    size_t n = std::min(bytes_to_go,
                        digest_length - requested_byte_in_digest);
    memcpy(data, &value_[requested_byte_in_digest], n);
    data += n;
    bytes_to_go -= n;
    generated_bytes_ += n;
    // Check max_number_of_bits_per_request from 10.1 Table 2
    // max_number_of_bits_per_request == 2^19 bits == 2^16 bytes
    DCHECK_LT(generated_bytes_, 1U << 16);
  }
  return bytes;
}

}  // namespace rappor
