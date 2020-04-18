// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_position_axis_list_interpolation_type.h"

#include "third_party/blink/renderer/core/animation/length_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"

namespace blink {

InterpolationValue
CSSPositionAxisListInterpolationType::ConvertPositionAxisCSSValue(
    const CSSValue& value) {
  if (value.IsValuePair()) {
    const CSSValuePair& pair = ToCSSValuePair(value);
    InterpolationValue result =
        LengthInterpolationFunctions::MaybeConvertCSSValue(pair.Second());
    CSSValueID side = ToCSSIdentifierValue(pair.First()).GetValueID();
    if (side == CSSValueRight || side == CSSValueBottom)
      LengthInterpolationFunctions::SubtractFromOneHundredPercent(result);
    return result;
  }

  if (value.IsPrimitiveValue())
    return LengthInterpolationFunctions::MaybeConvertCSSValue(value);

  if (!value.IsIdentifierValue())
    return nullptr;

  const CSSIdentifierValue& ident = ToCSSIdentifierValue(value);
  switch (ident.GetValueID()) {
    case CSSValueLeft:
    case CSSValueTop:
      return LengthInterpolationFunctions::CreateInterpolablePercent(0);
    case CSSValueRight:
    case CSSValueBottom:
      return LengthInterpolationFunctions::CreateInterpolablePercent(100);
    case CSSValueCenter:
      return LengthInterpolationFunctions::CreateInterpolablePercent(50);
    default:
      NOTREACHED();
      return nullptr;
  }
}

InterpolationValue CSSPositionAxisListInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  if (!value.IsBaseValueList()) {
    return ListInterpolationFunctions::CreateList(
        1, [&value](size_t) { return ConvertPositionAxisCSSValue(value); });
  }

  const CSSValueList& list = ToCSSValueList(value);
  return ListInterpolationFunctions::CreateList(
      list.length(), [&list](size_t index) {
        return ConvertPositionAxisCSSValue(list.Item(index));
      });
}

}  // namespace blink
