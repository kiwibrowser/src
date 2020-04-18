// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_transform_component.h"

#include "third_party/blink/renderer/core/css/cssom/css_matrix_component.h"
#include "third_party/blink/renderer/core/css/cssom/css_perspective.h"
#include "third_party/blink/renderer/core/css/cssom/css_rotate.h"
#include "third_party/blink/renderer/core/css/cssom/css_scale.h"
#include "third_party/blink/renderer/core/css/cssom/css_skew.h"
#include "third_party/blink/renderer/core/css/cssom/css_skew_x.h"
#include "third_party/blink/renderer/core/css/cssom/css_skew_y.h"
#include "third_party/blink/renderer/core/css/cssom/css_translate.h"

namespace blink {

CSSTransformComponent* CSSTransformComponent::FromCSSValue(
    const CSSValue& value) {
  if (!value.IsFunctionValue())
    return nullptr;

  const CSSFunctionValue& function_value = ToCSSFunctionValue(value);
  switch (function_value.FunctionType()) {
    case CSSValueMatrix:
    case CSSValueMatrix3d:
      return CSSMatrixComponent::FromCSSValue(function_value);
    case CSSValuePerspective:
      return CSSPerspective::FromCSSValue(function_value);
    case CSSValueRotate:
    case CSSValueRotateX:
    case CSSValueRotateY:
    case CSSValueRotateZ:
    case CSSValueRotate3d:
      return CSSRotate::FromCSSValue(function_value);
    case CSSValueScale:
    case CSSValueScaleX:
    case CSSValueScaleY:
    case CSSValueScaleZ:
    case CSSValueScale3d:
      return CSSScale::FromCSSValue(function_value);
    case CSSValueSkew:
      return CSSSkew::FromCSSValue(function_value);
    case CSSValueSkewX:
      return CSSSkewX::FromCSSValue(function_value);
    case CSSValueSkewY:
      return CSSSkewY::FromCSSValue(function_value);
    case CSSValueTranslate:
    case CSSValueTranslateX:
    case CSSValueTranslateY:
    case CSSValueTranslateZ:
    case CSSValueTranslate3d:
      return CSSTranslate::FromCSSValue(function_value);
    default:
      return nullptr;
  }
}

String CSSTransformComponent::toString() const {
  const CSSValue* result = ToCSSValue();
  return result ? result->CssText() : "";
}

}  // namespace blink
