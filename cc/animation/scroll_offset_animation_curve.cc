// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/scroll_offset_animation_curve.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "cc/animation/timing_function.h"
#include "cc/base/time_util.h"
#include "ui/gfx/animation/tween.h"

using DurationBehavior = cc::ScrollOffsetAnimationCurve::DurationBehavior;

const double kConstantDuration = 9.0;
const double kDurationDivisor = 60.0;

const double kInverseDeltaRampStartPx = 120.0;
const double kInverseDeltaRampEndPx = 480.0;
const double kInverseDeltaMinDuration = 6.0;
const double kInverseDeltaMaxDuration = 12.0;

const double kInverseDeltaSlope =
    (kInverseDeltaMinDuration - kInverseDeltaMaxDuration) /
    (kInverseDeltaRampEndPx - kInverseDeltaRampStartPx);

const double kInverseDeltaOffset =
    kInverseDeltaMaxDuration - kInverseDeltaRampStartPx * kInverseDeltaSlope;

namespace cc {

namespace {

const double kEpsilon = 0.01f;

static float MaximumDimension(const gfx::Vector2dF& delta) {
  return std::abs(delta.x()) > std::abs(delta.y()) ? delta.x() : delta.y();
}

static std::unique_ptr<TimingFunction> EaseOutWithInitialVelocity(
    double velocity) {
  // Clamp velocity to a sane value.
  velocity = std::min(std::max(velocity, -1000.0), 1000.0);

  // Based on CubicBezierTimingFunction::EaseType::EASE_IN_OUT preset
  // with first control point scaled.
  const double x1 = 0.42;
  const double y1 = velocity * x1;
  return CubicBezierTimingFunction::Create(x1, y1, 0.58, 1);
}

}  // namespace

std::unique_ptr<ScrollOffsetAnimationCurve> ScrollOffsetAnimationCurve::Create(
    const gfx::ScrollOffset& target_value,
    std::unique_ptr<TimingFunction> timing_function,
    DurationBehavior duration_behavior) {
  return base::WrapUnique(new ScrollOffsetAnimationCurve(
      target_value, std::move(timing_function), duration_behavior));
}

ScrollOffsetAnimationCurve::ScrollOffsetAnimationCurve(
    const gfx::ScrollOffset& target_value,
    std::unique_ptr<TimingFunction> timing_function,
    DurationBehavior duration_behavior)
    : target_value_(target_value),
      timing_function_(std::move(timing_function)),
      duration_behavior_(duration_behavior),
      has_set_initial_value_(false) {}

ScrollOffsetAnimationCurve::~ScrollOffsetAnimationCurve() = default;

base::TimeDelta ScrollOffsetAnimationCurve::SegmentDuration(
    const gfx::Vector2dF& delta,
    DurationBehavior behavior,
    base::TimeDelta delayed_by) {
  double duration = kConstantDuration;
  switch (behavior) {
    case DurationBehavior::CONSTANT:
      duration = kConstantDuration;
      break;
    case DurationBehavior::DELTA_BASED:
      duration = std::sqrt(std::abs(MaximumDimension(delta)));
      break;
    case DurationBehavior::INVERSE_DELTA:
      duration = std::min(
          std::max(kInverseDeltaOffset +
                       std::abs(MaximumDimension(delta)) * kInverseDeltaSlope,
                   kInverseDeltaMinDuration),
          kInverseDeltaMaxDuration);
      break;
    default:
      NOTREACHED();
  }

  base::TimeDelta time_delta = base::TimeDelta::FromMicroseconds(
      duration / kDurationDivisor * base::Time::kMicrosecondsPerSecond);

  time_delta -= delayed_by;
  if (time_delta >= base::TimeDelta())
    return time_delta;
  return base::TimeDelta();
}

void ScrollOffsetAnimationCurve::SetInitialValue(
    const gfx::ScrollOffset& initial_value,
    base::TimeDelta delayed_by) {
  initial_value_ = initial_value;
  has_set_initial_value_ = true;
  total_animation_duration_ = SegmentDuration(
      target_value_.DeltaFrom(initial_value_), duration_behavior_, delayed_by);
}

bool ScrollOffsetAnimationCurve::HasSetInitialValue() const {
  return has_set_initial_value_;
}

void ScrollOffsetAnimationCurve::ApplyAdjustment(
    const gfx::Vector2dF& adjustment) {
  initial_value_ = ScrollOffsetWithDelta(initial_value_, adjustment);
  target_value_ = ScrollOffsetWithDelta(target_value_, adjustment);
}

gfx::ScrollOffset ScrollOffsetAnimationCurve::GetValue(
    base::TimeDelta t) const {
  base::TimeDelta duration = total_animation_duration_ - last_retarget_;
  t -= last_retarget_;

  if (duration.is_zero())
    return target_value_;

  if (t <= base::TimeDelta())
    return initial_value_;

  if (t >= duration)
    return target_value_;

  double progress = timing_function_->GetValue(TimeUtil::Divide(t, duration));
  return gfx::ScrollOffset(
      gfx::Tween::FloatValueBetween(
          progress, initial_value_.x(), target_value_.x()),
      gfx::Tween::FloatValueBetween(
          progress, initial_value_.y(), target_value_.y()));
}

base::TimeDelta ScrollOffsetAnimationCurve::Duration() const {
  return total_animation_duration_;
}

AnimationCurve::CurveType ScrollOffsetAnimationCurve::Type() const {
  return SCROLL_OFFSET;
}

std::unique_ptr<AnimationCurve> ScrollOffsetAnimationCurve::Clone() const {
  return CloneToScrollOffsetAnimationCurve();
}

std::unique_ptr<ScrollOffsetAnimationCurve>
ScrollOffsetAnimationCurve::CloneToScrollOffsetAnimationCurve() const {
  std::unique_ptr<TimingFunction> timing_function(
      static_cast<TimingFunction*>(timing_function_->Clone().release()));
  std::unique_ptr<ScrollOffsetAnimationCurve> curve_clone =
      Create(target_value_, std::move(timing_function), duration_behavior_);
  curve_clone->initial_value_ = initial_value_;
  curve_clone->total_animation_duration_ = total_animation_duration_;
  curve_clone->last_retarget_ = last_retarget_;
  curve_clone->has_set_initial_value_ = has_set_initial_value_;
  return curve_clone;
}

static double VelocityBasedDurationBound(gfx::Vector2dF old_delta,
                                         double old_normalized_velocity,
                                         double old_duration,
                                         gfx::Vector2dF new_delta) {
  double old_delta_max_dimension = MaximumDimension(old_delta);
  double new_delta_max_dimension = MaximumDimension(new_delta);

  // If we are already at the target, stop animating.
  if (std::abs(new_delta_max_dimension) < kEpsilon)
    return 0;

  // Guard against division by zero.
  if (std::abs(old_delta_max_dimension) < kEpsilon ||
      std::abs(old_normalized_velocity) < kEpsilon) {
    return std::numeric_limits<double>::infinity();
  }

  // Estimate how long it will take to reach the new target at our present
  // velocity, with some fudge factor to account for the "ease out".
  double old_true_velocity =
      old_normalized_velocity * old_delta_max_dimension / old_duration;
  double bound = (new_delta_max_dimension / old_true_velocity) * 2.5f;

  // If bound < 0 we are moving in the opposite direction.
  return bound < 0 ? std::numeric_limits<double>::infinity() : bound;
}

void ScrollOffsetAnimationCurve::UpdateTarget(
    double t,
    const gfx::ScrollOffset& new_target) {
  if (std::abs(MaximumDimension(target_value_.DeltaFrom(new_target))) <
      kEpsilon) {
    target_value_ = new_target;
    return;
  }

  base::TimeDelta delayed_by = base::TimeDelta::FromSecondsD(
      std::max(0.0, last_retarget_.InSecondsF() - t));
  t = std::max(t, last_retarget_.InSecondsF());

  gfx::ScrollOffset current_position =
      GetValue(base::TimeDelta::FromSecondsD(t));
  gfx::Vector2dF old_delta = target_value_.DeltaFrom(initial_value_);
  gfx::Vector2dF new_delta = new_target.DeltaFrom(current_position);

  // The last segement was of zero duration.
  if ((total_animation_duration_ - last_retarget_).is_zero()) {
    DCHECK_EQ(t, last_retarget_.InSecondsF());
    total_animation_duration_ =
        SegmentDuration(new_delta, duration_behavior_, delayed_by);
    target_value_ = new_target;
    return;
  }

  double old_duration =
      (total_animation_duration_ - last_retarget_).InSecondsF();
  double old_normalized_velocity = timing_function_->Velocity(
      (t - last_retarget_.InSecondsF()) / old_duration);

  // Use the velocity-based duration bound when it is less than the constant
  // segment duration. This minimizes the "rubber-band" bouncing effect when
  // old_normalized_velocity is large and new_delta is small.
  double new_duration = std::min(
      SegmentDuration(new_delta, duration_behavior_, delayed_by).InSecondsF(),
      VelocityBasedDurationBound(old_delta, old_normalized_velocity,
                                 old_duration, new_delta));

  if (new_duration < kEpsilon) {
    // We are already at or very close to the new target. Stop animating.
    target_value_ = new_target;
    total_animation_duration_ = base::TimeDelta::FromSecondsD(t);
    return;
  }

  // TimingFunction::Velocity gives the slope of the curve from 0 to 1.
  // To match the "true" velocity in px/sec we must adjust this slope for
  // differences in duration and scroll delta between old and new curves.
  double new_normalized_velocity =
      old_normalized_velocity * (new_duration / old_duration) *
      (MaximumDimension(old_delta) / MaximumDimension(new_delta));

  initial_value_ = current_position;
  target_value_ = new_target;
  total_animation_duration_ = base::TimeDelta::FromSecondsD(t + new_duration);
  last_retarget_ = base::TimeDelta::FromSecondsD(t);
  timing_function_ = EaseOutWithInitialVelocity(new_normalized_velocity);
}

}  // namespace cc
