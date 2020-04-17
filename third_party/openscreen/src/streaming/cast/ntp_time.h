// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_NTP_TIME_H_
#define STREAMING_CAST_NTP_TIME_H_

#include <stdint.h>

#include "platform/api/time.h"

namespace openscreen {
namespace cast_streaming {

// NTP timestamps are 64-bit timestamps that consist of two 32-bit parts: 1) The
// number of seconds since 1 January 1900; and 2) The fraction of the second,
// where 0 maps to 0x00000000 and each unit increment represents another 2^-32
// seconds.
//
// Note that it is part of the design of NTP for the seconds part to roll around
// on 7 February 2036.
using NtpTimestamp = uint64_t;

// NTP fixed-point time math: Declare two std::chrono::duration types with the
// bit-width necessary to reliably perform all conversions to/from NTP format.
using NtpSeconds = std::chrono::duration<int64_t, std::chrono::seconds::period>;
using NtpFraction =
    std::chrono::duration<int64_t, std::ratio<1, INT64_C(0x100000000)>>;

constexpr NtpSeconds NtpSecondsPart(NtpTimestamp timestamp) {
  return NtpSeconds(timestamp >> 32);
}

constexpr NtpFraction NtpFractionPart(NtpTimestamp timestamp) {
  return NtpFraction(timestamp & 0xffffffff);
}

constexpr NtpTimestamp AssembleNtpTimestamp(NtpSeconds seconds,
                                            NtpFraction fraction) {
  return (static_cast<uint64_t>(seconds.count()) << 32) |
         static_cast<uint32_t>(fraction.count());
}

// Converts between platform::Clock::time_points and NtpTimestamps. The class is
// instantiated with platform::Clock::now() and the current wall clock time, and
// these are used to determine a fixed origin reference point for all
// conversions. Thus, to avoid introducing unintended timing-related behaviors,
// only one NtpTimeConverter instance should be used for converting all the NTP
// timestamps in the same streaming session.
class NtpTimeConverter {
 public:
  NtpTimeConverter(platform::Clock::time_point now = platform::Clock::now(),
                   std::chrono::seconds since_unix_epoch =
                       platform::GetWallTimeSinceUnixEpoch());
  ~NtpTimeConverter();

  NtpTimestamp ToNtpTimestamp(platform::Clock::time_point time_point) const;
  platform::Clock::time_point ToLocalTime(NtpTimestamp timestamp) const;

 private:
  // The time point on the platform clock's timeline that corresponds to
  // approximately the same time point on the NTP timeline. Note that it is
  // acceptable for the granularity of the NTP seconds value to be whole seconds
  // here: Both a Cast Streaming Sender and Receiver will assume their clocks
  // can be off (with respect to each other) by even a large amount; and all
  // that matters is that time ticks forward at a reasonable pace from some
  // initial point.
  const platform::Clock::time_point start_time_;
  const NtpSeconds since_ntp_epoch_;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_NTP_TIME_H_
