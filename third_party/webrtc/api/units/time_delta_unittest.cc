/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/units/time_delta.h"

#include "test/gtest.h"

namespace webrtc {
namespace test {

TEST(TimeDeltaTest, GetBackSameValues) {
  const int64_t kValue = 499;
  for (int sign = -1; sign <= 1; ++sign) {
    int64_t value = kValue * sign;
    EXPECT_EQ(TimeDelta::ms(value).ms(), value);
    EXPECT_EQ(TimeDelta::us(value).us(), value);
    EXPECT_EQ(TimeDelta::seconds(value).seconds(), value);
    EXPECT_EQ(TimeDelta::seconds(value).seconds(), value);
  }
  EXPECT_EQ(TimeDelta::Zero().us(), 0);
}

TEST(TimeDeltaTest, GetDifferentPrefix) {
  const int64_t kValue = 3000000;
  EXPECT_EQ(TimeDelta::us(kValue).seconds(), kValue / 1000000);
  EXPECT_EQ(TimeDelta::ms(kValue).seconds(), kValue / 1000);
  EXPECT_EQ(TimeDelta::us(kValue).ms(), kValue / 1000);

  EXPECT_EQ(TimeDelta::ms(kValue).us(), kValue * 1000);
  EXPECT_EQ(TimeDelta::seconds(kValue).ms(), kValue * 1000);
  EXPECT_EQ(TimeDelta::seconds(kValue).us(), kValue * 1000000);
}

TEST(TimeDeltaTest, IdentityChecks) {
  const int64_t kValue = 3000;
  EXPECT_TRUE(TimeDelta::Zero().IsZero());
  EXPECT_FALSE(TimeDelta::ms(kValue).IsZero());

  EXPECT_TRUE(TimeDelta::PlusInfinity().IsInfinite());
  EXPECT_TRUE(TimeDelta::MinusInfinity().IsInfinite());
  EXPECT_FALSE(TimeDelta::Zero().IsInfinite());
  EXPECT_FALSE(TimeDelta::ms(-kValue).IsInfinite());
  EXPECT_FALSE(TimeDelta::ms(kValue).IsInfinite());

  EXPECT_FALSE(TimeDelta::PlusInfinity().IsFinite());
  EXPECT_FALSE(TimeDelta::MinusInfinity().IsFinite());
  EXPECT_TRUE(TimeDelta::ms(-kValue).IsFinite());
  EXPECT_TRUE(TimeDelta::ms(kValue).IsFinite());
  EXPECT_TRUE(TimeDelta::Zero().IsFinite());
}

TEST(TimeDeltaTest, ComparisonOperators) {
  const int64_t kSmall = 450;
  const int64_t kLarge = 451;
  const TimeDelta small = TimeDelta::ms(kSmall);
  const TimeDelta large = TimeDelta::ms(kLarge);

  EXPECT_EQ(TimeDelta::Zero(), TimeDelta::ms(0));
  EXPECT_EQ(TimeDelta::PlusInfinity(), TimeDelta::PlusInfinity());
  EXPECT_EQ(small, TimeDelta::ms(kSmall));
  EXPECT_LE(small, TimeDelta::ms(kSmall));
  EXPECT_GE(small, TimeDelta::ms(kSmall));
  EXPECT_NE(small, TimeDelta::ms(kLarge));
  EXPECT_LE(small, TimeDelta::ms(kLarge));
  EXPECT_LT(small, TimeDelta::ms(kLarge));
  EXPECT_GE(large, TimeDelta::ms(kSmall));
  EXPECT_GT(large, TimeDelta::ms(kSmall));
  EXPECT_LT(TimeDelta::Zero(), small);
  EXPECT_GT(TimeDelta::Zero(), TimeDelta::ms(-kSmall));
  EXPECT_GT(TimeDelta::Zero(), TimeDelta::ms(-kSmall));

  EXPECT_GT(TimeDelta::PlusInfinity(), large);
  EXPECT_LT(TimeDelta::MinusInfinity(), TimeDelta::Zero());
}

TEST(TimeDeltaTest, MathOperations) {
  const int64_t kValueA = 267;
  const int64_t kValueB = 450;
  const TimeDelta delta_a = TimeDelta::ms(kValueA);
  const TimeDelta delta_b = TimeDelta::ms(kValueB);
  EXPECT_EQ((delta_a + delta_b).ms(), kValueA + kValueB);
  EXPECT_EQ((delta_a - delta_b).ms(), kValueA - kValueB);

  const int32_t kInt32Value = 123;
  const double kFloatValue = 123.0;
  EXPECT_EQ((TimeDelta::us(kValueA) * kValueB).us(), kValueA * kValueB);
  EXPECT_EQ((TimeDelta::us(kValueA) * kInt32Value).us(), kValueA * kInt32Value);
  EXPECT_EQ((TimeDelta::us(kValueA) * kFloatValue).us(), kValueA * kFloatValue);

  EXPECT_EQ(TimeDelta::us(-kValueA).Abs().us(), kValueA);
  EXPECT_EQ(TimeDelta::us(kValueA).Abs().us(), kValueA);
}
}  // namespace test
}  // namespace webrtc
