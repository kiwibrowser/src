// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/animation/animation_test_helper.h"
#include "third_party/blink/renderer/core/animation/css_number_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/interpolation_effect.h"
#include "third_party/blink/renderer/core/animation/transition_interpolation.h"

namespace blink {

namespace {

const double kInterpolationTestDuration = 1.0;

double GetInterpolableNumber(scoped_refptr<Interpolation> value) {
  TransitionInterpolation& interpolation =
      ToTransitionInterpolation(*value.get());
  std::unique_ptr<TypedInterpolationValue> interpolated_value =
      interpolation.GetInterpolatedValue();
  return ToInterpolableNumber(interpolated_value->GetInterpolableValue())
      .Value();
}

scoped_refptr<Interpolation> CreateInterpolation(int from, int to) {
  // We require a property that maps to CSSNumberInterpolationType. 'z-index'
  // suffices for this, and also means we can ignore the AnimatableValues for
  // the compositor (as z-index isn't compositor-compatible).
  PropertyHandle property_handle(GetCSSPropertyZIndex());
  CSSNumberInterpolationType interpolation_type(property_handle);
  InterpolationValue start(InterpolableNumber::Create(from));
  InterpolationValue end(InterpolableNumber::Create(to));
  return TransitionInterpolation::Create(property_handle, interpolation_type,
                                         std::move(start), std::move(end),
                                         nullptr, nullptr);
}

}  // namespace

TEST(AnimationInterpolationEffectTest, SingleInterpolation) {
  InterpolationEffect interpolation_effect;
  interpolation_effect.AddInterpolation(
      CreateInterpolation(0, 10), scoped_refptr<TimingFunction>(), 0, 1, -1, 2);

  Vector<scoped_refptr<Interpolation>> active_interpolations;
  interpolation_effect.GetActiveInterpolations(-2, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(0ul, active_interpolations.size());

  interpolation_effect.GetActiveInterpolations(-0.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_EQ(-5, GetInterpolableNumber(active_interpolations.at(0)));

  interpolation_effect.GetActiveInterpolations(0.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(5, GetInterpolableNumber(active_interpolations.at(0)));

  interpolation_effect.GetActiveInterpolations(1.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(15, GetInterpolableNumber(active_interpolations.at(0)));

  interpolation_effect.GetActiveInterpolations(3, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(0ul, active_interpolations.size());
}

TEST(AnimationInterpolationEffectTest, MultipleInterpolations) {
  InterpolationEffect interpolation_effect;
  interpolation_effect.AddInterpolation(
      CreateInterpolation(10, 15), scoped_refptr<TimingFunction>(), 1, 2, 1, 3);
  interpolation_effect.AddInterpolation(
      CreateInterpolation(0, 1), LinearTimingFunction::Shared(), 0, 1, 0, 1);
  interpolation_effect.AddInterpolation(
      CreateInterpolation(1, 6),
      CubicBezierTimingFunction::Preset(
          CubicBezierTimingFunction::EaseType::EASE),
      0.5, 1.5, 0.5, 1.5);

  Vector<scoped_refptr<Interpolation>> active_interpolations;
  interpolation_effect.GetActiveInterpolations(-0.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(0ul, active_interpolations.size());

  interpolation_effect.GetActiveInterpolations(0, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(0, GetInterpolableNumber(active_interpolations.at(0)));

  interpolation_effect.GetActiveInterpolations(0.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(2ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(0.5f, GetInterpolableNumber(active_interpolations.at(0)));
  EXPECT_FLOAT_EQ(1, GetInterpolableNumber(active_interpolations.at(1)));

  interpolation_effect.GetActiveInterpolations(1, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(2ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(10, GetInterpolableNumber(active_interpolations.at(0)));
  EXPECT_FLOAT_EQ(5.0282884f,
                  GetInterpolableNumber(active_interpolations.at(1)));

  interpolation_effect.GetActiveInterpolations(
      1, kInterpolationTestDuration * 1000, active_interpolations);
  EXPECT_EQ(2ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(10, GetInterpolableNumber(active_interpolations.at(0)));
  EXPECT_FLOAT_EQ(5.0120168f,
                  GetInterpolableNumber(active_interpolations.at(1)));

  interpolation_effect.GetActiveInterpolations(1.5, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(12.5f, GetInterpolableNumber(active_interpolations.at(0)));

  interpolation_effect.GetActiveInterpolations(2, kInterpolationTestDuration,
                                               active_interpolations);
  EXPECT_EQ(1ul, active_interpolations.size());
  EXPECT_FLOAT_EQ(15, GetInterpolableNumber(active_interpolations.at(0)));
}

}  // namespace blink
