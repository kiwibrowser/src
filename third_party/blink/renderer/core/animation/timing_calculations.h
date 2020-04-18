/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TIMING_CALCULATIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TIMING_CALCULATIONS_H_

#include "third_party/blink/renderer/core/animation/animation_effect.h"
#include "third_party/blink/renderer/core/animation/timing.h"
#include "third_party/blink/renderer/platform/animation/animation_utilities.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"

namespace blink {

static inline double MultiplyZeroAlwaysGivesZero(double x, double y) {
  DCHECK(!IsNull(x));
  DCHECK(!IsNull(y));
  return x && y ? x * y : 0;
}

static inline AnimationEffect::Phase CalculatePhase(double active_duration,
                                                    double local_time,
                                                    const Timing& specified) {
  DCHECK_GE(active_duration, 0);
  if (IsNull(local_time))
    return AnimationEffect::kPhaseNone;
  double end_time =
      specified.start_delay + active_duration + specified.end_delay;
  if (local_time < std::min(specified.start_delay, end_time))
    return AnimationEffect::kPhaseBefore;
  if (local_time >= std::min(specified.start_delay + active_duration, end_time))
    return AnimationEffect::kPhaseAfter;
  return AnimationEffect::kPhaseActive;
}

static inline bool IsActiveInParentPhase(AnimationEffect::Phase parent_phase,
                                         Timing::FillMode fill_mode) {
  switch (parent_phase) {
    case AnimationEffect::kPhaseBefore:
      return fill_mode == Timing::FillMode::BACKWARDS ||
             fill_mode == Timing::FillMode::BOTH;
    case AnimationEffect::kPhaseActive:
      return true;
    case AnimationEffect::kPhaseAfter:
      return fill_mode == Timing::FillMode::FORWARDS ||
             fill_mode == Timing::FillMode::BOTH;
    default:
      NOTREACHED();
      return false;
  }
}

static inline double CalculateActiveTime(double active_duration,
                                         Timing::FillMode fill_mode,
                                         double local_time,
                                         AnimationEffect::Phase parent_phase,
                                         AnimationEffect::Phase phase,
                                         const Timing& specified) {
  DCHECK_GE(active_duration, 0);
  DCHECK_EQ(phase, CalculatePhase(active_duration, local_time, specified));

  switch (phase) {
    case AnimationEffect::kPhaseBefore:
      if (fill_mode == Timing::FillMode::BACKWARDS ||
          fill_mode == Timing::FillMode::BOTH)
        return 0;
      return NullValue();
    case AnimationEffect::kPhaseActive:
      if (IsActiveInParentPhase(parent_phase, fill_mode))
        return local_time - specified.start_delay;
      return NullValue();
    case AnimationEffect::kPhaseAfter:
      if (fill_mode == Timing::FillMode::FORWARDS ||
          fill_mode == Timing::FillMode::BOTH)
        return std::max(0.0, std::min(active_duration,
                                      active_duration + specified.end_delay));
      return NullValue();
    case AnimationEffect::kPhaseNone:
      DCHECK(IsNull(local_time));
      return NullValue();
    default:
      NOTREACHED();
      return NullValue();
  }
}

static inline double CalculateScaledActiveTime(double active_duration,
                                               double active_time,
                                               double start_offset,
                                               const Timing& specified) {
  DCHECK_GE(active_duration, 0);
  DCHECK_GE(start_offset, 0);

  if (IsNull(active_time))
    return NullValue();

  DCHECK(active_time >= 0 && active_time <= active_duration);

  if (specified.playback_rate == 0)
    return start_offset;

  if (!std::isfinite(active_time))
    return std::numeric_limits<double>::infinity();

  return MultiplyZeroAlwaysGivesZero(specified.playback_rate < 0
                                         ? active_time - active_duration
                                         : active_time,
                                     specified.playback_rate) +
         start_offset;
}

static inline bool EndsOnIterationBoundary(double iteration_count,
                                           double iteration_start) {
  DCHECK(std::isfinite(iteration_count));
  return !fmod(iteration_count + iteration_start, 1);
}

// TODO(crbug.com/630915): Align this function with current Web Animations spec
// text.
static inline double CalculateIterationTime(double iteration_duration,
                                            double repeated_duration,
                                            double scaled_active_time,
                                            double start_offset,
                                            AnimationEffect::Phase phase,
                                            const Timing& specified) {
  DCHECK_GT(iteration_duration, 0);
  DCHECK_EQ(repeated_duration,
            MultiplyZeroAlwaysGivesZero(iteration_duration,
                                        specified.iteration_count));

  if (IsNull(scaled_active_time))
    return NullValue();

  DCHECK_GE(scaled_active_time, 0);
  DCHECK_LE(scaled_active_time, repeated_duration + start_offset);

  if (!std::isfinite(scaled_active_time) ||
      (scaled_active_time - start_offset == repeated_duration &&
       specified.iteration_count &&
       EndsOnIterationBoundary(specified.iteration_count,
                               specified.iteration_start)))
    return iteration_duration;

  DCHECK(std::isfinite(scaled_active_time));
  double iteration_time = fmod(scaled_active_time, iteration_duration);

  // This implements step 3 of
  // https://drafts.csswg.org/web-animations/#calculating-the-simple-iteration-progress
  if (iteration_time == 0 && phase == AnimationEffect::kPhaseAfter &&
      repeated_duration != 0 && scaled_active_time != 0)
    return iteration_duration;

  return iteration_time;
}

static inline double CalculateCurrentIteration(double iteration_duration,
                                               double iteration_time,
                                               double scaled_active_time,
                                               const Timing& specified) {
  DCHECK_GT(iteration_duration, 0);
  DCHECK(IsNull(iteration_time) || iteration_time >= 0);

  if (IsNull(scaled_active_time))
    return NullValue();

  DCHECK_GE(iteration_time, 0);
  DCHECK_LE(iteration_time, iteration_duration);
  DCHECK_GE(scaled_active_time, 0);

  if (!scaled_active_time)
    return 0;

  if (iteration_time == iteration_duration)
    return specified.iteration_start + specified.iteration_count - 1;

  return floor(scaled_active_time / iteration_duration);
}

static inline double CalculateDirectedTime(double current_iteration,
                                           double iteration_duration,
                                           double iteration_time,
                                           const Timing& specified) {
  DCHECK(IsNull(current_iteration) || current_iteration >= 0);
  DCHECK_GT(iteration_duration, 0);

  if (IsNull(iteration_time))
    return NullValue();

  DCHECK_GE(current_iteration, 0);
  DCHECK_GE(iteration_time, 0);
  DCHECK_LE(iteration_time, iteration_duration);

  const bool current_iteration_is_odd = fmod(current_iteration, 2) >= 1;
  const bool current_direction_is_forwards =
      specified.direction == Timing::PlaybackDirection::NORMAL ||
      (specified.direction == Timing::PlaybackDirection::ALTERNATE_NORMAL &&
       !current_iteration_is_odd) ||
      (specified.direction == Timing::PlaybackDirection::ALTERNATE_REVERSE &&
       current_iteration_is_odd);

  return current_direction_is_forwards ? iteration_time
                                       : iteration_duration - iteration_time;
}

static inline base::Optional<double> CalculateTransformedTime(
    double current_iteration,
    double iteration_duration,
    double iteration_time,
    const Timing& specified) {
  DCHECK(IsNull(current_iteration) || current_iteration >= 0);
  DCHECK_GT(iteration_duration, 0);
  DCHECK(IsNull(iteration_time) ||
         (iteration_time >= 0 && iteration_time <= iteration_duration));

  double directed_time = CalculateDirectedTime(
      current_iteration, iteration_duration, iteration_time, specified);
  if (IsNull(directed_time))
    return base::nullopt;
  if (!std::isfinite(iteration_duration))
    return directed_time;
  double time_fraction = directed_time / iteration_duration;
  DCHECK(time_fraction >= 0 && time_fraction <= 1);
  return MultiplyZeroAlwaysGivesZero(
      iteration_duration,
      specified.timing_function->Evaluate(
          time_fraction, AccuracyForDuration(iteration_duration)));
}

}  // namespace blink

#endif
