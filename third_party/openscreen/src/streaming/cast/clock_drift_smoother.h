// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STREAMING_CAST_CLOCK_DRIFT_SMOOTHER_H_
#define STREAMING_CAST_CLOCK_DRIFT_SMOOTHER_H_

#include <chrono>

#include "platform/api/time.h"

namespace openscreen {
namespace cast_streaming {

// Tracks the jitter and drift between clocks, providing a smoothed offset.
// Internally, a Simple IIR filter is used to maintain a running average that
// moves at a rate based on the passage of time.
class ClockDriftSmoother {
 public:
  using Clock = platform::Clock;

  // |time_constant| is the amount of time an impulse signal takes to decay by
  // ~62.6%.  Interpretation: If the value passed to several Update() calls is
  // held constant for T seconds, then the running average will have moved
  // towards the value by ~62.6% from where it started.
  explicit ClockDriftSmoother(Clock::duration time_constant);
  ~ClockDriftSmoother();

  // Returns the current offset.
  Clock::duration Current() const;

  // Discard all history and reset to exactly |offset|, measured |now|.
  void Reset(Clock::time_point now, Clock::duration offset);

  // Update the current offset, which was measured |now|.  The weighting that
  // |measured_offset| will have on the running average is influenced by how
  // much time has passed since the last call to this method (or Reset()).
  // |now| should be monotonically non-decreasing over successive calls of this
  // method.
  void Update(Clock::time_point now, Clock::duration measured_offset);

  // A time constant suitable for most use cases, where the clocks are expected
  // to drift very little with respect to each other, and the jitter caused by
  // clock imprecision is effectively canceled out.
  static constexpr std::chrono::seconds kDefaultTimeConstant{30};

 private:
  const std::chrono::duration<double, Clock::duration::period> time_constant_;

  // The time at which |estimated_tick_offset_| was last updated.
  Clock::time_point last_update_time_;

  // The current estimated offset, as number of Clock::duration ticks.
  double estimated_tick_offset_;
};

}  // namespace cast_streaming
}  // namespace openscreen

#endif  // STREAMING_CAST_CLOCK_DRIFT_SMOOTHER_H_
