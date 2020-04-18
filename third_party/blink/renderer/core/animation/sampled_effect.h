// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SAMPLED_EFFECT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SAMPLED_EFFECT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/animation/animation.h"
#include "third_party/blink/renderer/core/animation/interpolation.h"
#include "third_party/blink/renderer/core/animation/keyframe_effect.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Associates the results of sampling an EffectModel with metadata used for
// effect ordering and managing composited animations.
class SampledEffect : public GarbageCollectedFinalized<SampledEffect> {
 public:
  static SampledEffect* Create(KeyframeEffect* effect,
                               unsigned sequence_number) {
    return new SampledEffect(effect, sequence_number);
  }

  void Clear();

  const Vector<scoped_refptr<Interpolation>>& Interpolations() const {
    return interpolations_;
  }
  Vector<scoped_refptr<Interpolation>>& MutableInterpolations() {
    return interpolations_;
  }

  KeyframeEffect* Effect() const { return effect_; }
  unsigned SequenceNumber() const { return sequence_number_; }
  KeyframeEffect::Priority GetPriority() const { return priority_; }
  bool WillNeverChange() const;
  void RemoveReplacedInterpolations(const HashSet<PropertyHandle>&);
  void UpdateReplacedProperties(HashSet<PropertyHandle>&);

  void Trace(blink::Visitor*);

 private:
  SampledEffect(KeyframeEffect*, unsigned sequence_number);

  WeakMember<KeyframeEffect> effect_;
  Vector<scoped_refptr<Interpolation>> interpolations_;
  const unsigned sequence_number_;
  KeyframeEffect::Priority priority_;
  DISALLOW_COPY_AND_ASSIGN(SampledEffect);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_SAMPLED_EFFECT_H_
