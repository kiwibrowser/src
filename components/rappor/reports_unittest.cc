// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/reports.h"

#include <stdlib.h>

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace rappor {

const NoiseParameters kTestNoiseParameters = {
    PROBABILITY_75 /* Fake data probability */,
    PROBABILITY_50 /* Fake one probability */,
    PROBABILITY_75 /* One coin probability */,
    PROBABILITY_50 /* Zero coin probability */,
};

TEST(RapporMetricTest, GetReportStatistics) {
  ByteVector real_bits(50);
  // Set 152 bits (19 bytes)
  for (char i = 0; i < 19; i++) {
    real_bits[i] = 0xff;
  }

  const int real_bit_count = CountBits(real_bits);
  EXPECT_EQ(real_bit_count, 152);

  const std::string secret = HmacByteVectorGenerator::GenerateEntropyInput();
  const ByteVector report =
      internal::GenerateReport(secret, kTestNoiseParameters, real_bits);

  // For the bits we actually set in the Bloom filter, get a count of how
  // many of them reported true.
  ByteVector from_true_reports = report;
  // Real bits AND report bits.
  ByteVectorMerge(real_bits, real_bits, &from_true_reports);
  const int true_from_true_count = CountBits(from_true_reports);

  // For the bits we didn't set in the Bloom filter, get a count of how
  // many of them reported true.
  ByteVector from_false_reports = report;
  ByteVectorOr(real_bits, &from_false_reports);
  const int true_from_false_count =
      CountBits(from_false_reports) - real_bit_count;

  // The probability of a true bit being true after redaction =
  //   [fake_prob]*[fake_true_prob] + (1-[fake_prob]) =
  //   .75 * .5 + (1-.75) = .625
  // The probablity of a false bit being true after redaction =
  //   [fake_prob]*[fake_true_prob] = .375
  // The probability of a bit reporting true =
  //   [redacted_prob] * [one_coin_prob:.75] +
  //      (1-[redacted_prob]) * [zero_coin_prob:.5] =
  //   0.65625 for true bits
  //   0.59375 for false bits

  // stats.binom(152, 0.65625).ppf(0.000005) = 73
  EXPECT_GT(true_from_true_count, 73);
  // stats.binom(152, 0.65625).ppf(0.999995) = 124
  EXPECT_LE(true_from_true_count, 124);

  // stats.binom(248, 0.59375).ppf(.000005) = 113
  EXPECT_GT(true_from_false_count, 113);
  // stats.binom(248, 0.59375).ppf(.999995) = 181
  EXPECT_LE(true_from_false_count, 181);
}

}  // namespace rappor
