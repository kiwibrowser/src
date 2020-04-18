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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_EFFECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_EFFECT_H_

#include "base/optional.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/animation/timing.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Animation;
class AnimationEffectOwner;
class EffectTiming;
class ComputedEffectTiming;
class OptionalEffectTiming;
class WorkletAnimation;

enum TimingUpdateReason {
  kTimingUpdateOnDemand,
  kTimingUpdateForAnimationFrame
};

static inline bool IsNull(double value) {
  return std::isnan(value);
}

static inline double NullValue() {
  return std::numeric_limits<double>::quiet_NaN();
}

// Represents the content of an Animation and its fractional timing state.
// https://drafts.csswg.org/web-animations/#the-animationeffect-interface
class CORE_EXPORT AnimationEffect : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();
  // Calls Attach/Detach, GetAnimation, UpdateInheritedTime.
  friend class Animation;
  friend class WorkletAnimation;

  // Calls GetAnimation().
  // TODO(majidvp): Remove this. EffectStack should not need to access animation
  // directly.
  friend class EffectStack;

 public:
  // Note that logic in CSSAnimations depends on the order of these values.
  enum Phase {
    kPhaseBefore,
    kPhaseActive,
    kPhaseAfter,
    kPhaseNone,
  };

  class EventDelegate : public GarbageCollectedFinalized<EventDelegate> {
   public:
    virtual ~EventDelegate() = default;
    virtual bool RequiresIterationEvents(const AnimationEffect&) = 0;
    virtual void OnEventCondition(const AnimationEffect&) = 0;
    virtual void Trace(blink::Visitor* visitor) {}
  };

  ~AnimationEffect() override = default;

  virtual bool IsKeyframeEffect() const { return false; }
  virtual bool IsInertEffect() const { return false; }

  Phase GetPhase() const { return EnsureCalculated().phase; }
  bool IsCurrent() const { return EnsureCalculated().is_current; }
  bool IsInEffect() const { return EnsureCalculated().is_in_effect; }
  bool IsInPlay() const { return EnsureCalculated().is_in_play; }
  double CurrentIteration() const {
    return EnsureCalculated().current_iteration;
  }
  base::Optional<double> Progress() const {
    return EnsureCalculated().progress;
  }
  double TimeToForwardsEffectChange() const {
    return EnsureCalculated().time_to_forwards_effect_change;
  }
  double TimeToReverseEffectChange() const {
    return EnsureCalculated().time_to_reverse_effect_change;
  }

  double IterationDuration() const;
  double ActiveDurationInternal() const;
  double EndTimeInternal() const;

  const Timing& SpecifiedTiming() const { return timing_; }
  void UpdateSpecifiedTiming(const Timing&);
  EventDelegate* GetEventDelegate() { return event_delegate_; }

  void getTiming(EffectTiming&) const;
  EffectTiming getTiming() const;
  void getComputedTiming(ComputedEffectTiming&) const;
  ComputedEffectTiming getComputedTiming() const;
  void updateTiming(OptionalEffectTiming&,
                    ExceptionState& = ASSERT_NO_EXCEPTION);

  // Attach/Detach the AnimationEffect from its owning animation.
  virtual void Attach(AnimationEffectOwner* owner) { owner_ = owner; }
  virtual void Detach() {
    DCHECK(owner_);
    owner_ = nullptr;
  }

  const Animation* GetAnimationForTesting() const { return GetAnimation(); }

  void Trace(blink::Visitor*) override;

 protected:
  explicit AnimationEffect(const Timing&, EventDelegate* = nullptr);

  // When AnimationEffect receives a new inherited time via updateInheritedTime
  // it will (if necessary) recalculate timings and (if necessary) call
  // updateChildrenAndEffects.
  void UpdateInheritedTime(double inherited_time, TimingUpdateReason) const;
  void Invalidate() const { needs_update_ = true; }
  void InvalidateAndNotifyOwner() const;
  bool RequiresIterationEvents() const {
    return event_delegate_ && event_delegate_->RequiresIterationEvents(*this);
  }
  void ClearEventDelegate() { event_delegate_ = nullptr; }

  double RepeatedDuration() const;

  virtual void UpdateChildrenAndEffects() const = 0;
  virtual double IntrinsicIterationDuration() const { return 0; }
  virtual double CalculateTimeToEffectChange(
      bool forwards,
      double local_time,
      double time_to_next_iteration) const = 0;

  const Animation* GetAnimation() const;
  Animation* GetAnimation();

  Member<AnimationEffectOwner> owner_;
  Timing timing_;
  Member<EventDelegate> event_delegate_;

  mutable struct CalculatedTiming {
    DISALLOW_NEW();
    Phase phase;
    double current_iteration;
    base::Optional<double> progress;
    bool is_current;
    bool is_in_effect;
    bool is_in_play;
    double local_time = NullValue();
    double time_to_forwards_effect_change;
    double time_to_reverse_effect_change;
  } calculated_;
  mutable bool needs_update_;
  mutable double last_update_time_;
  String name_;

  const CalculatedTiming& EnsureCalculated() const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_ANIMATION_EFFECT_H_
