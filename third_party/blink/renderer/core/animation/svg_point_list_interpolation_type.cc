// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/svg_point_list_interpolation_type.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/interpolation_environment.h"
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/animation/underlying_length_checker.h"
#include "third_party/blink/renderer/core/svg/svg_point_list.h"

namespace blink {

InterpolationValue SVGPointListInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  size_t underlying_length =
      UnderlyingLengthChecker::GetUnderlyingLength(underlying);
  conversion_checkers.push_back(
      UnderlyingLengthChecker::Create(underlying_length));

  if (underlying_length == 0)
    return nullptr;

  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(underlying_length);
  for (size_t i = 0; i < underlying_length; i++)
    result->Set(i, InterpolableNumber::Create(0));
  return InterpolationValue(std::move(result));
}

InterpolationValue SVGPointListInterpolationType::MaybeConvertSVGValue(
    const SVGPropertyBase& svg_value) const {
  if (svg_value.GetType() != kAnimatedPoints)
    return nullptr;

  const SVGPointList& point_list = ToSVGPointList(svg_value);
  std::unique_ptr<InterpolableList> result =
      InterpolableList::Create(point_list.length() * 2);
  for (size_t i = 0; i < point_list.length(); i++) {
    const SVGPoint& point = *point_list.at(i);
    result->Set(2 * i, InterpolableNumber::Create(point.X()));
    result->Set(2 * i + 1, InterpolableNumber::Create(point.Y()));
  }

  return InterpolationValue(std::move(result));
}

PairwiseInterpolationValue SVGPointListInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  size_t start_length = ToInterpolableList(*start.interpolable_value).length();
  size_t end_length = ToInterpolableList(*end.interpolable_value).length();
  if (start_length != end_length)
    return nullptr;

  return InterpolationType::MaybeMergeSingles(std::move(start), std::move(end));
}

void SVGPointListInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  size_t start_length =
      ToInterpolableList(*underlying_value_owner.Value().interpolable_value)
          .length();
  size_t end_length = ToInterpolableList(*value.interpolable_value).length();
  if (start_length == end_length)
    InterpolationType::Composite(underlying_value_owner, underlying_fraction,
                                 value, interpolation_fraction);
  else
    underlying_value_owner.Set(*this, value);
}

SVGPropertyBase* SVGPointListInterpolationType::AppliedSVGValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*) const {
  SVGPointList* result = SVGPointList::Create();

  const InterpolableList& list = ToInterpolableList(interpolable_value);
  DCHECK_EQ(list.length() % 2, 0U);
  for (size_t i = 0; i < list.length(); i += 2) {
    FloatPoint point =
        FloatPoint(ToInterpolableNumber(list.Get(i))->Value(),
                   ToInterpolableNumber(list.Get(i + 1))->Value());
    result->Append(SVGPoint::Create(point));
  }

  return result;
}

}  // namespace blink
