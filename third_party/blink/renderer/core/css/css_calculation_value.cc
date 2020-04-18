/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/css_calculation_value.h"

#include "third_party/blink/renderer/core/css/css_primitive_value_mappings.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

static const int maxExpressionDepth = 100;

enum ParseState { OK, TooDeep, NoMoreTokens };

namespace blink {

static CalculationCategory UnitCategory(CSSPrimitiveValue::UnitType type) {
  switch (type) {
    case CSSPrimitiveValue::UnitType::kNumber:
    case CSSPrimitiveValue::UnitType::kInteger:
      return kCalcNumber;
    case CSSPrimitiveValue::UnitType::kPercentage:
      return kCalcPercent;
    case CSSPrimitiveValue::UnitType::kEms:
    case CSSPrimitiveValue::UnitType::kExs:
    case CSSPrimitiveValue::UnitType::kPixels:
    case CSSPrimitiveValue::UnitType::kCentimeters:
    case CSSPrimitiveValue::UnitType::kMillimeters:
    case CSSPrimitiveValue::UnitType::kQuarterMillimeters:
    case CSSPrimitiveValue::UnitType::kInches:
    case CSSPrimitiveValue::UnitType::kPoints:
    case CSSPrimitiveValue::UnitType::kPicas:
    case CSSPrimitiveValue::UnitType::kUserUnits:
    case CSSPrimitiveValue::UnitType::kRems:
    case CSSPrimitiveValue::UnitType::kChs:
    case CSSPrimitiveValue::UnitType::kViewportWidth:
    case CSSPrimitiveValue::UnitType::kViewportHeight:
    case CSSPrimitiveValue::UnitType::kViewportMin:
    case CSSPrimitiveValue::UnitType::kViewportMax:
      return kCalcLength;
    case CSSPrimitiveValue::UnitType::kDegrees:
    case CSSPrimitiveValue::UnitType::kGradians:
    case CSSPrimitiveValue::UnitType::kRadians:
    case CSSPrimitiveValue::UnitType::kTurns:
      return kCalcAngle;
    case CSSPrimitiveValue::UnitType::kMilliseconds:
    case CSSPrimitiveValue::UnitType::kSeconds:
      return kCalcTime;
    case CSSPrimitiveValue::UnitType::kHertz:
    case CSSPrimitiveValue::UnitType::kKilohertz:
      return kCalcFrequency;
    default:
      return kCalcOther;
  }
}

static bool HasDoubleValue(CSSPrimitiveValue::UnitType type) {
  switch (type) {
    case CSSPrimitiveValue::UnitType::kNumber:
    case CSSPrimitiveValue::UnitType::kPercentage:
    case CSSPrimitiveValue::UnitType::kEms:
    case CSSPrimitiveValue::UnitType::kExs:
    case CSSPrimitiveValue::UnitType::kChs:
    case CSSPrimitiveValue::UnitType::kRems:
    case CSSPrimitiveValue::UnitType::kPixels:
    case CSSPrimitiveValue::UnitType::kCentimeters:
    case CSSPrimitiveValue::UnitType::kMillimeters:
    case CSSPrimitiveValue::UnitType::kQuarterMillimeters:
    case CSSPrimitiveValue::UnitType::kInches:
    case CSSPrimitiveValue::UnitType::kPoints:
    case CSSPrimitiveValue::UnitType::kPicas:
    case CSSPrimitiveValue::UnitType::kUserUnits:
    case CSSPrimitiveValue::UnitType::kDegrees:
    case CSSPrimitiveValue::UnitType::kRadians:
    case CSSPrimitiveValue::UnitType::kGradians:
    case CSSPrimitiveValue::UnitType::kTurns:
    case CSSPrimitiveValue::UnitType::kMilliseconds:
    case CSSPrimitiveValue::UnitType::kSeconds:
    case CSSPrimitiveValue::UnitType::kHertz:
    case CSSPrimitiveValue::UnitType::kKilohertz:
    case CSSPrimitiveValue::UnitType::kViewportWidth:
    case CSSPrimitiveValue::UnitType::kViewportHeight:
    case CSSPrimitiveValue::UnitType::kViewportMin:
    case CSSPrimitiveValue::UnitType::kViewportMax:
    case CSSPrimitiveValue::UnitType::kDotsPerPixel:
    case CSSPrimitiveValue::UnitType::kDotsPerInch:
    case CSSPrimitiveValue::UnitType::kDotsPerCentimeter:
    case CSSPrimitiveValue::UnitType::kFraction:
    case CSSPrimitiveValue::UnitType::kInteger:
      return true;
    case CSSPrimitiveValue::UnitType::kUnknown:
    case CSSPrimitiveValue::UnitType::kCalc:
    case CSSPrimitiveValue::UnitType::kCalcPercentageWithNumber:
    case CSSPrimitiveValue::UnitType::kCalcPercentageWithLength:
    case CSSPrimitiveValue::UnitType::kCalcLengthWithNumber:
    case CSSPrimitiveValue::UnitType::kCalcPercentageWithLengthAndNumber:
    case CSSPrimitiveValue::UnitType::kQuirkyEms:
      return false;
  };
  NOTREACHED();
  return false;
}

static String BuildCSSText(const String& expression) {
  StringBuilder result;
  result.Append("calc");
  bool expression_has_single_term = expression[0] != '(';
  if (expression_has_single_term)
    result.Append('(');
  result.Append(expression);
  if (expression_has_single_term)
    result.Append(')');
  return result.ToString();
}

String CSSCalcValue::CustomCSSText() const {
  return BuildCSSText(expression_->CustomCSSText());
}

bool CSSCalcValue::Equals(const CSSCalcValue& other) const {
  return DataEquivalent(expression_, other.expression_);
}

double CSSCalcValue::ClampToPermittedRange(double value) const {
  return non_negative_ && value < 0 ? 0 : value;
}

double CSSCalcValue::DoubleValue() const {
  return ClampToPermittedRange(expression_->DoubleValue());
}

double CSSCalcValue::ComputeLengthPx(
    const CSSToLengthConversionData& conversion_data) const {
  return ClampToPermittedRange(expression_->ComputeLengthPx(conversion_data));
}

class CSSCalcPrimitiveValue final : public CSSCalcExpressionNode {
 public:
  static CSSCalcPrimitiveValue* Create(CSSPrimitiveValue* value,
                                       bool is_integer) {
    return new CSSCalcPrimitiveValue(value, is_integer);
  }

  static CSSCalcPrimitiveValue* Create(double value,
                                       CSSPrimitiveValue::UnitType type,
                                       bool is_integer) {
    if (std::isnan(value) || std::isinf(value))
      return nullptr;
    return new CSSCalcPrimitiveValue(CSSPrimitiveValue::Create(value, type),
                                     is_integer);
  }

  bool IsZero() const override { return !value_->GetDoubleValue(); }

  String CustomCSSText() const override { return value_->CssText(); }

  void AccumulatePixelsAndPercent(
      const CSSToLengthConversionData& conversion_data,
      PixelsAndPercent& value,
      float multiplier) const override {
    switch (category_) {
      case kCalcLength:
        value.pixels = clampTo<float>(
            value.pixels +
            value_->ComputeLength<double>(conversion_data) * multiplier);
        break;
      case kCalcPercent:
        DCHECK(value_->IsPercentage());
        value.percent = clampTo<float>(value.percent +
                                       value_->GetDoubleValue() * multiplier);
        break;
      case kCalcNumber:
        // TODO(alancutter): Stop treating numbers like pixels unconditionally
        // in calcs to be able to accomodate border-image-width
        // https://drafts.csswg.org/css-backgrounds-3/#the-border-image-width
        value.pixels = clampTo<float>(value.pixels +
                                      value_->GetDoubleValue() *
                                          conversion_data.Zoom() * multiplier);
        break;
      default:
        NOTREACHED();
    }
  }

  double DoubleValue() const override {
    if (HasDoubleValue(TypeWithCalcResolved()))
      return value_->GetDoubleValue();
    NOTREACHED();
    return 0;
  }

  double ComputeLengthPx(
      const CSSToLengthConversionData& conversion_data) const override {
    switch (category_) {
      case kCalcLength:
        return value_->ComputeLength<double>(conversion_data);
      case kCalcNumber:
      case kCalcPercent:
        return value_->GetDoubleValue();
      case kCalcAngle:
      case kCalcFrequency:
      case kCalcPercentLength:
      case kCalcPercentNumber:
      case kCalcTime:
      case kCalcLengthNumber:
      case kCalcPercentLengthNumber:
      case kCalcOther:
        NOTREACHED();
        break;
    }
    NOTREACHED();
    return 0;
  }

  void AccumulateLengthArray(CSSLengthArray& length_array,
                             double multiplier) const override {
    DCHECK_NE(Category(), kCalcNumber);
    value_->AccumulateLengthArray(length_array, multiplier);
  }

  bool operator==(const CSSCalcExpressionNode& other) const override {
    if (GetType() != other.GetType())
      return false;

    return DataEquivalent(
        value_, static_cast<const CSSCalcPrimitiveValue&>(other).value_);
  }

  Type GetType() const override { return kCssCalcPrimitiveValue; }
  CSSPrimitiveValue::UnitType TypeWithCalcResolved() const override {
    return value_->TypeWithCalcResolved();
  }
  const CSSCalcExpressionNode* LeftExpressionNode() const override {
    NOTREACHED();
    return nullptr;
  }

  const CSSCalcExpressionNode* RightExpressionNode() const override {
    NOTREACHED();
    return nullptr;
  }

  CalcOperator OperatorType() const override {
    NOTREACHED();
    return kCalcAdd;
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(value_);
    CSSCalcExpressionNode::Trace(visitor);
  }

 private:
  CSSCalcPrimitiveValue(CSSPrimitiveValue* value, bool is_integer)
      : CSSCalcExpressionNode(UnitCategory(value->TypeWithCalcResolved()),
                              is_integer),
        value_(value) {}

  Member<CSSPrimitiveValue> value_;
};

static const CalculationCategory kAddSubtractResult[kCalcOther][kCalcOther] = {
    /* CalcNumber */ {kCalcNumber, kCalcLengthNumber, kCalcPercentNumber,
                      kCalcPercentNumber, kCalcOther, kCalcOther, kCalcOther,
                      kCalcOther, kCalcLengthNumber, kCalcPercentLengthNumber},
    /* CalcLength */
    {kCalcLengthNumber, kCalcLength, kCalcPercentLength, kCalcOther,
     kCalcPercentLength, kCalcOther, kCalcOther, kCalcOther, kCalcLengthNumber,
     kCalcPercentLengthNumber},
    /* CalcPercent */
    {kCalcPercentNumber, kCalcPercentLength, kCalcPercent, kCalcPercentNumber,
     kCalcPercentLength, kCalcOther, kCalcOther, kCalcOther,
     kCalcPercentLengthNumber, kCalcPercentLengthNumber},
    /* CalcPercentNumber */
    {kCalcPercentNumber, kCalcPercentLengthNumber, kCalcPercentNumber,
     kCalcPercentNumber, kCalcPercentLengthNumber, kCalcOther, kCalcOther,
     kCalcOther, kCalcOther, kCalcPercentLengthNumber},
    /* CalcPercentLength */
    {kCalcPercentLengthNumber, kCalcPercentLength, kCalcPercentLength,
     kCalcPercentLengthNumber, kCalcPercentLength, kCalcOther, kCalcOther,
     kCalcOther, kCalcOther, kCalcPercentLengthNumber},
    /* CalcAngle  */
    {kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcAngle,
     kCalcOther, kCalcOther, kCalcOther, kCalcOther},
    /* CalcTime */
    {kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcOther,
     kCalcTime, kCalcOther, kCalcOther, kCalcOther},
    /* CalcFrequency */
    {kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcOther, kCalcOther,
     kCalcOther, kCalcFrequency, kCalcOther, kCalcOther},
    /* CalcLengthNumber */
    {kCalcLengthNumber, kCalcLengthNumber, kCalcPercentLengthNumber,
     kCalcPercentLengthNumber, kCalcPercentLengthNumber, kCalcOther, kCalcOther,
     kCalcOther, kCalcLengthNumber, kCalcPercentLengthNumber},
    /* CalcPercentLengthNumber */
    {kCalcPercentLengthNumber, kCalcPercentLengthNumber,
     kCalcPercentLengthNumber, kCalcPercentLengthNumber,
     kCalcPercentLengthNumber, kCalcOther, kCalcOther, kCalcOther,
     kCalcPercentLengthNumber, kCalcPercentLengthNumber}};

static CalculationCategory DetermineCategory(
    const CSSCalcExpressionNode& left_side,
    const CSSCalcExpressionNode& right_side,
    CalcOperator op) {
  CalculationCategory left_category = left_side.Category();
  CalculationCategory right_category = right_side.Category();

  if (left_category == kCalcOther || right_category == kCalcOther)
    return kCalcOther;

  switch (op) {
    case kCalcAdd:
    case kCalcSubtract:
      return kAddSubtractResult[left_category][right_category];
    case kCalcMultiply:
      if (left_category != kCalcNumber && right_category != kCalcNumber)
        return kCalcOther;
      return left_category == kCalcNumber ? right_category : left_category;
    case kCalcDivide:
      if (right_category != kCalcNumber || right_side.IsZero())
        return kCalcOther;
      return left_category;
  }

  NOTREACHED();
  return kCalcOther;
}

static bool IsIntegerResult(const CSSCalcExpressionNode* left_side,
                            const CSSCalcExpressionNode* right_side,
                            CalcOperator op) {
  // Not testing for actual integer values.
  // Performs W3C spec's type checking for calc integers.
  // http://www.w3.org/TR/css3-values/#calc-type-checking
  return op != kCalcDivide && left_side->IsInteger() && right_side->IsInteger();
}

class CSSCalcBinaryOperation final : public CSSCalcExpressionNode {
 public:
  static CSSCalcExpressionNode* Create(CSSCalcExpressionNode* left_side,
                                       CSSCalcExpressionNode* right_side,
                                       CalcOperator op) {
    DCHECK_NE(left_side->Category(), kCalcOther);
    DCHECK_NE(right_side->Category(), kCalcOther);

    CalculationCategory new_category =
        DetermineCategory(*left_side, *right_side, op);
    if (new_category == kCalcOther)
      return nullptr;

    return new CSSCalcBinaryOperation(left_side, right_side, op, new_category);
  }

  static CSSCalcExpressionNode* CreateSimplified(
      CSSCalcExpressionNode* left_side,
      CSSCalcExpressionNode* right_side,
      CalcOperator op) {
    CalculationCategory left_category = left_side->Category();
    CalculationCategory right_category = right_side->Category();
    DCHECK_NE(left_category, kCalcOther);
    DCHECK_NE(right_category, kCalcOther);

    bool is_integer = IsIntegerResult(left_side, right_side, op);

    // Simplify numbers.
    if (left_category == kCalcNumber && right_category == kCalcNumber) {
      return CSSCalcPrimitiveValue::Create(
          EvaluateOperator(left_side->DoubleValue(), right_side->DoubleValue(),
                           op),
          CSSPrimitiveValue::UnitType::kNumber, is_integer);
    }

    // Simplify addition and subtraction between same types.
    if (op == kCalcAdd || op == kCalcSubtract) {
      if (left_category == right_side->Category()) {
        CSSPrimitiveValue::UnitType left_type =
            left_side->TypeWithCalcResolved();
        if (HasDoubleValue(left_type)) {
          CSSPrimitiveValue::UnitType right_type =
              right_side->TypeWithCalcResolved();
          if (left_type == right_type)
            return CSSCalcPrimitiveValue::Create(
                EvaluateOperator(left_side->DoubleValue(),
                                 right_side->DoubleValue(), op),
                left_type, is_integer);
          CSSPrimitiveValue::UnitCategory left_unit_category =
              CSSPrimitiveValue::UnitTypeToUnitCategory(left_type);
          if (left_unit_category != CSSPrimitiveValue::kUOther &&
              left_unit_category ==
                  CSSPrimitiveValue::UnitTypeToUnitCategory(right_type)) {
            CSSPrimitiveValue::UnitType canonical_type =
                CSSPrimitiveValue::CanonicalUnitTypeForCategory(
                    left_unit_category);
            if (canonical_type != CSSPrimitiveValue::UnitType::kUnknown) {
              double left_value = clampTo<double>(
                  left_side->DoubleValue() *
                  CSSPrimitiveValue::ConversionToCanonicalUnitsScaleFactor(
                      left_type));
              double right_value = clampTo<double>(
                  right_side->DoubleValue() *
                  CSSPrimitiveValue::ConversionToCanonicalUnitsScaleFactor(
                      right_type));
              return CSSCalcPrimitiveValue::Create(
                  EvaluateOperator(left_value, right_value, op), canonical_type,
                  is_integer);
            }
          }
        }
      }
    } else {
      // Simplify multiplying or dividing by a number for simplifiable types.
      DCHECK(op == kCalcMultiply || op == kCalcDivide);
      CSSCalcExpressionNode* number_side = GetNumberSide(left_side, right_side);
      if (!number_side)
        return Create(left_side, right_side, op);
      if (number_side == left_side && op == kCalcDivide)
        return nullptr;
      CSSCalcExpressionNode* other_side =
          left_side == number_side ? right_side : left_side;

      double number = number_side->DoubleValue();
      if (std::isnan(number) || std::isinf(number))
        return nullptr;
      if (op == kCalcDivide && !number)
        return nullptr;

      CSSPrimitiveValue::UnitType other_type =
          other_side->TypeWithCalcResolved();
      if (HasDoubleValue(other_type))
        return CSSCalcPrimitiveValue::Create(
            EvaluateOperator(other_side->DoubleValue(), number, op), other_type,
            is_integer);
    }

    return Create(left_side, right_side, op);
  }

  bool IsZero() const override { return !DoubleValue(); }

  void AccumulatePixelsAndPercent(
      const CSSToLengthConversionData& conversion_data,
      PixelsAndPercent& value,
      float multiplier) const override {
    switch (operator_) {
      case kCalcAdd:
        left_side_->AccumulatePixelsAndPercent(conversion_data, value,
                                               multiplier);
        right_side_->AccumulatePixelsAndPercent(conversion_data, value,
                                                multiplier);
        break;
      case kCalcSubtract:
        left_side_->AccumulatePixelsAndPercent(conversion_data, value,
                                               multiplier);
        right_side_->AccumulatePixelsAndPercent(conversion_data, value,
                                                -multiplier);
        break;
      case kCalcMultiply:
        DCHECK_NE((left_side_->Category() == kCalcNumber),
                  (right_side_->Category() == kCalcNumber));
        if (left_side_->Category() == kCalcNumber)
          right_side_->AccumulatePixelsAndPercent(
              conversion_data, value, multiplier * left_side_->DoubleValue());
        else
          left_side_->AccumulatePixelsAndPercent(
              conversion_data, value, multiplier * right_side_->DoubleValue());
        break;
      case kCalcDivide:
        DCHECK_EQ(right_side_->Category(), kCalcNumber);
        left_side_->AccumulatePixelsAndPercent(
            conversion_data, value, multiplier / right_side_->DoubleValue());
        break;
      default:
        NOTREACHED();
    }
  }

  double DoubleValue() const override {
    return Evaluate(left_side_->DoubleValue(), right_side_->DoubleValue());
  }

  double ComputeLengthPx(
      const CSSToLengthConversionData& conversion_data) const override {
    const double left_value = left_side_->ComputeLengthPx(conversion_data);
    const double right_value = right_side_->ComputeLengthPx(conversion_data);
    return Evaluate(left_value, right_value);
  }

  void AccumulateLengthArray(CSSLengthArray& length_array,
                             double multiplier) const override {
    switch (operator_) {
      case kCalcAdd:
        left_side_->AccumulateLengthArray(length_array, multiplier);
        right_side_->AccumulateLengthArray(length_array, multiplier);
        break;
      case kCalcSubtract:
        left_side_->AccumulateLengthArray(length_array, multiplier);
        right_side_->AccumulateLengthArray(length_array, -multiplier);
        break;
      case kCalcMultiply:
        DCHECK_NE((left_side_->Category() == kCalcNumber),
                  (right_side_->Category() == kCalcNumber));
        if (left_side_->Category() == kCalcNumber)
          right_side_->AccumulateLengthArray(
              length_array, multiplier * left_side_->DoubleValue());
        else
          left_side_->AccumulateLengthArray(
              length_array, multiplier * right_side_->DoubleValue());
        break;
      case kCalcDivide:
        DCHECK_EQ(right_side_->Category(), kCalcNumber);
        left_side_->AccumulateLengthArray(
            length_array, multiplier / right_side_->DoubleValue());
        break;
      default:
        NOTREACHED();
    }
  }

  static String BuildCSSText(const String& left_expression,
                             const String& right_expression,
                             CalcOperator op) {
    StringBuilder result;
    result.Append('(');
    result.Append(left_expression);
    result.Append(' ');
    result.Append(static_cast<char>(op));
    result.Append(' ');
    result.Append(right_expression);
    result.Append(')');

    return result.ToString();
  }

  String CustomCSSText() const override {
    return BuildCSSText(left_side_->CustomCSSText(),
                        right_side_->CustomCSSText(), operator_);
  }

  bool operator==(const CSSCalcExpressionNode& exp) const override {
    if (GetType() != exp.GetType())
      return false;

    const CSSCalcBinaryOperation& other =
        static_cast<const CSSCalcBinaryOperation&>(exp);
    return DataEquivalent(left_side_, other.left_side_) &&
           DataEquivalent(right_side_, other.right_side_) &&
           operator_ == other.operator_;
  }

  Type GetType() const override { return kCssCalcBinaryOperation; }
  const CSSCalcExpressionNode* LeftExpressionNode() const override {
    return left_side_;
  }

  const CSSCalcExpressionNode* RightExpressionNode() const override {
    return right_side_;
  }

  CalcOperator OperatorType() const override { return operator_; }

  CSSPrimitiveValue::UnitType TypeWithCalcResolved() const override {
    switch (category_) {
      case kCalcNumber:
        DCHECK_EQ(left_side_->Category(), kCalcNumber);
        DCHECK_EQ(right_side_->Category(), kCalcNumber);
        return CSSPrimitiveValue::UnitType::kNumber;
      case kCalcLength:
      case kCalcPercent: {
        if (left_side_->Category() == kCalcNumber)
          return right_side_->TypeWithCalcResolved();
        if (right_side_->Category() == kCalcNumber)
          return left_side_->TypeWithCalcResolved();
        CSSPrimitiveValue::UnitType left_type =
            left_side_->TypeWithCalcResolved();
        if (left_type == right_side_->TypeWithCalcResolved())
          return left_type;
        return CSSPrimitiveValue::UnitType::kUnknown;
      }
      case kCalcAngle:
        return CSSPrimitiveValue::UnitType::kDegrees;
      case kCalcTime:
        return CSSPrimitiveValue::UnitType::kMilliseconds;
      case kCalcFrequency:
        return CSSPrimitiveValue::UnitType::kHertz;
      case kCalcPercentLength:
      case kCalcPercentNumber:
      case kCalcLengthNumber:
      case kCalcPercentLengthNumber:
      case kCalcOther:
        return CSSPrimitiveValue::UnitType::kUnknown;
    }
    NOTREACHED();
    return CSSPrimitiveValue::UnitType::kUnknown;
  }

  void Trace(blink::Visitor* visitor) override {
    visitor->Trace(left_side_);
    visitor->Trace(right_side_);
    CSSCalcExpressionNode::Trace(visitor);
  }

 private:
  CSSCalcBinaryOperation(CSSCalcExpressionNode* left_side,
                         CSSCalcExpressionNode* right_side,
                         CalcOperator op,
                         CalculationCategory category)
      : CSSCalcExpressionNode(category,
                              IsIntegerResult(left_side, right_side, op)),
        left_side_(left_side),
        right_side_(right_side),
        operator_(op) {}

  static CSSCalcExpressionNode* GetNumberSide(
      CSSCalcExpressionNode* left_side,
      CSSCalcExpressionNode* right_side) {
    if (left_side->Category() == kCalcNumber)
      return left_side;
    if (right_side->Category() == kCalcNumber)
      return right_side;
    return nullptr;
  }

  double Evaluate(double left_side, double right_side) const {
    return EvaluateOperator(left_side, right_side, operator_);
  }

  static double EvaluateOperator(double left_value,
                                 double right_value,
                                 CalcOperator op) {
    switch (op) {
      case kCalcAdd:
        return clampTo<double>(left_value + right_value);
      case kCalcSubtract:
        return clampTo<double>(left_value - right_value);
      case kCalcMultiply:
        return clampTo<double>(left_value * right_value);
      case kCalcDivide:
        if (right_value)
          return clampTo<double>(left_value / right_value);
        return std::numeric_limits<double>::quiet_NaN();
    }
    return 0;
  }

  const Member<CSSCalcExpressionNode> left_side_;
  const Member<CSSCalcExpressionNode> right_side_;
  const CalcOperator operator_;
};

static ParseState CheckDepthAndIndex(int* depth, CSSParserTokenRange tokens) {
  (*depth)++;
  if (tokens.AtEnd())
    return NoMoreTokens;
  if (*depth > maxExpressionDepth)
    return TooDeep;
  return OK;
}

class CSSCalcExpressionNodeParser {
  STACK_ALLOCATED();

 public:
  CSSCalcExpressionNodeParser() {}

  CSSCalcExpressionNode* ParseCalc(CSSParserTokenRange tokens) {
    Value result;
    tokens.ConsumeWhitespace();
    bool ok = ParseValueExpression(tokens, 0, &result);
    if (!ok || !tokens.AtEnd())
      return nullptr;
    return result.value;
  }

 private:
  struct Value {
    STACK_ALLOCATED();

   public:
    Member<CSSCalcExpressionNode> value;
  };

  char OperatorValue(const CSSParserToken& token) {
    if (token.GetType() == kDelimiterToken)
      return token.Delimiter();
    return 0;
  }

  bool ParseValue(CSSParserTokenRange& tokens, Value* result) {
    CSSParserToken token = tokens.ConsumeIncludingWhitespace();
    if (!(token.GetType() == kNumberToken ||
          token.GetType() == kPercentageToken ||
          token.GetType() == kDimensionToken))
      return false;

    CSSPrimitiveValue::UnitType type = token.GetUnitType();
    if (UnitCategory(type) == kCalcOther)
      return false;

    result->value = CSSCalcPrimitiveValue::Create(
        CSSPrimitiveValue::Create(token.NumericValue(), type),
        token.GetNumericValueType() == kIntegerValueType);

    return true;
  }

  bool ParseValueTerm(CSSParserTokenRange& tokens, int depth, Value* result) {
    if (CheckDepthAndIndex(&depth, tokens) != OK)
      return false;

    if (tokens.Peek().GetType() == kLeftParenthesisToken ||
        tokens.Peek().FunctionId() == CSSValueCalc) {
      CSSParserTokenRange inner_range = tokens.ConsumeBlock();
      tokens.ConsumeWhitespace();
      inner_range.ConsumeWhitespace();
      if (!ParseValueExpression(inner_range, depth, result))
        return false;
      result->value->SetIsNestedCalc();
      return true;
    }

    return ParseValue(tokens, result);
  }

  bool ParseValueMultiplicativeExpression(CSSParserTokenRange& tokens,
                                          int depth,
                                          Value* result) {
    if (CheckDepthAndIndex(&depth, tokens) != OK)
      return false;

    if (!ParseValueTerm(tokens, depth, result))
      return false;

    while (!tokens.AtEnd()) {
      char operator_character = OperatorValue(tokens.Peek());
      if (operator_character != kCalcMultiply &&
          operator_character != kCalcDivide)
        break;
      tokens.ConsumeIncludingWhitespace();

      Value rhs;
      if (!ParseValueTerm(tokens, depth, &rhs))
        return false;

      result->value = CSSCalcBinaryOperation::CreateSimplified(
          result->value, rhs.value,
          static_cast<CalcOperator>(operator_character));

      if (!result->value)
        return false;
    }

    return true;
  }

  bool ParseAdditiveValueExpression(CSSParserTokenRange& tokens,
                                    int depth,
                                    Value* result) {
    if (CheckDepthAndIndex(&depth, tokens) != OK)
      return false;

    if (!ParseValueMultiplicativeExpression(tokens, depth, result))
      return false;

    while (!tokens.AtEnd()) {
      char operator_character = OperatorValue(tokens.Peek());
      if (operator_character != kCalcAdd && operator_character != kCalcSubtract)
        break;
      if ((&tokens.Peek() - 1)->GetType() != kWhitespaceToken)
        return false;  // calc(1px+ 2px) is invalid
      tokens.Consume();
      if (tokens.Peek().GetType() != kWhitespaceToken)
        return false;  // calc(1px +2px) is invalid
      tokens.ConsumeIncludingWhitespace();

      Value rhs;
      if (!ParseValueMultiplicativeExpression(tokens, depth, &rhs))
        return false;

      result->value = CSSCalcBinaryOperation::CreateSimplified(
          result->value, rhs.value,
          static_cast<CalcOperator>(operator_character));

      if (!result->value)
        return false;
    }

    return true;
  }

  bool ParseValueExpression(CSSParserTokenRange& tokens,
                            int depth,
                            Value* result) {
    return ParseAdditiveValueExpression(tokens, depth, result);
  }
};

CSSCalcExpressionNode* CSSCalcValue::CreateExpressionNode(
    CSSPrimitiveValue* value,
    bool is_integer) {
  return CSSCalcPrimitiveValue::Create(value, is_integer);
}

CSSCalcExpressionNode* CSSCalcValue::CreateExpressionNode(
    CSSCalcExpressionNode* left_side,
    CSSCalcExpressionNode* right_side,
    CalcOperator op) {
  return CSSCalcBinaryOperation::Create(left_side, right_side, op);
}

CSSCalcExpressionNode* CSSCalcValue::CreateExpressionNode(double pixels,
                                                          double percent) {
  return CreateExpressionNode(
      CreateExpressionNode(CSSPrimitiveValue::Create(
                               pixels, CSSPrimitiveValue::UnitType::kPixels),
                           pixels == trunc(pixels)),
      CreateExpressionNode(
          CSSPrimitiveValue::Create(percent,
                                    CSSPrimitiveValue::UnitType::kPercentage),
          percent == trunc(percent)),
      kCalcAdd);
}

CSSCalcValue* CSSCalcValue::Create(const CSSParserTokenRange& tokens,
                                   ValueRange range) {
  CSSCalcExpressionNodeParser parser;
  CSSCalcExpressionNode* expression = parser.ParseCalc(tokens);

  return expression ? new CSSCalcValue(expression, range) : nullptr;
}

CSSCalcValue* CSSCalcValue::Create(CSSCalcExpressionNode* expression,
                                   ValueRange range) {
  return new CSSCalcValue(expression, range);
}

}  // namespace blink
