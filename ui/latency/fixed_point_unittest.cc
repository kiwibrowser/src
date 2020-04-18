// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/latency/fixed_point.h"

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace frame_metrics {

// Verify range of a fixed point value stored as a uint32_t has enough range
// for our requirements.
TEST(FrameMetricsFixedPointTest, kFixedPointMultiplier) {
  uint32_t max_fixed = std::numeric_limits<uint32_t>::max();
  double max_float = static_cast<double>(max_fixed) / kFixedPointMultiplier;

  // The maximum time delta between two frames we'd like to support.
  double frame_delta = 64 * base::TimeTicks::kMicrosecondsPerSecond;

  // The minimum frame duration we'd like to support.
  // 1kHz should give us plenty of headroom.
  double frame_duration = base::TimeTicks::kMicrosecondsPerSecond / 1000;

  // Verify the resulting slope is within the range.
  double frame_slope = frame_delta / frame_duration;
  EXPECT_LE(frame_slope, max_float);
}

// Some code will take the square root of a 32-bit value by shifting it left
// 32-bits beforehand. Verify this is okay and more accurate than not shifting
// at all.
TEST(FrameMetricsFixedPointTest, kFixedPointRootMultiplier) {
  uint64_t value = 0xFFFFFFFF;

  // Calculate SMR with kFixedPointRootMultiplier.
  // Truncate to 32 bits to verify multiplying by kFixedPointRootMultiplier
  // will not result in overflow when stored as a 32 bit value.
  uint32_t root1 = std::sqrt(value * kFixedPointRootMultiplier);
  double value1 =
      static_cast<uint64_t>(root1) * root1 / kFixedPointRootMultiplier;
  double error1 = std::abs(value1 - value);

  // Calculate SMR without kFixedPointRootMultiplier.
  uint32_t root2 = std::sqrt(value);
  double value2 = root2 * root2;
  double error2 = std::abs(value2 - value);

  // Verify using kFixedPointRootMultiplier is relatively more accurate.
  EXPECT_LE(error1, error2);

  // Verify using kFixedPointRootMultiplier is accurate in an absolute sense.
  EXPECT_LE(error1, 1);
}

TEST(FrameMetricsFixedPointTest, kFixedPointRootMultiplierSqrt) {
  EXPECT_EQ(kFixedPointRootMultiplierSqrt,
            std::sqrt(kFixedPointRootMultiplier));
}

TEST(FrameMetricsFixedPointTest, kFixedPointRootShift) {
  EXPECT_EQ(kFixedPointRootMultiplier, 1LL << kFixedPointRootShift);
}

// Verify Accumulator96b's squared weight constructor.
TEST(FrameMetricsFixedPointTest, Accumulator96bConstructor) {
  // A small value that fits in 32 bits.
  uint64_t a = 13;
  Accumulator96b a1(a, 2);
  EXPECT_DOUBLE_EQ(a1.ToDouble(), a * a * 2);

  // A "medium" value that fits in 64 bits.
  uint64_t b = 0x10000001;
  Accumulator96b a2(b, 2);
  EXPECT_DOUBLE_EQ(a2.ToDouble(), b * b * 2);

  // A large value that fits in 96 bits.
  uint64_t c = 0x80000001;
  Accumulator96b a3(c, c);
  EXPECT_DOUBLE_EQ(a3.ToDouble(), std::pow(c, 3));

  // The largest initial 96-bit value.
  uint64_t d = 0xFFFFFFFF;
  Accumulator96b a4(d, d);
  EXPECT_DOUBLE_EQ(a4.ToDouble(), std::pow(d, 3));

  // A mix of the two above.
  double cf = c;
  double df = d;
  Accumulator96b a5(c, d);
  EXPECT_DOUBLE_EQ(a5.ToDouble(), cf * cf * df);
  Accumulator96b a6(d, c);
  EXPECT_DOUBLE_EQ(a6.ToDouble(), df * df * cf);
}

// Verify Accumulator96b::Add and Subtract.
TEST(FrameMetricsFixedPointTest, Accumulator96bAddSub) {
  uint32_t v = 0xFFFFFFFF;

  // A small value that fits in 32 bits and would carry into
  // upper most 64 bits during accumulation.
  Accumulator96b a1(1, v);
  Accumulator96b accum1;
  for (int i = 0; i <= 0xFF; i++) {
    accum1.Add(a1);
    EXPECT_DOUBLE_EQ(accum1.ToDouble(), static_cast<double>(v) * (i + 1));
  }
  for (int i = 0xFF; i >= 0; i--) {
    accum1.Subtract(a1);
    EXPECT_DOUBLE_EQ(accum1.ToDouble(), static_cast<double>(v) * i);
  }

  // A larger value that fits in 64 bits and would carry into
  // upper most 32 bits during accumulation.
  Accumulator96b a2(v, 1);
  Accumulator96b accum2;
  for (int i = 0; i <= 0xFF; i++) {
    accum2.Add(a2);
    EXPECT_DOUBLE_EQ(accum2.ToDouble(), static_cast<double>(v) * v * (i + 1));
  }
  for (int i = 0xFF; i >= 0; i--) {
    accum2.Subtract(a2);
    EXPECT_DOUBLE_EQ(accum2.ToDouble(), static_cast<double>(v) * v * i);
  }
}

// Verify Accumulator96b precision is always 1.
TEST(FrameMetricsFixedPointTest, Accumulator96bPrecision) {
  uint32_t v = 0xFFFFFFFF;
  Accumulator96b a1(1, 1);  // 1. Smallest non-zero value possible.
  Accumulator96b a2(v, v);  // Largest initial value possible.
  Accumulator96b a3(v, v);  // Largest initial value possible, minus 1.
  a3.Subtract(a1);

  // Verify that conversion to a double loses precision from a3.
  double a2f = a2.ToDouble();
  double a3f = a3.ToDouble();
  EXPECT_DOUBLE_EQ(a2f, a3f);
  EXPECT_DOUBLE_EQ(0, a2f - a3f);

  // Verify delta between a2 and a3 is 1 when computed internally.
  Accumulator96b a4(a2);
  a4.Subtract(a3);
  EXPECT_DOUBLE_EQ(1, a4.ToDouble());
}

}  // namespace frame_metrics
}  // namespace ui
