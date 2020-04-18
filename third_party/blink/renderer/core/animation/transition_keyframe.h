// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TRANSITION_KEYFRAME_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_TRANSITION_KEYFRAME_H_

#include "third_party/blink/renderer/core/animation/animatable/animatable_value.h"
#include "third_party/blink/renderer/core/animation/keyframe.h"
#include "third_party/blink/renderer/core/animation/typed_interpolation_value.h"
#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

// An implementation of Keyframe specifically for CSS Transitions.
//
// TransitionKeyframes are a simple form of keyframe, which only have one
// (property, value) pair. CSS Transitions do not support SVG attributes, so the
// property will always be a CSSPropertyID (for CSS properties and presentation
// attributes) or an AtomicString (for custom CSS properties).
class CORE_EXPORT TransitionKeyframe : public Keyframe {
 public:
  static scoped_refptr<TransitionKeyframe> Create(
      const PropertyHandle& property) {
    DCHECK(!property.IsSVGAttribute());
    return base::AdoptRef(new TransitionKeyframe(property));
  }
  void SetValue(std::unique_ptr<TypedInterpolationValue> value) {
    // Speculative CHECK to help investigate crbug.com/826627. The theory is
    // that |SetValue| is being called with a |value| that has no underlying
    // InterpolableValue. This then would later cause a crash in the
    // TransitionInterpolation constructor.
    // TODO(crbug.com/826627): Revert once bug is fixed.
    CHECK(!!value->Value());
    value_ = std::move(value);
  }
  void SetCompositorValue(scoped_refptr<AnimatableValue>);
  PropertyHandleSet Properties() const final;

  void AddKeyframePropertiesToV8Object(V8ObjectBuilder&) const override;

  class PropertySpecificKeyframe : public Keyframe::PropertySpecificKeyframe {
   public:
    static scoped_refptr<PropertySpecificKeyframe> Create(
        double offset,
        scoped_refptr<TimingFunction> easing,
        EffectModel::CompositeOperation composite,
        std::unique_ptr<TypedInterpolationValue> value,
        scoped_refptr<AnimatableValue> compositor_value) {
      return base::AdoptRef(new PropertySpecificKeyframe(
          offset, std::move(easing), composite, std::move(value),
          std::move(compositor_value)));
    }

    const AnimatableValue* GetAnimatableValue() const final {
      return compositor_value_.get();
    }

    bool IsNeutral() const final { return false; }
    scoped_refptr<Keyframe::PropertySpecificKeyframe> NeutralKeyframe(
        double offset,
        scoped_refptr<TimingFunction> easing) const final {
      NOTREACHED();
      return nullptr;
    }
    scoped_refptr<Interpolation> CreateInterpolation(
        const PropertyHandle&,
        const Keyframe::PropertySpecificKeyframe& other) const final;

    bool IsTransitionPropertySpecificKeyframe() const final { return true; }

   private:
    PropertySpecificKeyframe(double offset,
                             scoped_refptr<TimingFunction> easing,
                             EffectModel::CompositeOperation composite,
                             std::unique_ptr<TypedInterpolationValue> value,
                             scoped_refptr<AnimatableValue> compositor_value)
        : Keyframe::PropertySpecificKeyframe(offset,
                                             std::move(easing),
                                             composite),
          value_(std::move(value)),
          compositor_value_(std::move(compositor_value)) {}

    scoped_refptr<Keyframe::PropertySpecificKeyframe> CloneWithOffset(
        double offset) const final {
      return Create(offset, easing_, composite_, value_->Clone(),
                    compositor_value_);
    }

    std::unique_ptr<TypedInterpolationValue> value_;
    scoped_refptr<AnimatableValue> compositor_value_;
  };

 private:
  TransitionKeyframe(const PropertyHandle& property) : property_(property) {}

  TransitionKeyframe(const TransitionKeyframe& copy_from)
      : Keyframe(copy_from.offset_, copy_from.composite_, copy_from.easing_),
        property_(copy_from.property_),
        value_(copy_from.value_->Clone()),
        compositor_value_(copy_from.compositor_value_) {}

  bool IsTransitionKeyframe() const final { return true; }

  scoped_refptr<Keyframe> Clone() const final {
    return base::AdoptRef(new TransitionKeyframe(*this));
  }

  scoped_refptr<Keyframe::PropertySpecificKeyframe>
  CreatePropertySpecificKeyframe(
      const PropertyHandle&,
      EffectModel::CompositeOperation effect_composite,
      double offset) const final;

  PropertyHandle property_;
  std::unique_ptr<TypedInterpolationValue> value_;
  scoped_refptr<AnimatableValue> compositor_value_;
};

using TransitionPropertySpecificKeyframe =
    TransitionKeyframe::PropertySpecificKeyframe;

DEFINE_TYPE_CASTS(TransitionKeyframe,
                  Keyframe,
                  value,
                  value->IsTransitionKeyframe(),
                  value.IsTransitionKeyframe());
DEFINE_TYPE_CASTS(TransitionPropertySpecificKeyframe,
                  Keyframe::PropertySpecificKeyframe,
                  value,
                  value->IsTransitionPropertySpecificKeyframe(),
                  value.IsTransitionPropertySpecificKeyframe());

}  // namespace blink

#endif
