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

#include "third_party/blink/renderer/core/animation/animation_effect.h"

#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/animation_input_helpers.h"
#include "third_party/blink/renderer/core/animation/computed_effect_timing.h"
#include "third_party/blink/renderer/core/animation/effect_timing.h"
#include "third_party/blink/renderer/core/animation/optional_effect_timing.h"
#include "third_party/blink/renderer/core/animation/timing_calculations.h"
#include "third_party/blink/renderer/core/animation/timing_input.h"

namespace blink {

namespace {

Timing::FillMode ResolvedFillMode(Timing::FillMode fill_mode,
                                  bool is_animation) {
  if (fill_mode != Timing::FillMode::AUTO)
    return fill_mode;
  if (is_animation)
    return Timing::FillMode::NONE;
  return Timing::FillMode::BOTH;
}

}  // namespace

AnimationEffect::AnimationEffect(const Timing& timing,
                                 EventDelegate* event_delegate)
    : owner_(nullptr),
      timing_(timing),
      event_delegate_(event_delegate),
      calculated_(),
      needs_update_(true),
      last_update_time_(NullValue()) {
  timing_.AssertValid();
}

double AnimationEffect::IterationDuration() const {
  double result = std::isnan(timing_.iteration_duration)
                      ? IntrinsicIterationDuration()
                      : timing_.iteration_duration;
  DCHECK_GE(result, 0);
  return result;
}

double AnimationEffect::RepeatedDuration() const {
  const double result =
      MultiplyZeroAlwaysGivesZero(IterationDuration(), timing_.iteration_count);
  DCHECK_GE(result, 0);
  return result;
}

double AnimationEffect::ActiveDurationInternal() const {
  const double result =
      timing_.playback_rate
          ? RepeatedDuration() / std::abs(timing_.playback_rate)
          : std::numeric_limits<double>::infinity();
  DCHECK_GE(result, 0);
  return result;
}

double AnimationEffect::EndTimeInternal() const {
  // Per the spec, the end time has a lower bound of 0.0:
  // https://drafts.csswg.org/web-animations-1/#end-time
  return std::max(
      timing_.start_delay + ActiveDurationInternal() + timing_.end_delay, 0.0);
}

void AnimationEffect::UpdateSpecifiedTiming(const Timing& timing) {
  // FIXME: Test whether the timing is actually different?
  timing_ = timing;
  InvalidateAndNotifyOwner();
}

void AnimationEffect::getTiming(EffectTiming& effect_timing) const {
  effect_timing.setDelay(SpecifiedTiming().start_delay * 1000);
  effect_timing.setEndDelay(SpecifiedTiming().end_delay * 1000);
  effect_timing.setFill(Timing::FillModeString(SpecifiedTiming().fill_mode));
  effect_timing.setIterationStart(SpecifiedTiming().iteration_start);
  effect_timing.setIterations(SpecifiedTiming().iteration_count);
  UnrestrictedDoubleOrString duration;
  if (IsNull(SpecifiedTiming().iteration_duration)) {
    duration.SetString("auto");
  } else {
    duration.SetUnrestrictedDouble(SpecifiedTiming().iteration_duration * 1000);
  }
  effect_timing.setDuration(duration);
  effect_timing.setDirection(
      Timing::PlaybackDirectionString(SpecifiedTiming().direction));
  effect_timing.setEasing(SpecifiedTiming().timing_function->ToString());
}

EffectTiming AnimationEffect::getTiming() const {
  EffectTiming result;
  getTiming(result);
  return result;
}

void AnimationEffect::getComputedTiming(
    ComputedEffectTiming& computed_timing) const {
  // ComputedEffectTiming members.
  computed_timing.setEndTime(EndTimeInternal() * 1000);
  computed_timing.setActiveDuration(ActiveDurationInternal() * 1000);

  if (IsNull(EnsureCalculated().local_time)) {
    computed_timing.setLocalTimeToNull();
  } else {
    computed_timing.setLocalTime(EnsureCalculated().local_time * 1000);
  }

  if (EnsureCalculated().is_in_effect) {
    computed_timing.setProgress(EnsureCalculated().progress.value());
    computed_timing.setCurrentIteration(EnsureCalculated().current_iteration);
  } else {
    computed_timing.setProgressToNull();
    computed_timing.setCurrentIterationToNull();
  }

  // For the EffectTiming members, getComputedTiming is equivalent to getTiming
  // except that the fill and duration must be resolved.
  //
  // https://drafts.csswg.org/web-animations-1/#dom-animationeffect-getcomputedtiming
  computed_timing.setDelay(SpecifiedTiming().start_delay * 1000);
  computed_timing.setEndDelay(SpecifiedTiming().end_delay * 1000);
  computed_timing.setFill(Timing::FillModeString(
      ResolvedFillMode(SpecifiedTiming().fill_mode, IsKeyframeEffect())));
  computed_timing.setIterationStart(SpecifiedTiming().iteration_start);
  computed_timing.setIterations(SpecifiedTiming().iteration_count);

  UnrestrictedDoubleOrString duration;
  duration.SetUnrestrictedDouble(IterationDuration() * 1000);
  computed_timing.setDuration(duration);

  computed_timing.setDirection(
      Timing::PlaybackDirectionString(SpecifiedTiming().direction));
  computed_timing.setEasing(SpecifiedTiming().timing_function->ToString());
}

ComputedEffectTiming AnimationEffect::getComputedTiming() const {
  ComputedEffectTiming result;
  getComputedTiming(result);
  return result;
}

void AnimationEffect::updateTiming(OptionalEffectTiming& optional_timing,
                                   ExceptionState& exception_state) {
  // TODO(crbug.com/827178): Determine whether we should pass a Document in here
  // (and which) to resolve the CSS secure/insecure context against.
  if (!TimingInput::Update(timing_, optional_timing, nullptr, exception_state))
    return;
  InvalidateAndNotifyOwner();
}

void AnimationEffect::UpdateInheritedTime(double inherited_time,
                                          TimingUpdateReason reason) const {
  bool needs_update =
      needs_update_ ||
      (last_update_time_ != inherited_time &&
       !(IsNull(last_update_time_) && IsNull(inherited_time))) ||
      (owner_ && owner_->EffectSuppressed());
  needs_update_ = false;
  last_update_time_ = inherited_time;

  const double local_time = inherited_time;
  double time_to_next_iteration = std::numeric_limits<double>::infinity();
  if (needs_update) {
    const double active_duration = this->ActiveDurationInternal();

    const Phase current_phase =
        CalculatePhase(active_duration, local_time, timing_);
    // FIXME: parentPhase depends on groups being implemented.
    const AnimationEffect::Phase kParentPhase = AnimationEffect::kPhaseActive;
    const double active_time = CalculateActiveTime(
        active_duration,
        ResolvedFillMode(timing_.fill_mode, IsKeyframeEffect()), local_time,
        kParentPhase, current_phase, timing_);

    double current_iteration;
    base::Optional<double> progress;
    if (const double iteration_duration = this->IterationDuration()) {
      const double start_offset = MultiplyZeroAlwaysGivesZero(
          timing_.iteration_start, iteration_duration);
      DCHECK_GE(start_offset, 0);
      const double scaled_active_time = CalculateScaledActiveTime(
          active_duration, active_time, start_offset, timing_);
      const double iteration_time = CalculateIterationTime(
          iteration_duration, RepeatedDuration(), scaled_active_time,
          start_offset, current_phase, timing_);

      current_iteration = CalculateCurrentIteration(
          iteration_duration, iteration_time, scaled_active_time, timing_);
      const base::Optional<double> transformed_time = CalculateTransformedTime(
          current_iteration, iteration_duration, iteration_time, timing_);

      // The infinite iterationDuration case here is a workaround because
      // the specified behaviour does not handle infinite durations well.
      // There is an open issue against the spec to fix this:
      // https://github.com/w3c/web-animations/issues/142
      if (!std::isfinite(iteration_duration))
        progress = fmod(timing_.iteration_start, 1.0);
      else if (transformed_time)
        progress = transformed_time.value() / iteration_duration;

      if (!IsNull(iteration_time)) {
        time_to_next_iteration = (iteration_duration - iteration_time) /
                                 std::abs(timing_.playback_rate);
        if (active_duration - active_time < time_to_next_iteration)
          time_to_next_iteration = std::numeric_limits<double>::infinity();
      }
    } else {
      const double kLocalIterationDuration = 1;
      const double local_repeated_duration =
          kLocalIterationDuration * timing_.iteration_count;
      DCHECK_GE(local_repeated_duration, 0);
      const double local_active_duration =
          timing_.playback_rate
              ? local_repeated_duration / std::abs(timing_.playback_rate)
              : std::numeric_limits<double>::infinity();
      DCHECK_GE(local_active_duration, 0);
      const double local_local_time =
          local_time < timing_.start_delay
              ? local_time
              : local_active_duration + timing_.start_delay;
      const AnimationEffect::Phase local_current_phase =
          CalculatePhase(local_active_duration, local_local_time, timing_);
      const double local_active_time = CalculateActiveTime(
          local_active_duration,
          ResolvedFillMode(timing_.fill_mode, IsKeyframeEffect()),
          local_local_time, kParentPhase, local_current_phase, timing_);
      const double start_offset =
          timing_.iteration_start * kLocalIterationDuration;
      DCHECK_GE(start_offset, 0);
      const double scaled_active_time = CalculateScaledActiveTime(
          local_active_duration, local_active_time, start_offset, timing_);
      const double iteration_time = CalculateIterationTime(
          kLocalIterationDuration, local_repeated_duration, scaled_active_time,
          start_offset, current_phase, timing_);

      current_iteration = CalculateCurrentIteration(
          kLocalIterationDuration, iteration_time, scaled_active_time, timing_);
      progress = CalculateTransformedTime(
          current_iteration, kLocalIterationDuration, iteration_time, timing_);
    }

    calculated_.current_iteration = current_iteration;
    calculated_.progress = progress;

    calculated_.phase = current_phase;
    calculated_.is_in_effect = !IsNull(active_time);
    calculated_.is_in_play = GetPhase() == kPhaseActive;
    calculated_.is_current = GetPhase() == kPhaseBefore || IsInPlay();
    calculated_.local_time = last_update_time_;
  }

  // Test for events even if timing didn't need an update as the animation may
  // have gained a start time.
  // FIXME: Refactor so that we can DCHECK(owner_) here, this is currently
  // required to be nullable for testing.
  if (reason == kTimingUpdateForAnimationFrame &&
      (!owner_ || owner_->IsEventDispatchAllowed())) {
    if (event_delegate_)
      event_delegate_->OnEventCondition(*this);
  }

  if (needs_update) {
    // FIXME: This probably shouldn't be recursive.
    UpdateChildrenAndEffects();
    calculated_.time_to_forwards_effect_change =
        CalculateTimeToEffectChange(true, local_time, time_to_next_iteration);
    calculated_.time_to_reverse_effect_change =
        CalculateTimeToEffectChange(false, local_time, time_to_next_iteration);
  }
}

void AnimationEffect::InvalidateAndNotifyOwner() const {
  Invalidate();
  if (owner_)
    owner_->EffectInvalidated();
}

const AnimationEffect::CalculatedTiming& AnimationEffect::EnsureCalculated()
    const {
  if (!owner_)
    return calculated_;

  owner_->UpdateIfNecessary();
  return calculated_;
}

Animation* AnimationEffect::GetAnimation() {
  return owner_ ? owner_->GetAnimation() : nullptr;
}
const Animation* AnimationEffect::GetAnimation() const {
  return owner_ ? owner_->GetAnimation() : nullptr;
}

void AnimationEffect::Trace(blink::Visitor* visitor) {
  visitor->Trace(owner_);
  visitor->Trace(event_delegate_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
