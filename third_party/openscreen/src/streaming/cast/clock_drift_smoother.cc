// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "streaming/cast/clock_drift_smoother.h"

#include <cmath>

#include "platform/api/logging.h"

namespace openscreen {
namespace cast_streaming {

namespace {
constexpr ClockDriftSmoother::Clock::time_point kNullTime =
    ClockDriftSmoother::Clock::time_point::min();
}

ClockDriftSmoother::ClockDriftSmoother(Clock::duration time_constant)
    : time_constant_(time_constant),
      last_update_time_(kNullTime),
      estimated_tick_offset_(0.0) {
  OSP_DCHECK(time_constant_ > decltype(time_constant_)::zero());
}

ClockDriftSmoother::~ClockDriftSmoother() = default;

ClockDriftSmoother::Clock::duration ClockDriftSmoother::Current() const {
  OSP_DCHECK(last_update_time_ != kNullTime);
  const double rounded_estimate = std::round(estimated_tick_offset_);
  if (rounded_estimate < Clock::duration::min().count()) {
    return Clock::duration::min();
  } else if (rounded_estimate > Clock::duration::max().count()) {
    return Clock::duration::max();
  }
  return Clock::duration(static_cast<Clock::duration::rep>(rounded_estimate));
}

void ClockDriftSmoother::Reset(Clock::time_point now,
                               Clock::duration measured_offset) {
  OSP_DCHECK(now != kNullTime);
  last_update_time_ = now;
  estimated_tick_offset_ = static_cast<double>(measured_offset.count());
}

void ClockDriftSmoother::Update(Clock::time_point now,
                                Clock::duration measured_offset) {
  OSP_DCHECK(now != kNullTime);
  if (last_update_time_ == kNullTime) {
    Reset(now, measured_offset);
  } else if (now < last_update_time_) {
    // |now| is not monotonically non-decreasing.
    OSP_NOTREACHED();
  } else {
    const double elapsed_ticks =
        static_cast<double>((now - last_update_time_).count());
    last_update_time_ = now;
    // Compute a weighted-average between the last estimate and
    // |measured_offset|. The more time that has elasped since the last call to
    // Update(), the more-heavily |measured_offset| will be weighed.
    const double weight =
        elapsed_ticks / (elapsed_ticks + time_constant_.count());
    estimated_tick_offset_ = weight * measured_offset.count() +
                             (1.0 - weight) * estimated_tick_offset_;
  }
}

}  // namespace cast_streaming
}  // namespace openscreen
