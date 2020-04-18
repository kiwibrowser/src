// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_default_interpolation_type.h"

#include "third_party/blink/renderer/core/animation/css_interpolation_environment.h"
#include "third_party/blink/renderer/core/animation/string_keyframe.h"
#include "third_party/blink/renderer/core/css/resolver/style_builder.h"

namespace blink {

CSSDefaultNonInterpolableValue::CSSDefaultNonInterpolableValue(
    const CSSValue* css_value)
    : css_value_(css_value) {
  DCHECK(css_value_);
}

DEFINE_NON_INTERPOLABLE_VALUE_TYPE(CSSDefaultNonInterpolableValue);

InterpolationValue CSSDefaultInterpolationType::MaybeConvertSingle(
    const PropertySpecificKeyframe& keyframe,
    const InterpolationEnvironment&,
    const InterpolationValue&,
    ConversionCheckers&) const {
  if (!ToCSSPropertySpecificKeyframe(keyframe).Value()) {
    DCHECK(keyframe.IsNeutral());
    return nullptr;
  }
  return InterpolationValue(
      InterpolableList::Create(0),
      CSSDefaultNonInterpolableValue::Create(
          ToCSSPropertySpecificKeyframe(keyframe).Value()));
}

void CSSDefaultInterpolationType::Apply(
    const InterpolableValue&,
    const NonInterpolableValue* non_interpolable_value,
    InterpolationEnvironment& environment) const {
  DCHECK(ToCSSDefaultNonInterpolableValue(non_interpolable_value)->CssValue());
  StyleBuilder::ApplyProperty(
      GetProperty().GetCSSProperty(),
      ToCSSInterpolationEnvironment(environment).GetState(),
      *ToCSSDefaultNonInterpolableValue(non_interpolable_value)->CssValue());
}

}  // namespace blink
