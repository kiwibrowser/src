// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_resolution_interpolation_type.h"

#include "third_party/blink/renderer/core/css/css_primitive_value.h"

namespace blink {

InterpolationValue CSSResolutionInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return InterpolationValue(InterpolableNumber::Create(0));
}

InterpolationValue CSSResolutionInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (!value.IsPrimitiveValue() ||
      !CSSPrimitiveValue::IsResolution(
          ToCSSPrimitiveValue(value).TypeWithCalcResolved()))
    return nullptr;
  return InterpolationValue(InterpolableNumber::Create(
      ToCSSPrimitiveValue(value).ComputeDotsPerPixel()));
}

const CSSValue* CSSResolutionInterpolationType::CreateCSSValue(
    const InterpolableValue& value,
    const NonInterpolableValue*,
    const StyleResolverState&) const {
  return CSSPrimitiveValue::Create(ToInterpolableNumber(value).Value(),
                                   CSSPrimitiveValue::UnitType::kDotsPerPixel);
}

}  // namespace blink
