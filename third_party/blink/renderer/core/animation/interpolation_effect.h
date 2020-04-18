// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_INTERPOLATION_EFFECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_INTERPOLATION_EFFECT_H_

#include "third_party/blink/renderer/core/animation/interpolation.h"
#include "third_party/blink/renderer/core/animation/keyframe.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/animation/timing_function.h"

namespace blink {

// Stores all adjacent pairs of keyframes (represented by Interpolations) in a
// KeyframeEffectModel with keyframe offset data preprocessed for more efficient
// active keyframe pair sampling.
class CORE_EXPORT InterpolationEffect {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  InterpolationEffect() : is_populated_(false) {}

  bool IsPopulated() const { return is_populated_; }
  void SetPopulated() { is_populated_ = true; }

  void Clear() {
    is_populated_ = false;
    interpolations_.clear();
  }

  void GetActiveInterpolations(double fraction,
                               double iteration_duration,
                               Vector<scoped_refptr<Interpolation>>&) const;

  void AddInterpolation(scoped_refptr<Interpolation> interpolation,
                        scoped_refptr<TimingFunction> easing,
                        double start,
                        double end,
                        double apply_from,
                        double apply_to) {
    interpolations_.push_back(InterpolationRecord(std::move(interpolation),
                                                  std::move(easing), start, end,
                                                  apply_from, apply_to));
  }

  void AddInterpolationsFromKeyframes(
      const PropertyHandle&,
      const Keyframe::PropertySpecificKeyframe& keyframe_a,
      const Keyframe::PropertySpecificKeyframe& keyframe_b,
      double apply_from,
      double apply_to);

 private:
  struct InterpolationRecord {
    InterpolationRecord(scoped_refptr<Interpolation> interpolation,
                        scoped_refptr<TimingFunction> easing,
                        double start,
                        double end,
                        double apply_from,
                        double apply_to)
        : interpolation_(std::move(interpolation)),
          easing_(std::move(easing)),
          start_(start),
          end_(end),
          apply_from_(apply_from),
          apply_to_(apply_to) {}

    scoped_refptr<Interpolation> interpolation_;
    scoped_refptr<TimingFunction> easing_;
    double start_;
    double end_;
    double apply_from_;
    double apply_to_;
  };

  bool is_populated_;
  Vector<InterpolationRecord> interpolations_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_INTERPOLATION_EFFECT_H_
