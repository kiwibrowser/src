// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/byte_vector_utils.h"

#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

namespace {

class SecondRequest : public HmacByteVectorGenerator {
 public:
  SecondRequest(const HmacByteVectorGenerator& first_request)
      : HmacByteVectorGenerator(first_request) {}
};

std::string HexToString(const char* hex) {
  ByteVector bv;
  base::HexStringToBytes(hex, &bv);
  return std::string(bv.begin(), bv.end());
}

}  // namespace

TEST(ByteVectorTest, Uint64ToByteVectorSmall) {
  ByteVector bytes(1);
  Uint64ToByteVector(0x10, 1, &bytes);
  EXPECT_EQ(1u, bytes.size());
  EXPECT_EQ(0x10, bytes[0]);
}

TEST(ByteVectorTest, Uint64ToByteVectorLarge) {
  ByteVector bytes(8);
  Uint64ToByteVector(0xfedcba9876543210, 8, &bytes);
  EXPECT_EQ(8u, bytes.size());
  EXPECT_EQ(0x10, bytes[0]);
  EXPECT_EQ(0xfe, bytes[7]);
}

TEST(ByteVectorTest, ByteVectorAnd) {
  ByteVector lhs(2);
  lhs[1] = 0x12;
  ByteVector rhs(2);
  rhs[1] = 0x03;

  EXPECT_EQ(0x02, (*ByteVectorAnd(lhs, &rhs))[1]);
}

TEST(ByteVectorTest, ByteVectorOr) {
  ByteVector lhs(2);
  lhs[1] = 0x12;
  ByteVector rhs(2);
  rhs[1] = 0x03;

  EXPECT_EQ(0x13, (*ByteVectorOr(lhs, &rhs))[1]);
}

TEST(ByteVectorTest, ByteVectorMerge) {
  ByteVector lhs(2);
  lhs[1] = 0x33;
  ByteVector rhs(2);
  rhs[1] = 0x55;
  ByteVector mask(2);
  mask[1] = 0x0f;

  EXPECT_EQ(0x35, (*ByteVectorMerge(mask, lhs, &rhs))[1]);
}

TEST(ByteVectorTest, ByteVectorGenerator) {
  ByteVectorGenerator generator(2u);
  ByteVector random_50 = generator.GetWeightedRandomByteVector(PROBABILITY_50);
  EXPECT_EQ(random_50.size(), 2u);
  ByteVector random_75 = generator.GetWeightedRandomByteVector(PROBABILITY_75);
  EXPECT_EQ(random_75.size(), 2u);
}

TEST(ByteVectorTest, HmacByteVectorGenerator) {
  HmacByteVectorGenerator generator(1u,
      std::string(HmacByteVectorGenerator::kEntropyInputSize, 0x00), "");
  ByteVector random_50 = generator.GetWeightedRandomByteVector(PROBABILITY_50);
  EXPECT_EQ(random_50.size(), 1u);
  EXPECT_EQ(random_50[0], 0x0B);
  ByteVector random_75 = generator.GetWeightedRandomByteVector(PROBABILITY_75);
  EXPECT_EQ(random_75.size(), 1u);
  EXPECT_EQ(random_75[0], 0xdf);
}

TEST(ByteVectorTest, HmacNist) {
  // Test case 0 for SHA-256 HMAC_DRBG no reseed tests from
  // http://csrc.nist.gov/groups/STM/cavp/
  const char entropy[] =
      "ca851911349384bffe89de1cbdc46e6831e44d34a4fb935ee285dd14b71a7488";
  const char nonce[] = "659ba96c601dc69fc902940805ec0ca8";
  const char expected[] =
      "e528e9abf2dece54d47c7e75e5fe302149f817ea9fb4bee6f4199697d04d5b89"
      "d54fbb978a15b5c443c9ec21036d2460b6f73ebad0dc2aba6e624abf07745bc1"
      "07694bb7547bb0995f70de25d6b29e2d3011bb19d27676c07162c8b5ccde0668"
      "961df86803482cb37ed6d5c0bb8d50cf1f50d476aa0458bdaba806f48be9dcb8";

  std::string entropy_input = HexToString(entropy) + HexToString(nonce);
  HmacByteVectorGenerator generator(1024u / 8, entropy_input, "");
  generator.GetWeightedRandomByteVector(PROBABILITY_50);
  SecondRequest generator2(generator);
  ByteVector random_50 = generator2.GetWeightedRandomByteVector(PROBABILITY_50);

  EXPECT_EQ(HexToString(expected),
      std::string(random_50.begin(), random_50.end()));
}

TEST(ByteVectorTest, WeightedRandomStatistics0) {
  ByteVectorGenerator generator(50u);
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_0);
  int bit_count = CountBits(random);
  EXPECT_EQ(bit_count, 0);
}

TEST(ByteVectorTest, WeightedRandomStatistics100) {
  ByteVectorGenerator generator(50u);
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_100);
  int bit_count = CountBits(random);
  EXPECT_EQ(bit_count, 50 * 8);
}

TEST(ByteVectorTest, WeightedRandomStatistics50) {
  ByteVectorGenerator generator(50u);
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_50);
  int bit_count = CountBits(random);
  // Check bounds on bit counts that are true with 99.999% probability.
  EXPECT_GT(bit_count, 155); // Binomial(400, .5) CDF(155) ~= 0.000004
  EXPECT_LE(bit_count, 244); // Binomial(400, .5) CDF(244) ~= 0.999996
}

TEST(ByteVectorTest, WeightedRandomStatistics75) {
  ByteVectorGenerator generator(50u);
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_75);
  int bit_count = CountBits(random);
  // Check bounds on bit counts that are true with 99.999% probability.
  EXPECT_GT(bit_count, 259); // Binomial(400, .75) CDF(259) ~= 0.000003
  EXPECT_LE(bit_count, 337); // Binomial(400, .75) CDF(337) ~= 0.999997
}

TEST(ByteVectorTest, HmacWeightedRandomStatistics50) {
  HmacByteVectorGenerator generator(50u,
      HmacByteVectorGenerator::GenerateEntropyInput(), "");
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_50);
  int bit_count = CountBits(random);
  // Check bounds on bit counts that are true with 99.999% probability.
  EXPECT_GT(bit_count, 155); // Binomial(400, .5) CDF(155) ~= 0.000004
  EXPECT_LE(bit_count, 244); // Binomial(400, .5) CDF(244) ~= 0.999996
}

TEST(ByteVectorTest, HmacWeightedRandomStatistics75) {
  HmacByteVectorGenerator generator(50u,
      HmacByteVectorGenerator::GenerateEntropyInput(), "");
  ByteVector random = generator.GetWeightedRandomByteVector(PROBABILITY_75);
  int bit_count = CountBits(random);
  // Check bounds on bit counts that are true with 99.999% probability.
  EXPECT_GT(bit_count, 259); // Binomial(400, .75) CDF(259) ~= 0.000003
  EXPECT_LE(bit_count, 337); // Binomial(400, .75) CDF(337) ~= 0.999997
}

}  // namespace rappor
