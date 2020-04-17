// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/ntp_time.h"

#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

using openscreen::platform::Clock;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::milliseconds;

namespace openscreen {
namespace cast_streaming {

TEST(NtpTimestampTest, SplitsIntoParts) {
  // 1 Jan 1900.
  NtpTimestamp timestamp = UINT64_C(0x0000000000000000);
  EXPECT_EQ(NtpSeconds::zero(), NtpSecondsPart(timestamp));
  EXPECT_EQ(NtpFraction::zero(), NtpFractionPart(timestamp));

  // 1 Jan 1900 plus 10 ms.
  timestamp = UINT64_C(0x00000000028f5c29);
  EXPECT_EQ(NtpSeconds::zero(), NtpSecondsPart(timestamp));
  EXPECT_EQ(milliseconds(10),
            duration_cast<milliseconds>(NtpFractionPart(timestamp)));

  // 1 Jan 1970 minus 2^-32 seconds.
  timestamp = UINT64_C(0x83aa7e80ffffffff);
  EXPECT_EQ(NtpSeconds(INT64_C(2208988800)), NtpSecondsPart(timestamp));
  EXPECT_EQ(NtpFraction(0xffffffff), NtpFractionPart(timestamp));

  // 2019-03-23 17:25:50.500.
  timestamp = UINT64_C(0xe0414d0e80000000);
  EXPECT_EQ(NtpSeconds(INT64_C(3762375950)), NtpSecondsPart(timestamp));
  EXPECT_EQ(milliseconds(500),
            duration_cast<milliseconds>(NtpFractionPart(timestamp)));
}

TEST(NtpTimestampTest, AssemblesFromParts) {
  // 1 Jan 1900.
  NtpTimestamp timestamp =
      AssembleNtpTimestamp(NtpSeconds::zero(), NtpFraction::zero());
  EXPECT_EQ(UINT64_C(0x0000000000000000), timestamp);

  // 1 Jan 1900 plus 10 ms. Note that the duration_cast<NtpFraction>(10ms)
  // truncates rather than rounds the 10ms value, so the resulting timestamp is
  // one fractional tick less than the one found in the SplitsIntoParts test.
  // The ~0.4 nanosecond error in the conversion is totally insignificant to a
  // live system.
  timestamp = AssembleNtpTimestamp(
      NtpSeconds::zero(), duration_cast<NtpFraction>(milliseconds(10)));
  EXPECT_EQ(UINT64_C(0x00000000028f5c28), timestamp);

  // 1 Jan 1970 minus 2^-32 seconds.
  timestamp = AssembleNtpTimestamp(NtpSeconds(INT64_C(2208988799)),
                                   NtpFraction(0xffffffff));
  EXPECT_EQ(UINT64_C(0x83aa7e7fffffffff), timestamp);

  // 2019-03-23 17:25:50.500.
  timestamp =
      AssembleNtpTimestamp(NtpSeconds(INT64_C(3762375950)),
                           duration_cast<NtpFraction>(milliseconds(500)));
  EXPECT_EQ(UINT64_C(0xe0414d0e80000000), timestamp);
}

TEST(NtpTimeConverterTest, ConvertsToNtpTimeAndBack) {
  // There is an undetermined amount of delay between the sampling of the two
  // clocks, but that is accounted for in the design (see class comments).
  // Normally, sampling real clocks in unit tests is a recipe for flakiness
  // down-the-road. However, if there is flakiness in this test, then some of
  // our core assumptions (or the design) about the time math are wrong and
  // should be looked into!
  const Clock::time_point steady_clock_start = Clock::now();
  const std::chrono::seconds wall_clock_start =
      platform::GetWallTimeSinceUnixEpoch();
  SCOPED_TRACE(::testing::Message()
               << "steady_clock_start.time_since_epoch().count() is "
               << steady_clock_start.time_since_epoch().count()
               << ", wall_clock_start.count() is " << wall_clock_start.count());

  const NtpTimeConverter converter(steady_clock_start, wall_clock_start);

  // Convert time points between the start time and 5 seconds later, in 10 ms
  // increments. Allow the converted-back time point to be at most 1 clock tick
  // off from the original value, but all converted values should always be
  // monotonically increasing.
  const Clock::time_point end_point = steady_clock_start + milliseconds(5000);
  NtpTimestamp last_ntp_timestamp = 0;
  Clock::time_point last_converted_back_time_point = Clock::time_point::min();
  for (Clock::time_point t = steady_clock_start; t < end_point;
       t += milliseconds(10)) {
    const NtpTimestamp ntp_timestamp = converter.ToNtpTimestamp(t);
    ASSERT_GT(ntp_timestamp, last_ntp_timestamp);
    last_ntp_timestamp = ntp_timestamp;

    const Clock::time_point converted_back_time_point =
        converter.ToLocalTime(ntp_timestamp);
    ASSERT_GT(converted_back_time_point, last_converted_back_time_point);
    last_converted_back_time_point = converted_back_time_point;

    ASSERT_NEAR(t.time_since_epoch().count(),
                converted_back_time_point.time_since_epoch().count(),
                1 /* tick */);
  }
}

}  // namespace cast_streaming
}  // namespace openscreen
