// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/animation/animation_test_helper.h"
#include "third_party/blink/renderer/core/animation/css_length_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/css_number_interpolation_type.h"
#include "third_party/blink/renderer/core/animation/interpolable_value.h"
#include "third_party/blink/renderer/core/animation/interpolation_value.h"
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/animation/transition_interpolation.h"

namespace blink {

class AnimationInterpolableValueTest : public testing::Test {
 protected:
  double InterpolateNumbers(int a, int b, double progress) {
    // We require a property that maps to CSSNumberInterpolationType. 'z-index'
    // suffices for this, and also means we can ignore the AnimatableValues for
    // the compositor (as z-index isn't compositor-compatible).
    PropertyHandle property_handle(GetCSSPropertyZIndex());
    CSSNumberInterpolationType interpolation_type(property_handle);
    InterpolationValue start(InterpolableNumber::Create(a));
    InterpolationValue end(InterpolableNumber::Create(b));
    scoped_refptr<TransitionInterpolation> i = TransitionInterpolation::Create(
        property_handle, interpolation_type, std::move(start), std::move(end),
        nullptr, nullptr);

    i->Interpolate(0, progress);
    std::unique_ptr<TypedInterpolationValue> interpolated_value =
        i->GetInterpolatedValue();
    EXPECT_TRUE(interpolated_value);
    return ToInterpolableNumber(interpolated_value->GetInterpolableValue())
        .Value();
  }

  void ScaleAndAdd(InterpolableValue& base,
                   double scale,
                   const InterpolableValue& add) {
    base.ScaleAndAdd(scale, add);
  }

  std::unique_ptr<TypedInterpolationValue> InterpolateLists(
      std::unique_ptr<InterpolableList> list_a,
      std::unique_ptr<InterpolableList> list_b,
      double progress) {
    // We require a property that maps to CSSLengthInterpolationType. 'left'
    // suffices for this, and also means we can ignore the AnimatableValues for
    // the compositor (as left isn't compositor-compatible).
    PropertyHandle property_handle(GetCSSPropertyLeft());
    CSSLengthInterpolationType interpolation_type(property_handle);
    InterpolationValue start(std::move(list_a));
    InterpolationValue end(std::move(list_b));
    scoped_refptr<TransitionInterpolation> i = TransitionInterpolation::Create(
        property_handle, interpolation_type, std::move(start), std::move(end),
        nullptr, nullptr);
    i->Interpolate(0, progress);
    return i->GetInterpolatedValue();
  }
};

TEST_F(AnimationInterpolableValueTest, InterpolateNumbers) {
  EXPECT_FLOAT_EQ(126, InterpolateNumbers(42, 0, -2));
  EXPECT_FLOAT_EQ(42, InterpolateNumbers(42, 0, 0));
  EXPECT_FLOAT_EQ(29.4f, InterpolateNumbers(42, 0, 0.3));
  EXPECT_FLOAT_EQ(21, InterpolateNumbers(42, 0, 0.5));
  EXPECT_FLOAT_EQ(0, InterpolateNumbers(42, 0, 1));
  EXPECT_FLOAT_EQ(-21, InterpolateNumbers(42, 0, 1.5));
}

TEST_F(AnimationInterpolableValueTest, SimpleList) {
  std::unique_ptr<InterpolableList> list_a = InterpolableList::Create(3);
  list_a->Set(0, InterpolableNumber::Create(0));
  list_a->Set(1, InterpolableNumber::Create(42));
  list_a->Set(2, InterpolableNumber::Create(20.5));

  std::unique_ptr<InterpolableList> list_b = InterpolableList::Create(3);
  list_b->Set(0, InterpolableNumber::Create(100));
  list_b->Set(1, InterpolableNumber::Create(-200));
  list_b->Set(2, InterpolableNumber::Create(300));

  std::unique_ptr<TypedInterpolationValue> interpolated_value =
      InterpolateLists(std::move(list_a), std::move(list_b), 0.3);
  const InterpolableList& out_list =
      ToInterpolableList(interpolated_value->GetInterpolableValue());

  EXPECT_FLOAT_EQ(30, ToInterpolableNumber(out_list.Get(0))->Value());
  EXPECT_FLOAT_EQ(-30.6f, ToInterpolableNumber(out_list.Get(1))->Value());
  EXPECT_FLOAT_EQ(104.35f, ToInterpolableNumber(out_list.Get(2))->Value());
}

TEST_F(AnimationInterpolableValueTest, NestedList) {
  std::unique_ptr<InterpolableList> list_a = InterpolableList::Create(3);
  list_a->Set(0, InterpolableNumber::Create(0));
  std::unique_ptr<InterpolableList> sub_list_a = InterpolableList::Create(1);
  sub_list_a->Set(0, InterpolableNumber::Create(100));
  list_a->Set(1, std::move(sub_list_a));
  list_a->Set(2, InterpolableNumber::Create(0));

  std::unique_ptr<InterpolableList> list_b = InterpolableList::Create(3);
  list_b->Set(0, InterpolableNumber::Create(100));
  std::unique_ptr<InterpolableList> sub_list_b = InterpolableList::Create(1);
  sub_list_b->Set(0, InterpolableNumber::Create(50));
  list_b->Set(1, std::move(sub_list_b));
  list_b->Set(2, InterpolableNumber::Create(1));

  std::unique_ptr<TypedInterpolationValue> interpolated_value =
      InterpolateLists(std::move(list_a), std::move(list_b), 0.5);
  const InterpolableList& out_list =
      ToInterpolableList(interpolated_value->GetInterpolableValue());

  EXPECT_FLOAT_EQ(50, ToInterpolableNumber(out_list.Get(0))->Value());
  EXPECT_FLOAT_EQ(
      75, ToInterpolableNumber(ToInterpolableList(out_list.Get(1))->Get(0))
              ->Value());
  EXPECT_FLOAT_EQ(0.5, ToInterpolableNumber(out_list.Get(2))->Value());
}

TEST_F(AnimationInterpolableValueTest, ScaleAndAddNumbers) {
  std::unique_ptr<InterpolableNumber> base = InterpolableNumber::Create(10);
  ScaleAndAdd(*base, 2, *InterpolableNumber::Create(1));
  EXPECT_FLOAT_EQ(21, base->Value());

  base = InterpolableNumber::Create(10);
  ScaleAndAdd(*base, 0, *InterpolableNumber::Create(5));
  EXPECT_FLOAT_EQ(5, base->Value());

  base = InterpolableNumber::Create(10);
  ScaleAndAdd(*base, -1, *InterpolableNumber::Create(8));
  EXPECT_FLOAT_EQ(-2, base->Value());
}

TEST_F(AnimationInterpolableValueTest, ScaleAndAddLists) {
  std::unique_ptr<InterpolableList> base_list = InterpolableList::Create(3);
  base_list->Set(0, InterpolableNumber::Create(5));
  base_list->Set(1, InterpolableNumber::Create(10));
  base_list->Set(2, InterpolableNumber::Create(15));
  std::unique_ptr<InterpolableList> add_list = InterpolableList::Create(3);
  add_list->Set(0, InterpolableNumber::Create(1));
  add_list->Set(1, InterpolableNumber::Create(2));
  add_list->Set(2, InterpolableNumber::Create(3));
  ScaleAndAdd(*base_list, 2, *add_list);
  EXPECT_FLOAT_EQ(11, ToInterpolableNumber(base_list->Get(0))->Value());
  EXPECT_FLOAT_EQ(22, ToInterpolableNumber(base_list->Get(1))->Value());
  EXPECT_FLOAT_EQ(33, ToInterpolableNumber(base_list->Get(2))->Value());
}

}  // namespace blink
