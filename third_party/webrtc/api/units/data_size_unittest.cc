/*
 *  Copyright (c) 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "api/units/data_size.h"
#include "test/gtest.h"

namespace webrtc {
namespace test {

TEST(DataSizeTest, GetBackSameValues) {
  const int64_t kValue = 123 * 8;
  EXPECT_EQ(DataSize::bytes(kValue).bytes(), kValue);
}

TEST(DataSizeTest, GetDifferentPrefix) {
  const int64_t kValue = 123 * 8000;
  EXPECT_EQ(DataSize::bytes(kValue).kilobytes(), kValue / 1000);
}

TEST(DataSizeTest, IdentityChecks) {
  const int64_t kValue = 3000;
  EXPECT_TRUE(DataSize::Zero().IsZero());
  EXPECT_FALSE(DataSize::bytes(kValue).IsZero());

  EXPECT_TRUE(DataSize::Infinity().IsInfinite());
  EXPECT_FALSE(DataSize::Zero().IsInfinite());
  EXPECT_FALSE(DataSize::bytes(kValue).IsInfinite());

  EXPECT_FALSE(DataSize::Infinity().IsFinite());
  EXPECT_TRUE(DataSize::bytes(kValue).IsFinite());
  EXPECT_TRUE(DataSize::Zero().IsFinite());
}

TEST(DataSizeTest, ComparisonOperators) {
  const int64_t kSmall = 450;
  const int64_t kLarge = 451;
  const DataSize small = DataSize::bytes(kSmall);
  const DataSize large = DataSize::bytes(kLarge);

  EXPECT_EQ(DataSize::Zero(), DataSize::bytes(0));
  EXPECT_EQ(DataSize::Infinity(), DataSize::Infinity());
  EXPECT_EQ(small, small);
  EXPECT_LE(small, small);
  EXPECT_GE(small, small);
  EXPECT_NE(small, large);
  EXPECT_LE(small, large);
  EXPECT_LT(small, large);
  EXPECT_GE(large, small);
  EXPECT_GT(large, small);
  EXPECT_LT(DataSize::Zero(), small);
  EXPECT_GT(DataSize::Infinity(), large);
}

TEST(DataSizeTest, MathOperations) {
  const int64_t kValueA = 450;
  const int64_t kValueB = 267;
  const DataSize size_a = DataSize::bytes(kValueA);
  const DataSize size_b = DataSize::bytes(kValueB);
  EXPECT_EQ((size_a + size_b).bytes(), kValueA + kValueB);
  EXPECT_EQ((size_a - size_b).bytes(), kValueA - kValueB);

  const int32_t kInt32Value = 123;
  const double kFloatValue = 123.0;
  EXPECT_EQ((size_a * kValueB).bytes(), kValueA * kValueB);
  EXPECT_EQ((size_a * kInt32Value).bytes(), kValueA * kInt32Value);
  EXPECT_EQ((size_a * kFloatValue).bytes(), kValueA * kFloatValue);

  EXPECT_EQ((size_a / 10).bytes(), kValueA / 10);

  DataSize mutable_size = DataSize::bytes(kValueA);
  mutable_size += size_b;
  EXPECT_EQ(mutable_size.bytes(), kValueA + kValueB);
  mutable_size -= size_a;
  EXPECT_EQ(mutable_size.bytes(), kValueB);
}
}  // namespace test
}  // namespace webrtc
