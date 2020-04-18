/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/units/timestamp.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {
TEST(TimestampTest, GetBackSameValues) {
  const int64_t kValue = 499;
  EXPECT_EQ(Timestamp::ms(kValue).ms(), kValue);
  EXPECT_EQ(Timestamp::us(kValue).us(), kValue);
  EXPECT_EQ(Timestamp::seconds(kValue).seconds(), kValue);
}

TEST(TimestampTest, GetDifferentPrefix) {
  const int64_t kValue = 3000000;
  EXPECT_EQ(Timestamp::us(kValue).seconds(), kValue / 1000000);
  EXPECT_EQ(Timestamp::ms(kValue).seconds(), kValue / 1000);
  EXPECT_EQ(Timestamp::us(kValue).ms(), kValue / 1000);

  EXPECT_EQ(Timestamp::ms(kValue).us(), kValue * 1000);
  EXPECT_EQ(Timestamp::seconds(kValue).ms(), kValue * 1000);
  EXPECT_EQ(Timestamp::seconds(kValue).us(), kValue * 1000000);
}

TEST(TimestampTest, IdentityChecks) {
  const int64_t kValue = 3000;

  EXPECT_TRUE(Timestamp::Infinity().IsInfinite());
  EXPECT_FALSE(Timestamp::ms(kValue).IsInfinite());

  EXPECT_FALSE(Timestamp::Infinity().IsFinite());
  EXPECT_TRUE(Timestamp::ms(kValue).IsFinite());
}

TEST(TimestampTest, ComparisonOperators) {
  const int64_t kSmall = 450;
  const int64_t kLarge = 451;

  EXPECT_EQ(Timestamp::Infinity(), Timestamp::Infinity());
  EXPECT_EQ(Timestamp::ms(kSmall), Timestamp::ms(kSmall));
  EXPECT_LE(Timestamp::ms(kSmall), Timestamp::ms(kSmall));
  EXPECT_GE(Timestamp::ms(kSmall), Timestamp::ms(kSmall));
  EXPECT_NE(Timestamp::ms(kSmall), Timestamp::ms(kLarge));
  EXPECT_LE(Timestamp::ms(kSmall), Timestamp::ms(kLarge));
  EXPECT_LT(Timestamp::ms(kSmall), Timestamp::ms(kLarge));
  EXPECT_GE(Timestamp::ms(kLarge), Timestamp::ms(kSmall));
  EXPECT_GT(Timestamp::ms(kLarge), Timestamp::ms(kSmall));
}

TEST(UnitConversionTest, TimestampAndTimeDeltaMath) {
  const int64_t kValueA = 267;
  const int64_t kValueB = 450;
  const Timestamp time_a = Timestamp::ms(kValueA);
  const Timestamp time_b = Timestamp::ms(kValueB);
  const TimeDelta delta_a = TimeDelta::ms(kValueA);

  EXPECT_EQ((time_a - time_b), TimeDelta::ms(kValueA - kValueB));
  EXPECT_EQ((time_b - delta_a), Timestamp::ms(kValueB - kValueA));
  EXPECT_EQ((time_b + delta_a), Timestamp::ms(kValueB + kValueA));
}
}  // namespace test
}  // namespace webrtc
