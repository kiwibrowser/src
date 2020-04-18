/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/svg/svg_length.h"

#include "third_party/blink/renderer/core/css/css_primitive_value.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/svg/svg_animation_element.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

SVGLength::SVGLength(SVGLengthMode mode)
    : value_(
          CSSPrimitiveValue::Create(0,
                                    CSSPrimitiveValue::UnitType::kUserUnits)),
      unit_mode_(static_cast<unsigned>(mode)) {
  DCHECK_EQ(UnitMode(), mode);
}

SVGLength::SVGLength(const SVGLength& o)
    : value_(o.value_), unit_mode_(o.unit_mode_) {}

void SVGLength::Trace(blink::Visitor* visitor) {
  visitor->Trace(value_);
  SVGPropertyBase::Trace(visitor);
}

SVGLength* SVGLength::Clone() const {
  return new SVGLength(*this);
}

SVGPropertyBase* SVGLength::CloneForAnimation(const String& value) const {
  SVGLength* length = Create();
  length->unit_mode_ = unit_mode_;

  if (length->SetValueAsString(value) != SVGParseStatus::kNoError)
    length->value_ =
        CSSPrimitiveValue::Create(0, CSSPrimitiveValue::UnitType::kUserUnits);

  return length;
}

bool SVGLength::operator==(const SVGLength& other) const {
  return unit_mode_ == other.unit_mode_ && value_ == other.value_;
}

float SVGLength::Value(const SVGLengthContext& context) const {
  if (IsCalculated())
    return context.ResolveValue(AsCSSPrimitiveValue(), UnitMode());

  return context.ConvertValueToUserUnits(value_->GetFloatValue(), UnitMode(),
                                         value_->TypeWithCalcResolved());
}

void SVGLength::SetValueAsNumber(float value) {
  value_ =
      CSSPrimitiveValue::Create(value, CSSPrimitiveValue::UnitType::kUserUnits);
}

void SVGLength::SetValue(float value, const SVGLengthContext& context) {
  value_ = CSSPrimitiveValue::Create(
      context.ConvertValueFromUserUnits(value, UnitMode(),
                                        value_->TypeWithCalcResolved()),
      value_->TypeWithCalcResolved());
}

static bool IsCalcCSSUnitType(CSSPrimitiveValue::UnitType type) {
  return type >= CSSPrimitiveValue::UnitType::kCalc &&
         type <=
             CSSPrimitiveValue::UnitType::kCalcPercentageWithLengthAndNumber;
}

static bool IsSupportedCSSUnitType(CSSPrimitiveValue::UnitType type) {
  return (CSSPrimitiveValue::IsLength(type) ||
          type == CSSPrimitiveValue::UnitType::kNumber ||
          type == CSSPrimitiveValue::UnitType::kPercentage ||
          IsCalcCSSUnitType(type)) &&
         type != CSSPrimitiveValue::UnitType::kQuirkyEms;
}

void SVGLength::SetUnitType(CSSPrimitiveValue::UnitType type) {
  DCHECK(IsSupportedCSSUnitType(type));
  value_ = CSSPrimitiveValue::Create(value_->GetFloatValue(), type);
}

float SVGLength::ValueAsPercentage() const {
  // LengthTypePercentage is represented with 100% = 100.0. Good for accuracy
  // but could eventually be changed.
  if (value_->IsPercentage()) {
    // Note: This division is a source of floating point inaccuracy.
    return value_->GetFloatValue() / 100;
  }

  return value_->GetFloatValue();
}

float SVGLength::ValueAsPercentage100() const {
  // LengthTypePercentage is represented with 100% = 100.0. Good for accuracy
  // but could eventually be changed.
  if (value_->IsPercentage())
    return value_->GetFloatValue();

  return value_->GetFloatValue() * 100;
}

float SVGLength::ScaleByPercentage(float input) const {
  float result = input * value_->GetFloatValue();
  if (value_->IsPercentage()) {
    // Delaying division by 100 as long as possible since it introduces floating
    // point errors.
    result = result / 100;
  }
  return result;
}

SVGParsingError SVGLength::SetValueAsString(const String& string) {
  // TODO(fs): Preferably we wouldn't need to special-case the null
  // string (which we'll get for example for removeAttribute.)
  // Hopefully work on crbug.com/225807 can help here.
  if (string.IsNull()) {
    value_ =
        CSSPrimitiveValue::Create(0, CSSPrimitiveValue::UnitType::kUserUnits);
    return SVGParseStatus::kNoError;
  }

  // NOTE(ikilpatrick): We will always parse svg lengths in the insecure
  // context mode. If a function/unit/etc will require a secure context check
  // in the future, plumbing will need to be added.
  CSSParserContext* svg_parser_context = CSSParserContext::Create(
      kSVGAttributeMode, SecureContextMode::kInsecureContext);
  const CSSValue* parsed =
      CSSParser::ParseSingleValue(CSSPropertyX, string, svg_parser_context);
  if (!parsed || !parsed->IsPrimitiveValue())
    return SVGParseStatus::kExpectedLength;

  const CSSPrimitiveValue* new_value = ToCSSPrimitiveValue(parsed);
  if (!IsSupportedCSSUnitType(new_value->TypeWithCalcResolved()))
    return SVGParseStatus::kExpectedLength;

  value_ = new_value;
  return SVGParseStatus::kNoError;
}

String SVGLength::ValueAsString() const {
  return value_->CustomCSSText();
}

void SVGLength::NewValueSpecifiedUnits(CSSPrimitiveValue::UnitType type,
                                       float value) {
  value_ = CSSPrimitiveValue::Create(value, type);
}

void SVGLength::ConvertToSpecifiedUnits(CSSPrimitiveValue::UnitType type,
                                        const SVGLengthContext& context) {
  DCHECK(IsSupportedCSSUnitType(type));

  float value_in_user_units = Value(context);
  value_ = CSSPrimitiveValue::Create(
      context.ConvertValueFromUserUnits(value_in_user_units, UnitMode(), type),
      type);
}

SVGLengthMode SVGLength::LengthModeForAnimatedLengthAttribute(
    const QualifiedName& attr_name) {
  typedef HashMap<QualifiedName, SVGLengthMode> LengthModeForLengthAttributeMap;
  DEFINE_STATIC_LOCAL(LengthModeForLengthAttributeMap, length_mode_map, ());

  if (length_mode_map.IsEmpty()) {
    length_mode_map.Set(SVGNames::xAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::yAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::cxAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::cyAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::dxAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::dyAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::frAttr, SVGLengthMode::kOther);
    length_mode_map.Set(SVGNames::fxAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::fyAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::rAttr, SVGLengthMode::kOther);
    length_mode_map.Set(SVGNames::rxAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::ryAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::widthAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::heightAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::x1Attr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::x2Attr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::y1Attr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::y2Attr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::refXAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::refYAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::markerWidthAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::markerHeightAttr, SVGLengthMode::kHeight);
    length_mode_map.Set(SVGNames::textLengthAttr, SVGLengthMode::kWidth);
    length_mode_map.Set(SVGNames::startOffsetAttr, SVGLengthMode::kWidth);
  }

  if (length_mode_map.Contains(attr_name))
    return length_mode_map.at(attr_name);

  return SVGLengthMode::kOther;
}

bool SVGLength::NegativeValuesForbiddenForAnimatedLengthAttribute(
    const QualifiedName& attr_name) {
  DEFINE_STATIC_LOCAL(
      HashSet<QualifiedName>, no_negative_values_set,
      ({
          SVGNames::frAttr, SVGNames::rAttr, SVGNames::rxAttr, SVGNames::ryAttr,
          SVGNames::widthAttr, SVGNames::heightAttr, SVGNames::markerWidthAttr,
          SVGNames::markerHeightAttr, SVGNames::textLengthAttr,
      }));
  return no_negative_values_set.Contains(attr_name);
}

void SVGLength::Add(SVGPropertyBase* other, SVGElement* context_element) {
  SVGLengthContext length_context(context_element);
  SetValue(Value(length_context) + ToSVGLength(other)->Value(length_context),
           length_context);
}

void SVGLength::CalculateAnimatedValue(
    SVGAnimationElement* animation_element,
    float percentage,
    unsigned repeat_count,
    SVGPropertyBase* from_value,
    SVGPropertyBase* to_value,
    SVGPropertyBase* to_at_end_of_duration_value,
    SVGElement* context_element) {
  SVGLength* from_length = ToSVGLength(from_value);
  SVGLength* to_length = ToSVGLength(to_value);
  SVGLength* to_at_end_of_duration_length =
      ToSVGLength(to_at_end_of_duration_value);

  SVGLengthContext length_context(context_element);
  float animated_number = Value(length_context);
  animation_element->AnimateAdditiveNumber(
      percentage, repeat_count, from_length->Value(length_context),
      to_length->Value(length_context),
      to_at_end_of_duration_length->Value(length_context), animated_number);

  DCHECK_EQ(UnitMode(), LengthModeForAnimatedLengthAttribute(
                            animation_element->AttributeName()));

  // TODO(shanmuga.m): Construct a calc() expression if the units fall in
  // different categories.
  CSSPrimitiveValue::UnitType new_unit =
      CSSPrimitiveValue::UnitType::kUserUnits;
  if (percentage < 0.5) {
    if (!from_length->IsCalculated())
      new_unit = from_length->TypeWithCalcResolved();
  } else {
    if (!to_length->IsCalculated())
      new_unit = to_length->TypeWithCalcResolved();
  }
  animated_number = length_context.ConvertValueFromUserUnits(
      animated_number, UnitMode(), new_unit);
  value_ = CSSPrimitiveValue::Create(animated_number, new_unit);
}

float SVGLength::CalculateDistance(SVGPropertyBase* to_value,
                                   SVGElement* context_element) {
  SVGLengthContext length_context(context_element);
  SVGLength* to_length = ToSVGLength(to_value);

  return fabsf(to_length->Value(length_context) - Value(length_context));
}

}  // namespace blink
