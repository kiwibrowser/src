// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_RTP_TIME_H_
#define STREAMING_CAST_RTP_TIME_H_

#include <stdint.h>

#include <chrono>
#include <cmath>
#include <limits>
#include <sstream>

#include "platform/api/time.h"
#include "streaming/cast/expanded_value_base.h"

namespace openscreen {
namespace cast_streaming {

// Forward declarations (see below).
class RtpTimeDelta;
class RtpTimeTicks;

// Convenience operator overloads for logging.
std::ostream& operator<<(std::ostream& out, const RtpTimeDelta rhs);
std::ostream& operator<<(std::ostream& out, const RtpTimeTicks rhs);

// The difference between two RtpTimeTicks values.  This data type is modeled
// off of Chromium's base::TimeDelta, and used for performing compiler-checked
// arithmetic with RtpTimeTicks.
//
// This data type wraps a value, providing only the meaningful set of math
// operations that may be performed on the value.  RtpTimeDeltas may be
// added/subtracted with other RtpTimeDeltas to produce a RtpTimeDelta holding
// the sum/difference.  RtpTimeDeltas may also be multiplied or divided by
// integer amounts.  Finally, RtpTimeDeltas may be divided by other
// RtpTimeDeltas to compute a number of periods (trunc'ed to an integer), or
// modulo each other to determine a time period remainder.
//
// The base class provides bit truncation/extension features for
// wire-formatting, and also the comparison operators.
//
// Usage example:
//
//   // Time math.
//   RtpTimeDelta zero;
//   RtpTimeDelta one_second_later =
//       zero + RtpTimeDelta::FromTicks(kAudioSamplingRate);
//   RtpTimeDelta ten_seconds_later = one_second_later * 10;
//   int64_t ten_periods = ten_seconds_later / one_second_later;
//
//   // Logging convenience.
//   OSP_DLOG_INFO << "The RTP time offset is " << ten_seconds_later;
//
//   // Convert (approximately!) between RTP timebase and microsecond timebase:
//   RtpTimeDelta nine_seconds_in_rtp = ten_seconds_later - one_second_later;
//   using std::chrono::microseconds;
//   microseconds nine_seconds_duration =
//       nine_seconds_in_rtp.ToDuration<microseconds>(kAudioSamplingRate);
//   RtpTimeDelta two_seconds_in_rtp =
//       RtpTimeDelta::FromDuration(std::chrono::seconds(2),
//                                  kAudioSamplingRate);
class RtpTimeDelta : public ExpandedValueBase<int64_t, RtpTimeDelta> {
 public:
  constexpr RtpTimeDelta() : ExpandedValueBase(0) {}

  // Arithmetic operators (with other deltas).
  constexpr RtpTimeDelta operator+(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ + rhs.value_);
  }
  constexpr RtpTimeDelta operator-(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ - rhs.value_);
  }
  constexpr RtpTimeDelta& operator+=(RtpTimeDelta rhs) {
    return (*this = (*this + rhs));
  }
  constexpr RtpTimeDelta& operator-=(RtpTimeDelta rhs) {
    return (*this = (*this - rhs));
  }
  constexpr RtpTimeDelta operator-() const { return RtpTimeDelta(-value_); }

  // Multiplicative operators (with other deltas).
  constexpr int64_t operator/(RtpTimeDelta rhs) const {
    return value_ / rhs.value_;
  }
  constexpr RtpTimeDelta operator%(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ % rhs.value_);
  }
  constexpr RtpTimeDelta& operator%=(RtpTimeDelta rhs) {
    return (*this = (*this % rhs));
  }

  // Multiplicative operators (with integer types).
  template <typename IntType>
  constexpr RtpTimeDelta operator*(IntType rhs) const {
    static_assert(std::numeric_limits<IntType>::is_integer,
                  "|rhs| must be a POD integer type");
    return RtpTimeDelta(value_ * rhs);
  }
  template <typename IntType>
  constexpr RtpTimeDelta operator/(IntType rhs) const {
    static_assert(std::numeric_limits<IntType>::is_integer,
                  "|rhs| must be a POD integer type");
    return RtpTimeDelta(value_ / rhs);
  }
  template <typename IntType>
  constexpr RtpTimeDelta& operator*=(IntType rhs) {
    return (*this = (*this * rhs));
  }
  template <typename IntType>
  constexpr RtpTimeDelta& operator/=(IntType rhs) {
    return (*this = (*this / rhs));
  }

  // Maps this RtpTimeDelta to an approximate std::chrono::duration using the
  // given RTP timebase.  Assumes a zero-valued Duration corresponds to a
  // zero-valued RtpTimeDelta.
  template <typename Duration>
  Duration ToDuration(int rtp_timebase) const {
    OSP_DCHECK_GT(rtp_timebase, 0);
    constexpr Duration kOneSecond =
        std::chrono::duration_cast<Duration>(std::chrono::seconds(1));
    return Duration(ToNearestRepresentativeValue<typename Duration::rep>(
        static_cast<double>(value_) / rtp_timebase * kOneSecond.count()));
  }

  // Maps the |duration| to an approximate RtpTimeDelta using the given RTP
  // timebase.  Assumes a zero-valued Duration corresponds to a zero-valued
  // RtpTimeDelta.
  template <typename Duration>
  static constexpr RtpTimeDelta FromDuration(Duration duration,
                                             int rtp_timebase) {
    constexpr Duration kOneSecond =
        std::chrono::duration_cast<Duration>(std::chrono::seconds(1));
    static_assert(kOneSecond > Duration::zero(),
                  "Duration is too coarse-grained to represent one second.");
    return RtpTimeDelta(ToNearestRepresentativeValue<int64_t>(
        static_cast<double>(duration.count()) / kOneSecond.count() *
        rtp_timebase));
  }

  // Construct a RtpTimeDelta from an exact number of ticks.
  static constexpr RtpTimeDelta FromTicks(int64_t ticks) {
    return RtpTimeDelta(ticks);
  }

 private:
  friend class ExpandedValueBase<int64_t, RtpTimeDelta>;
  friend class RtpTimeTicks;
  friend std::ostream& operator<<(std::ostream& out, const RtpTimeDelta rhs);

  constexpr explicit RtpTimeDelta(int64_t ticks) : ExpandedValueBase(ticks) {}

  constexpr int64_t value() const { return value_; }

  template <typename Rep>
  static constexpr Rep ToNearestRepresentativeValue(double ticks) {
    if (!std::is_floating_point<Rep>::value) {
      ticks = std::round(ticks);
    }
    OSP_DCHECK(std::isfinite(ticks));
    if (ticks < std::numeric_limits<Rep>::min()) {
      return std::numeric_limits<Rep>::min();
    } else if (ticks > std::numeric_limits<Rep>::max()) {
      return std::numeric_limits<Rep>::max();
    }
    return Rep(ticks);
  }
};

// A media timestamp whose timebase matches the periodicity of the content
// (e.g., for audio, the timebase would be the sampling frequency).  This data
// type is modeled off of Chromium's base::TimeTicks.
//
// This data type wraps a value, providing only the meaningful set of math
// operations that may be performed on the value.  The difference between two
// RtpTimeTicks is a RtpTimeDelta.  Likewise, adding or subtracting a
// RtpTimeTicks with a RtpTimeDelta produces an off-set RtpTimeTicks.
//
// The base class provides bit truncation/extension features for
// wire-formatting, and also the comparison operators.
//
// Usage example:
//
//   // Time math.
//   RtpTimeTicks origin;
//   RtpTimeTicks at_one_second =
//       origin + RtpTimeDelta::FromTicks(kAudioSamplingRate);
//   RtpTimeTicks at_two_seconds =
//       at_one_second + RtpTimeDelta::FromTicks(kAudioSamplingRate);
//   RtpTimeDelta elasped_in_between = at_two_seconds - at_one_second;
//   RtpTimeDelta thrice_as_much_elasped = elasped_in_between * 3;
//   RtpTimeTicks at_four_seconds = at_one_second + thrice_as_much_elasped;
//
//   // Logging convenience.
//   OSP_DLOG_INFO << "The RTP timestamp is " << at_four_seconds;
//
//   // Convert (approximately!) between RTP timebase and stream time offsets in
//   // microsecond timebase:
//   using std::chrono::microseconds;
//   microseconds four_seconds_since_stream_start =
//       at_four_seconds.ToTimeSinceOrigin<microseconds>(kAudioSamplingRate);
//   RtpTimeTicks at_three_seconds = RtpTimeDelta::FromTimeSinceOrigin(
//       std::chrono::seconds(3), kAudioSamplingRate);
class RtpTimeTicks : public ExpandedValueBase<int64_t, RtpTimeTicks> {
 public:
  constexpr RtpTimeTicks() : ExpandedValueBase(0) {}

  // Compute the difference between two RtpTimeTickses.
  constexpr RtpTimeDelta operator-(RtpTimeTicks rhs) const {
    return RtpTimeDelta(value_ - rhs.value_);
  }

  // Return a new RtpTimeTicks before or after this one.
  constexpr RtpTimeTicks operator+(RtpTimeDelta rhs) const {
    return RtpTimeTicks(value_ + rhs.value());
  }
  constexpr RtpTimeTicks operator-(RtpTimeDelta rhs) const {
    return RtpTimeTicks(value_ - rhs.value());
  }
  constexpr RtpTimeTicks& operator+=(RtpTimeDelta rhs) {
    return (*this = (*this + rhs));
  }
  constexpr RtpTimeTicks& operator-=(RtpTimeDelta rhs) {
    return (*this = (*this - rhs));
  }

  // Maps this RtpTimeTicks to an approximate std::chrono::duration representing
  // the amount of time since the origin point (e.g., the start of a stream)
  // using the given |rtp_timebase|.  Assumes a zero-valued Duration corresponds
  // to a zero-valued RtpTimeTicks.
  template <typename Duration>
  Duration ToTimeSinceOrigin(int rtp_timebase) const {
    return (*this - RtpTimeTicks()).ToDuration<Duration>(rtp_timebase);
  }

  // Maps the |time_since_origin| to an approximate RtpTimeTicks using the given
  // RTP timebase.  Assumes a zero-valued Duration corresponds to a zero-valued
  // RtpTimeTicks.
  template <typename Duration>
  static constexpr RtpTimeTicks FromTimeSinceOrigin(Duration time_since_origin,
                                                    int rtp_timebase) {
    return RtpTimeTicks() +
           RtpTimeDelta::FromDuration(time_since_origin, rtp_timebase);
  }

 private:
  friend class ExpandedValueBase<int64_t, RtpTimeTicks>;
  friend std::ostream& operator<<(std::ostream& out, const RtpTimeTicks rhs);

  constexpr explicit RtpTimeTicks(int64_t value) : ExpandedValueBase(value) {}

  constexpr int64_t value() const { return value_; }
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_RTP_TIME_H_
