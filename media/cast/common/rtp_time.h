// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_COMMON_RTP_TIME_H_
#define MEDIA_CAST_COMMON_RTP_TIME_H_

#include <stdint.h>

#include <limits>
#include <sstream>

#include "base/time/time.h"
#include "media/cast/common/expanded_value_base.h"

namespace media {
namespace cast {

// Forward declarations (see below).
class RtpTimeDelta;
class RtpTimeTicks;

// Convenience operator overloads for logging.
std::ostream& operator<<(std::ostream& out, const RtpTimeDelta rhs);
std::ostream& operator<<(std::ostream& out, const RtpTimeTicks rhs);

// The difference between two RtpTimeTicks values.  This data type is modeled
// off of base::TimeDelta, and used for performing compiler-checked arithmetic
// with RtpTimeTicks.
//
// This data type wraps a value, providing only the meaningful set of math
// operations that may be performed on the value.  RtpTimeDeltas may be
// added/subtracted with other RtpTimeDeltas to produce a RtpTimeDelta holding
// the sum/difference.  RtpTimeDeltas may also be multiplied or divided by
// integer amounts.  Finally, RtpTimeDeltas may be divided by other
// RtpTimeDeltas to compute a number of periods (floor'ed to an integer), or
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
//   DLOG(INFO) << "The RTP time offset is " << ten_seconds_later;
//
//   // Convert (approximately!) between RTP timebase and microsecond timebase:
//   base::TimeDelta nine_seconds_offset =
//       (ten_seconds_later - one_second_later).ToTimeDelta(kAudioSamplingRate);
//   RtpTimeDelta nine_seconds_rtp =
//       RtpTimeDelta::FromTimeDelta(nine_seconds_offset, kAudioSamplingRate);
class RtpTimeDelta : public ExpandedValueBase<int64_t, RtpTimeDelta> {
 public:
  RtpTimeDelta() : ExpandedValueBase(0) {}

  // Arithmetic operators (with other deltas).
  RtpTimeDelta operator+(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ + rhs.value_);
  }
  RtpTimeDelta operator-(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ - rhs.value_);
  }
  RtpTimeDelta& operator+=(RtpTimeDelta rhs) { return (*this = (*this + rhs)); }
  RtpTimeDelta& operator-=(RtpTimeDelta rhs) { return (*this = (*this - rhs)); }
  RtpTimeDelta operator-() const { return RtpTimeDelta(-value_); }

  // Multiplicative operators (with other deltas).
  int64_t operator/(RtpTimeDelta rhs) const { return value_ / rhs.value_; }
  RtpTimeDelta operator%(RtpTimeDelta rhs) const {
    return RtpTimeDelta(value_ % rhs.value_);
  }
  RtpTimeDelta& operator%=(RtpTimeDelta rhs) { return (*this = (*this % rhs)); }

  // Multiplicative operators (with integer types).
  template <typename IntType>
  RtpTimeDelta operator*(IntType rhs) const {
    static_assert(std::numeric_limits<IntType>::is_integer,
                  "|rhs| must be a POD integer type");
    return RtpTimeDelta(value_ * rhs);
  }
  template <typename IntType>
  RtpTimeDelta operator/(IntType rhs) const {
    static_assert(std::numeric_limits<IntType>::is_integer,
                  "|rhs| must be a POD integer type");
    return RtpTimeDelta(value_ / rhs);
  }
  template <typename IntType>
  RtpTimeDelta& operator*=(IntType rhs) { return (*this = (*this * rhs)); }
  template <typename IntType>
  RtpTimeDelta& operator/=(IntType rhs) { return (*this = (*this / rhs)); }

  // Maps this RtpTimeDelta to an approximate TimeDelta using the given
  // RTP timebase.  Assumes a zero-valued TimeDelta corresponds to a zero-valued
  // RtpTimeDelta.
  base::TimeDelta ToTimeDelta(int rtp_timebase) const;

  // Maps the TimeDelta to an approximate RtpTimeDelta using the given RTP
  // timebase.  Assumes a zero-valued TimeDelta corresponds to a zero-valued
  // RtpTimeDelta.
  static RtpTimeDelta FromTimeDelta(base::TimeDelta delta, int rtp_timebase);

  // Construct a RtpTimeDelta from an exact number of ticks.
  static RtpTimeDelta FromTicks(int64_t ticks);

 private:
  friend class ExpandedValueBase<int64_t, RtpTimeDelta>;
  friend class RtpTimeTicks;
  friend std::ostream& operator<<(std::ostream& out, const RtpTimeDelta rhs);

  explicit RtpTimeDelta(int64_t ticks) : ExpandedValueBase(ticks) {}

  int64_t value() const { return value_; }
};

// A media timestamp whose timebase matches the periodicity of the content
// (e.g., for audio, the timebase would be the sampling frequency).  This data
// type is modeled off of base::TimeTicks.
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
//   DLOG(INFO) << "The RTP timestamp is " << at_four_seconds;
//
//   // Convert (approximately!) between RTP timebase and media timestamps in
//   // microsecond timebase:
//   base::TimeDelta four_seconds_timestamp =
//       at_four_seconds.ToTimeDelta(kAudioSamplingRate);
//   video_frame->set_timestamp(four_seconds_timestamp);
//   RtpTimeTicks four_seconds_rtp = RtpTimeDelta::FromTimeDelta(
//       video_frame->timestamp(), kAudioSamplingRate);
class RtpTimeTicks : public ExpandedValueBase<int64_t, RtpTimeTicks> {
 public:
  RtpTimeTicks() : ExpandedValueBase(0) {}

  // Compute the difference between two RtpTimeTickses.
  RtpTimeDelta operator-(RtpTimeTicks rhs) const {
    return RtpTimeDelta(value_ - rhs.value_);
  }

  // Return a new RtpTimeTicks before or after this one.
  RtpTimeTicks operator+(RtpTimeDelta rhs) const {
    return RtpTimeTicks(value_ + rhs.value());
  }
  RtpTimeTicks operator-(RtpTimeDelta rhs) const {
    return RtpTimeTicks(value_ - rhs.value());
  }
  RtpTimeTicks& operator+=(RtpTimeDelta rhs) { return (*this = (*this + rhs)); }
  RtpTimeTicks& operator-=(RtpTimeDelta rhs) { return (*this = (*this - rhs)); }

  // Maps this RtpTimeTicks to an approximate TimeDelta using the given
  // RTP timebase.  Assumes a zero-valued TimeDelta corresponds to a zero-valued
  // RtpTimeTicks.
  base::TimeDelta ToTimeDelta(int rtp_timebase) const;

  // Maps the TimeDelta to an approximate RtpTimeTicks using the given RTP
  // timebase.  Assumes a zero-valued TimeDelta corresponds to a zero-valued
  // RtpTimeTicks.
  static RtpTimeTicks FromTimeDelta(base::TimeDelta delta, int rtp_timebase);

 private:
  friend class ExpandedValueBase<int64_t, RtpTimeTicks>;
  friend std::ostream& operator<<(std::ostream& out, const RtpTimeTicks rhs);

  explicit RtpTimeTicks(int64_t value) : ExpandedValueBase(value) {}

  int64_t value() const { return value_; }
};

}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_COMMON_RTP_TIME_H_
