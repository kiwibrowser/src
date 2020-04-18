// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_skew.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_numeric_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_style_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_unit_value.h"
#include "third_party/blink/renderer/core/geometry/dom_matrix.h"

namespace blink {

namespace {

bool IsValidSkewAngle(CSSNumericValue* value) {
  return value &&
         value->Type().MatchesBaseType(CSSNumericValueType::BaseType::kAngle);
}

}  // namespace

CSSSkew* CSSSkew::Create(CSSNumericValue* ax,
                         CSSNumericValue* ay,
                         ExceptionState& exception_state) {
  if (!IsValidSkewAngle(ax) || !IsValidSkewAngle(ay)) {
    exception_state.ThrowTypeError("CSSSkew does not support non-angles");
    return nullptr;
  }
  return new CSSSkew(ax, ay);
}

void CSSSkew::setAx(CSSNumericValue* value, ExceptionState& exception_state) {
  if (!IsValidSkewAngle(value)) {
    exception_state.ThrowTypeError("Must specify an angle unit");
    return;
  }
  ax_ = value;
}

void CSSSkew::setAy(CSSNumericValue* value, ExceptionState& exception_state) {
  if (!IsValidSkewAngle(value)) {
    exception_state.ThrowTypeError("Must specify an angle unit");
    return;
  }
  ay_ = value;
}

CSSSkew* CSSSkew::FromCSSValue(const CSSFunctionValue& value) {
  DCHECK_GT(value.length(), 0U);
  const CSSPrimitiveValue& x_value = ToCSSPrimitiveValue(value.Item(0));
  DCHECK_EQ(value.FunctionType(), CSSValueSkew);
  if (value.length() == 1U) {
    return CSSSkew::Create(
        CSSNumericValue::FromCSSValue(x_value),
        CSSUnitValue::Create(0, CSSPrimitiveValue::UnitType::kDegrees));
  } else if (value.length() == 2U) {
    const CSSPrimitiveValue& y_value = ToCSSPrimitiveValue(value.Item(1));
    return CSSSkew::Create(CSSNumericValue::FromCSSValue(x_value),
                           CSSNumericValue::FromCSSValue(y_value));
  }
  NOTREACHED();
  return nullptr;
}

DOMMatrix* CSSSkew::toMatrix(ExceptionState&) const {
  CSSUnitValue* ax = ax_->to(CSSPrimitiveValue::UnitType::kRadians);
  CSSUnitValue* ay = ay_->to(CSSPrimitiveValue::UnitType::kRadians);
  DCHECK(ax);
  DCHECK(ay);
  DOMMatrix* result = DOMMatrix::Create();
  result->setM12(std::tan(ay->value()));
  result->setM21(std::tan(ax->value()));
  return result;
}

const CSSFunctionValue* CSSSkew::ToCSSValue() const {
  const CSSValue* ax = ax_->ToCSSValue();
  const CSSValue* ay = ay_->ToCSSValue();
  if (!ax || !ay)
    return nullptr;

  CSSFunctionValue* result = CSSFunctionValue::Create(CSSValueSkew);
  result->Append(*ax);
  if (!ay_->IsUnitValue() || ToCSSUnitValue(ay_)->value() != 0)
    result->Append(*ay);
  return result;
}

CSSSkew::CSSSkew(CSSNumericValue* ax, CSSNumericValue* ay)
    : CSSTransformComponent(true /* is2D */), ax_(ax), ay_(ay) {
  DCHECK(ax);
  DCHECK(ay);
}

}  // namespace blink
