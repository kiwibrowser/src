// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/svg_length_interpolation_type.h"

#include <memory>
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/animation/svg_interpolation_environment.h"
#include "third_party/blink/renderer/core/css/css_resolution_units.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg/svg_length.h"
#include "third_party/blink/renderer/core/svg/svg_length_context.h"

namespace blink {

std::unique_ptr<InterpolableValue>
SVGLengthInterpolationType::NeutralInterpolableValue() {
  std::unique_ptr<InterpolableList> list_of_values =
      InterpolableList::Create(CSSPrimitiveValue::kLengthUnitTypeCount);
  for (size_t i = 0; i < CSSPrimitiveValue::kLengthUnitTypeCount; ++i)
    list_of_values->Set(i, InterpolableNumber::Create(0));

  return std::move(list_of_values);
}

InterpolationValue SVGLengthInterpolationType::ConvertSVGLength(
    const SVGLength& length) {
  const CSSPrimitiveValue& primitive_value = length.AsCSSPrimitiveValue();

  CSSLengthArray length_array;
  primitive_value.AccumulateLengthArray(length_array);

  std::unique_ptr<InterpolableList> list_of_values =
      InterpolableList::Create(CSSPrimitiveValue::kLengthUnitTypeCount);
  for (size_t i = 0; i < CSSPrimitiveValue::kLengthUnitTypeCount; ++i)
    list_of_values->Set(i, InterpolableNumber::Create(length_array.values[i]));

  return InterpolationValue(std::move(list_of_values));
}

SVGLength* SVGLengthInterpolationType::ResolveInterpolableSVGLength(
    const InterpolableValue& interpolable_value,
    const SVGLengthContext& length_context,
    SVGLengthMode unit_mode,
    bool negative_values_forbidden) {
  const InterpolableList& list_of_values =
      ToInterpolableList(interpolable_value);

  double value = 0;
  CSSPrimitiveValue::UnitType unit_type =
      CSSPrimitiveValue::UnitType::kUserUnits;
  unsigned unit_type_count = 0;
  // We optimise for the common case where only one unit type is involved.
  for (size_t i = 0; i < CSSPrimitiveValue::kLengthUnitTypeCount; i++) {
    double entry = ToInterpolableNumber(list_of_values.Get(i))->Value();
    if (!entry)
      continue;
    unit_type_count++;
    if (unit_type_count > 1)
      break;

    value = entry;
    unit_type = CSSPrimitiveValue::LengthUnitTypeToUnitType(
        static_cast<CSSPrimitiveValue::LengthUnitType>(i));
  }

  if (unit_type_count > 1) {
    value = 0;
    unit_type = CSSPrimitiveValue::UnitType::kUserUnits;

    // SVGLength does not support calc expressions, so we convert to canonical
    // units.
    for (size_t i = 0; i < CSSPrimitiveValue::kLengthUnitTypeCount; i++) {
      double entry = ToInterpolableNumber(list_of_values.Get(i))->Value();
      if (entry)
        value += length_context.ConvertValueToUserUnits(
            entry, unit_mode,
            CSSPrimitiveValue::LengthUnitTypeToUnitType(
                static_cast<CSSPrimitiveValue::LengthUnitType>(i)));
    }
  }

  if (negative_values_forbidden && value < 0)
    value = 0;

  SVGLength* result = SVGLength::Create(unit_mode);  // defaults to the length 0
  result->NewValueSpecifiedUnits(unit_type, value);
  return result;
}

InterpolationValue SVGLengthInterpolationType::MaybeConvertNeutral(
    const InterpolationValue&,
    ConversionCheckers&) const {
  return InterpolationValue(NeutralInterpolableValue());
}

InterpolationValue SVGLengthInterpolationType::MaybeConvertSVGValue(
    const SVGPropertyBase& svg_value) const {
  if (svg_value.GetType() != kAnimatedLength)
    return nullptr;

  return ConvertSVGLength(ToSVGLength(svg_value));
}

SVGPropertyBase* SVGLengthInterpolationType::AppliedSVGValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue*) const {
  NOTREACHED();
  // This function is no longer called, because apply has been overridden.
  return nullptr;
}

void SVGLengthInterpolationType::Apply(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    InterpolationEnvironment& environment) const {
  SVGElement& element = ToSVGInterpolationEnvironment(environment).SvgElement();
  SVGLengthContext length_context(&element);
  element.SetWebAnimatedAttribute(
      Attribute(),
      ResolveInterpolableSVGLength(interpolable_value, length_context,
                                   unit_mode_, negative_values_forbidden_));
}

}  // namespace blink
