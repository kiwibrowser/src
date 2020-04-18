// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_perspective.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_calculation_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_unit_value.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix.h"

namespace blink {

namespace {

bool IsValidPerspectiveLength(CSSNumericValue* value) {
  return value &&
         value->Type().MatchesBaseType(CSSNumericValueType::BaseType::kLength);
}

}  // namespace

CSSPerspective* CSSPerspective::Create(CSSNumericValue* length,
                                       ExceptionState& exception_state) {
  if (!IsValidPerspectiveLength(length)) {
    exception_state.ThrowTypeError("Must pass length to CSSPerspective");
    return nullptr;
  }
  return new CSSPerspective(length);
}

void CSSPerspective::setLength(CSSNumericValue* length,
                               ExceptionState& exception_state) {
  if (!IsValidPerspectiveLength(length)) {
    exception_state.ThrowTypeError("Must pass length to CSSPerspective");
    return;
  }
  length_ = length;
}

CSSPerspective* CSSPerspective::FromCSSValue(const CSSFunctionValue& value) {
  DCHECK_EQ(value.FunctionType(), CSSValuePerspective);
  DCHECK_EQ(value.length(), 1U);
  CSSNumericValue* length =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value.Item(0)));
  return new CSSPerspective(length);
}

DOMMatrix* CSSPerspective::toMatrix(ExceptionState& exception_state) const {
  if (length_->IsUnitValue() && ToCSSUnitValue(length_)->value() < 0) {
    // Negative values are invalid.
    // https://github.com/w3c/css-houdini-drafts/issues/420
    return nullptr;
  }
  CSSUnitValue* length = length_->to(CSSPrimitiveValue::UnitType::kPixels);
  if (!length) {
    exception_state.ThrowTypeError(
        "Cannot create matrix if units are not compatible with px");
    return nullptr;
  }
  DOMMatrix* matrix = DOMMatrix::Create();
  matrix->perspectiveSelf(length->value());
  return matrix;
}

const CSSFunctionValue* CSSPerspective::ToCSSValue() const {
  const CSSValue* length = nullptr;
  if (length_->IsUnitValue() && ToCSSUnitValue(length_)->value() < 0) {
    // Wrap out of range length with a calc.
    CSSCalcExpressionNode* node = length_->ToCalcExpressionNode();
    node->SetIsNestedCalc();
    length = CSSPrimitiveValue::Create(CSSCalcValue::Create(node));
  } else {
    length = length_->ToCSSValue();
  }

  DCHECK(length);
  CSSFunctionValue* result = CSSFunctionValue::Create(CSSValuePerspective);
  result->Append(*length);
  return result;
}

CSSPerspective::CSSPerspective(CSSNumericValue* length)
    : CSSTransformComponent(false /* is2D */), length_(length) {
  DCHECK(length);
}

}  // namespace blink
