// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/svg_number_optional_number_interpolation_type.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/interpolation_environment.h"
#include "third_party/blink/renderer/core/svg/svg_number_optional_number.h"

namespace blink {

InterpolationValue
SVGNumberOptionalNumberInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  std::unique_ptr<InterpolableList> result = InterpolableList::Create(2);
  result->Set(0, InterpolableNumber::Create(0));
  result->Set(1, InterpolableNumber::Create(0));
  return InterpolationValue(std::move(result));
}

InterpolationValue
SVGNumberOptionalNumberInterpolationType::MaybeConvertSVGValue(
    const SVGPropertyBase& svg_value) const {
  if (svg_value.GetType() != kAnimatedNumberOptionalNumber)
    return nullptr;

  const SVGNumberOptionalNumber& number_optional_number =
      ToSVGNumberOptionalNumber(svg_value);
  std::unique_ptr<InterpolableList> result = InterpolableList::Create(2);
  result->Set(0, InterpolableNumber::Create(
                     number_optional_number.FirstNumber()->Value()));
  result->Set(1, InterpolableNumber::Create(
                     number_optional_number.SecondNumber()->Value()));
  return InterpolationValue(std::move(result));
}

SVGPropertyBase* SVGNumberOptionalNumberInterpolationType::AppliedSVGValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*) const {
  const InterpolableList& list = ToInterpolableList(interpolable_value);
  return SVGNumberOptionalNumber::Create(
      SVGNumber::Create(ToInterpolableNumber(list.Get(0))->Value()),
      SVGNumber::Create(ToInterpolableNumber(list.Get(1))->Value()));
}

}  // namespace blink
