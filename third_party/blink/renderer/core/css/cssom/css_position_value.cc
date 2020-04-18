// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/cssom/css_position_value.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/cssom/css_math_sum.h"
#include "third_party/blink/renderer/core/css/cssom/css_numeric_value.h"
#include "third_party/blink/renderer/core/css/cssom/css_unit_value.h"

namespace blink {

namespace {

bool IsValidPositionCoord(CSSNumericValue* v) {
  return v && v->Type().MatchesBaseTypePercentage(
                  CSSNumericValueType::BaseType::kLength);
}

CSSNumericValue* FromSingleValue(const CSSValue& value) {
  if (value.IsIdentifierValue()) {
    const auto& ident = ToCSSIdentifierValue(value);
    switch (ident.GetValueID()) {
      case CSSValueLeft:
      case CSSValueTop:
        return CSSUnitValue::Create(0,
                                    CSSPrimitiveValue::UnitType::kPercentage);
      case CSSValueCenter:
        return CSSUnitValue::Create(50,
                                    CSSPrimitiveValue::UnitType::kPercentage);
      case CSSValueRight:
      case CSSValueBottom:
        return CSSUnitValue::Create(100,
                                    CSSPrimitiveValue::UnitType::kPercentage);
      default:
        NOTREACHED();
        return nullptr;
    }
  }

  if (value.IsPrimitiveValue())
    return CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(value));

  DCHECK(value.IsValuePair());
  const auto& pair = ToCSSValuePair(value);
  DCHECK(pair.First().IsIdentifierValue());
  DCHECK(pair.Second().IsPrimitiveValue());

  CSSNumericValue* offset =
      CSSNumericValue::FromCSSValue(ToCSSPrimitiveValue(pair.Second()));
  DCHECK(offset);

  switch (ToCSSIdentifierValue(pair.First()).GetValueID()) {
    case CSSValueLeft:
    case CSSValueTop:
      return offset;
    case CSSValueRight:
    case CSSValueBottom: {
      CSSNumericValueVector args;
      args.push_back(
          CSSUnitValue::Create(100, CSSPrimitiveValue::UnitType::kPercentage));
      args.push_back(offset->Negate());
      return CSSMathSum::Create(std::move(args));
    }
    default:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace

CSSPositionValue* CSSPositionValue::Create(CSSNumericValue* x,
                                           CSSNumericValue* y,
                                           ExceptionState& exception_state) {
  if (!IsValidPositionCoord(x)) {
    exception_state.ThrowTypeError(
        "Must pass length or percentage to x in CSSPositionValue");
    return nullptr;
  }
  if (!IsValidPositionCoord(y)) {
    exception_state.ThrowTypeError(
        "Must pass length or percentage to y in CSSPositionValue");
    return nullptr;
  }
  return new CSSPositionValue(x, y);
}

CSSPositionValue* CSSPositionValue::Create(CSSNumericValue* x,
                                           CSSNumericValue* y) {
  if (!IsValidPositionCoord(x) || !IsValidPositionCoord(y))
    return nullptr;
  return new CSSPositionValue(x, y);
}

CSSPositionValue* CSSPositionValue::FromCSSValue(const CSSValue& value) {
  DCHECK(value.IsValuePair());
  const auto& pair = ToCSSValuePair(value);

  CSSNumericValue* x = FromSingleValue(pair.First());
  CSSNumericValue* y = FromSingleValue(pair.Second());
  DCHECK(x);
  DCHECK(y);

  return CSSPositionValue::Create(x, y);
}

void CSSPositionValue::setX(CSSNumericValue* x,
                            ExceptionState& exception_state) {
  if (!IsValidPositionCoord(x)) {
    exception_state.ThrowTypeError(
        "Must pass length or percentage to x in CSSPositionValue");
    return;
  }
  x_ = x;
}

void CSSPositionValue::setY(CSSNumericValue* y,
                            ExceptionState& exception_state) {
  if (!IsValidPositionCoord(y)) {
    exception_state.ThrowTypeError(
        "Must pass length or percentage to y in CSSPositionValue");
    return;
  }
  y_ = y;
}

const CSSValue* CSSPositionValue::ToCSSValue() const {
  const CSSValue* x = x_->ToCSSValue();
  const CSSValue* y = y_->ToCSSValue();
  if (!x || !y)
    return nullptr;
  return CSSValuePair::Create(x, y, CSSValuePair::kKeepIdenticalValues);
}

}  // namespace blink
