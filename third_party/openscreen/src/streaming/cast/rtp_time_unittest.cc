// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/rtp_time.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace openscreen {
namespace cast_streaming {

// Tests that conversions between std::chrono durations and RtpTimeDelta are
// accurate.  Note that this implicitly tests the conversions to/from
// RtpTimeTicks as well due to shared implementation.
TEST(RtpTimeDeltaTest, ConversionToAndFromDurations) {
  using std::chrono::microseconds;
  using std::chrono::milliseconds;
  using std::chrono::seconds;

  constexpr int kTimebase = 48000;

  // Origin in both timelines is equivalent.
  ASSERT_EQ(RtpTimeDelta(), RtpTimeDelta::FromTicks(0));
  ASSERT_EQ(RtpTimeDelta(),
            RtpTimeDelta::FromDuration(microseconds(0), kTimebase));
  ASSERT_EQ(microseconds::zero(),
            RtpTimeDelta::FromTicks(0).ToDuration<microseconds>(kTimebase));

  // Conversions that are exact (i.e., do not require rounding).
  ASSERT_EQ(RtpTimeDelta::FromTicks(480),
            RtpTimeDelta::FromDuration(milliseconds(10), kTimebase));
  ASSERT_EQ(RtpTimeDelta::FromTicks(96000),
            RtpTimeDelta::FromDuration(seconds(2), kTimebase));
  ASSERT_EQ(milliseconds(10),
            RtpTimeDelta::FromTicks(480).ToDuration<microseconds>(kTimebase));
  ASSERT_EQ(seconds(2),
            RtpTimeDelta::FromTicks(96000).ToDuration<microseconds>(kTimebase));

  // Conversions that are approximate (i.e., are rounded).
  for (int error_us = -3; error_us <= +3; ++error_us) {
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(0),
        RtpTimeDelta::FromDuration(microseconds(0 + error_us), kTimebase));
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(1),
        RtpTimeDelta::FromDuration(microseconds(21 + error_us), kTimebase));
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(2),
        RtpTimeDelta::FromDuration(microseconds(42 + error_us), kTimebase));
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(3),
        RtpTimeDelta::FromDuration(microseconds(63 + error_us), kTimebase));
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(4),
        RtpTimeDelta::FromDuration(microseconds(83 + error_us), kTimebase));
    ASSERT_EQ(
        RtpTimeDelta::FromTicks(11200000000000),
        RtpTimeDelta::FromDuration(
            microseconds(INT64_C(233333333333333) + error_us), kTimebase));
  }
  ASSERT_EQ(microseconds(21),
            RtpTimeDelta::FromTicks(1).ToDuration<microseconds>(kTimebase));
  ASSERT_EQ(microseconds(42),
            RtpTimeDelta::FromTicks(2).ToDuration<microseconds>(kTimebase));
  ASSERT_EQ(microseconds(63),
            RtpTimeDelta::FromTicks(3).ToDuration<microseconds>(kTimebase));
  ASSERT_EQ(microseconds(83),
            RtpTimeDelta::FromTicks(4).ToDuration<microseconds>(kTimebase));
  ASSERT_EQ(microseconds(INT64_C(233333333333333)),
            RtpTimeDelta::FromTicks(11200000000000)
                .ToDuration<microseconds>(kTimebase));
}

}  // namespace cast_streaming
}  // namespace openscreen
