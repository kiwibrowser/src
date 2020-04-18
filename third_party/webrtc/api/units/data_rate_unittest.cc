/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/units/data_rate.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {
TEST(DataRateTest, GetBackSameValues) {
  const int64_t kValue = 123 * 8;
  EXPECT_EQ(DataRate::bits_per_second(kValue).bits_per_second(), kValue);
  EXPECT_EQ(DataRate::bps(kValue).bps(), kValue);
  EXPECT_EQ(DataRate::kbps(kValue).kbps(), kValue);
}

TEST(DataRateTest, GetDifferentPrefix) {
  const int64_t kValue = 123 * 8000;
  EXPECT_EQ(DataRate::bps(kValue).kbps(), kValue / 1000);
}

TEST(DataRateTest, IdentityChecks) {
  const int64_t kValue = 3000;
  EXPECT_TRUE(DataRate::Zero().IsZero());
  EXPECT_FALSE(DataRate::bits_per_second(kValue).IsZero());

  EXPECT_TRUE(DataRate::Infinity().IsInfinite());
  EXPECT_FALSE(DataRate::Zero().IsInfinite());
  EXPECT_FALSE(DataRate::bits_per_second(kValue).IsInfinite());

  EXPECT_FALSE(DataRate::Infinity().IsFinite());
  EXPECT_TRUE(DataRate::bits_per_second(kValue).IsFinite());
  EXPECT_TRUE(DataRate::Zero().IsFinite());
}

TEST(DataRateTest, ComparisonOperators) {
  const int64_t kSmall = 450;
  const int64_t kLarge = 451;
  const DataRate small = DataRate::bits_per_second(kSmall);
  const DataRate large = DataRate::bits_per_second(kLarge);

  EXPECT_EQ(DataRate::Zero(), DataRate::bps(0));
  EXPECT_EQ(DataRate::Infinity(), DataRate::Infinity());
  EXPECT_EQ(small, small);
  EXPECT_LE(small, small);
  EXPECT_GE(small, small);
  EXPECT_NE(small, large);
  EXPECT_LE(small, large);
  EXPECT_LT(small, large);
  EXPECT_GE(large, small);
  EXPECT_GT(large, small);
  EXPECT_LT(DataRate::Zero(), small);
  EXPECT_GT(DataRate::Infinity(), large);
}

TEST(DataRateTest, MathOperations) {
  const int64_t kValueA = 450;
  const int64_t kValueB = 267;
  const DataRate size_a = DataRate::bits_per_second(kValueA);
  const int32_t kInt32Value = 123;
  const double kFloatValue = 123.0;
  EXPECT_EQ((size_a * kValueB).bits_per_second(), kValueA * kValueB);
  EXPECT_EQ((size_a * kInt32Value).bits_per_second(), kValueA * kInt32Value);
  EXPECT_EQ((size_a * kFloatValue).bits_per_second(), kValueA * kFloatValue);
}

TEST(UnitConversionTest, DataRateAndDataSizeAndTimeDelta) {
  const int64_t kSeconds = 5;
  const int64_t kBitsPerSecond = 440;
  const int64_t kBytes = 44000;
  const TimeDelta delta_a = TimeDelta::seconds(kSeconds);
  const DataRate rate_b = DataRate::bits_per_second(kBitsPerSecond);
  const DataSize size_c = DataSize::bytes(kBytes);
  EXPECT_EQ((delta_a * rate_b).bytes(), kSeconds * kBitsPerSecond / 8);
  EXPECT_EQ((rate_b * delta_a).bytes(), kSeconds * kBitsPerSecond / 8);
  EXPECT_EQ((size_c / delta_a).bits_per_second(), kBytes * 8 / kSeconds);
  EXPECT_EQ((size_c / rate_b).seconds(), kBytes * 8 / kBitsPerSecond);
}

#if GTEST_HAS_DEATH_TEST && !defined(WEBRTC_ANDROID)
TEST(UnitConversionTest, DivisionByZeroFails) {
  const DataSize non_zero_size = DataSize::bytes(100);
  const DataSize zero_size = DataSize::Zero();
  const DataRate zero_rate = DataRate::Zero();
  const TimeDelta zero_delta = TimeDelta::Zero();

  EXPECT_DEATH(non_zero_size / zero_rate, "");
  EXPECT_DEATH(non_zero_size / zero_delta, "");
  EXPECT_DEATH(zero_size / zero_rate, "");
  EXPECT_DEATH(zero_size / zero_delta, "");
}

TEST(UnitConversionTest, DivisionFailsOnLargeSize) {
  // Note that the failure is expected since the current implementation  is
  // implementated in a way that does not support division of large sizes. If
  // the implementation is changed, this test can safely be removed.
  const int64_t kJustSmallEnoughForDivision =
      std::numeric_limits<int64_t>::max() / 8000000;
  const int64_t kToolargeForDivision = kJustSmallEnoughForDivision + 1;
  const DataSize large_size = DataSize::bytes(kJustSmallEnoughForDivision);
  const DataSize too_large_size = DataSize::bytes(kToolargeForDivision);
  const DataRate data_rate = DataRate::kbps(100);
  const TimeDelta time_delta = TimeDelta::ms(100);
  EXPECT_TRUE((large_size / data_rate).IsFinite());
  EXPECT_TRUE((large_size / time_delta).IsFinite());

  EXPECT_DEATH(too_large_size / data_rate, "");
  EXPECT_DEATH(too_large_size / time_delta, "");
}
#endif  // GTEST_HAS_DEATH_TEST && !!defined(WEBRTC_ANDROID)
}  // namespace test
}  // namespace webrtc
