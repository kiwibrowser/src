// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/svg_number_interpolation_type.h"

#include "third_party/blink/renderer/core/animation/interpolation_environment.h"
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/svg/properties/svg_animated_property.h"
#include "third_party/blink/renderer/core/svg/svg_number.h"

namespace blink {

InterpolationValue SVGNumberInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return InterpolationValue(InterpolableNumber::Create(0));
}

InterpolationValue SVGNumberInterpolationType::MaybeConvertSVGValue(
    const SVGPropertyBase& svg_value) const {
  if (svg_value.GetType() != kAnimatedNumber)
    return nullptr;
  return InterpolationValue(
      InterpolableNumber::Create(ToSVGNumber(svg_value).Value()));
}

SVGPropertyBase* SVGNumberInterpolationType::AppliedSVGValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*) const {
  double value = ToInterpolableNumber(interpolable_value).Value();
  return SVGNumber::Create(is_non_negative_ && value < 0 ? 0 : value);
}

}  // namespace blink
