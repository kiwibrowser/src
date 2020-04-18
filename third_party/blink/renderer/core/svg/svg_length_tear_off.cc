/*
 * Copyright (C) 2014 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/svg/svg_length_tear_off.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/exception_code.h"
#include "third_party/blink/renderer/core/svg/svg_element.h"

namespace blink {

namespace {

inline bool IsValidLengthUnit(CSSPrimitiveValue::UnitType unit) {
  return unit == CSSPrimitiveValue::UnitType::kNumber ||
         unit == CSSPrimitiveValue::UnitType::kPercentage ||
         unit == CSSPrimitiveValue::UnitType::kEms ||
         unit == CSSPrimitiveValue::UnitType::kExs ||
         unit == CSSPrimitiveValue::UnitType::kPixels ||
         unit == CSSPrimitiveValue::UnitType::kCentimeters ||
         unit == CSSPrimitiveValue::UnitType::kMillimeters ||
         unit == CSSPrimitiveValue::UnitType::kInches ||
         unit == CSSPrimitiveValue::UnitType::kPoints ||
         unit == CSSPrimitiveValue::UnitType::kPicas;
}

inline bool IsValidLengthUnit(unsigned short type) {
  return IsValidLengthUnit(static_cast<CSSPrimitiveValue::UnitType>(type));
}

inline bool CanResolveRelativeUnits(const SVGElement* context_element) {
  return context_element && context_element->isConnected();
}

inline CSSPrimitiveValue::UnitType ToCSSUnitType(unsigned short type) {
  DCHECK(IsValidLengthUnit(type));
  if (type == SVGLengthTearOff::kSvgLengthtypeNumber)
    return CSSPrimitiveValue::UnitType::kUserUnits;
  return static_cast<CSSPrimitiveValue::UnitType>(type);
}

inline unsigned short ToInterfaceConstant(CSSPrimitiveValue::UnitType type) {
  switch (type) {
    case CSSPrimitiveValue::UnitType::kUnknown:
      return SVGLengthTearOff::kSvgLengthtypeUnknown;
    case CSSPrimitiveValue::UnitType::kUserUnits:
      return SVGLengthTearOff::kSvgLengthtypeNumber;
    case CSSPrimitiveValue::UnitType::kPercentage:
      return SVGLengthTearOff::kSvgLengthtypePercentage;
    case CSSPrimitiveValue::UnitType::kEms:
      return SVGLengthTearOff::kSvgLengthtypeEms;
    case CSSPrimitiveValue::UnitType::kExs:
      return SVGLengthTearOff::kSvgLengthtypeExs;
    case CSSPrimitiveValue::UnitType::kPixels:
      return SVGLengthTearOff::kSvgLengthtypePx;
    case CSSPrimitiveValue::UnitType::kCentimeters:
      return SVGLengthTearOff::kSvgLengthtypeCm;
    case CSSPrimitiveValue::UnitType::kMillimeters:
      return SVGLengthTearOff::kSvgLengthtypeMm;
    case CSSPrimitiveValue::UnitType::kInches:
      return SVGLengthTearOff::kSvgLengthtypeIn;
    case CSSPrimitiveValue::UnitType::kPoints:
      return SVGLengthTearOff::kSvgLengthtypePt;
    case CSSPrimitiveValue::UnitType::kPicas:
      return SVGLengthTearOff::kSvgLengthtypePc;
    default:
      return SVGLengthTearOff::kSvgLengthtypeUnknown;
  }
}

}  // namespace

bool SVGLengthTearOff::HasExposedLengthUnit() {
  if (Target()->IsCalculated())
    return false;

  CSSPrimitiveValue::UnitType unit = Target()->TypeWithCalcResolved();
  return IsValidLengthUnit(unit) ||
         unit == CSSPrimitiveValue::UnitType::kUnknown ||
         unit == CSSPrimitiveValue::UnitType::kUserUnits;
}

unsigned short SVGLengthTearOff::unitType() {
  return HasExposedLengthUnit()
             ? ToInterfaceConstant(Target()->TypeWithCalcResolved())
             : kSvgLengthtypeUnknown;
}

SVGLengthMode SVGLengthTearOff::UnitMode() {
  return Target()->UnitMode();
}

float SVGLengthTearOff::value(ExceptionState& exception_state) {
  if (Target()->IsRelative() && !CanResolveRelativeUnits(contextElement())) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Could not resolve relative length.");
    return 0;
  }
  SVGLengthContext length_context(contextElement());
  return Target()->Value(length_context);
}

void SVGLengthTearOff::setValue(float value, ExceptionState& exception_state) {
  if (IsImmutable()) {
    ThrowReadOnly(exception_state);
    return;
  }
  if (Target()->IsRelative() && !CanResolveRelativeUnits(contextElement())) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Could not resolve relative length.");
    return;
  }
  SVGLengthContext length_context(contextElement());
  if (Target()->IsCalculated())
    Target()->SetValueAsNumber(value);
  else
    Target()->SetValue(value, length_context);
  CommitChange();
}

float SVGLengthTearOff::valueInSpecifiedUnits() {
  if (Target()->IsCalculated())
    return 0;
  return Target()->ValueInSpecifiedUnits();
}

void SVGLengthTearOff::setValueInSpecifiedUnits(
    float value,
    ExceptionState& exception_state) {
  if (IsImmutable()) {
    ThrowReadOnly(exception_state);
    return;
  }
  if (Target()->IsCalculated())
    Target()->SetValueAsNumber(value);
  else
    Target()->SetValueInSpecifiedUnits(value);
  CommitChange();
}

String SVGLengthTearOff::valueAsString() {
  // TODO(shanmuga.m@samsung.com): Not all <length> properties have 0 (with no
  // unit) as the default (lacuna) value. We need to return default value
  // instead of 0.
  return HasExposedLengthUnit() ? Target()->ValueAsString() : String::Number(0);
}

void SVGLengthTearOff::setValueAsString(const String& str,
                                        ExceptionState& exception_state) {
  if (IsImmutable()) {
    ThrowReadOnly(exception_state);
    return;
  }
  String old_value = Target()->ValueAsString();
  SVGParsingError status = Target()->SetValueAsString(str);
  if (status == SVGParseStatus::kNoError && !HasExposedLengthUnit()) {
    Target()->SetValueAsString(old_value);  // rollback to old value
    status = SVGParseStatus::kParsingFailed;
  }
  if (status != SVGParseStatus::kNoError) {
    exception_state.ThrowDOMException(
        kSyntaxError, "The value provided ('" + str + "') is invalid.");
    return;
  }
  CommitChange();
}

void SVGLengthTearOff::newValueSpecifiedUnits(unsigned short unit_type,
                                              float value_in_specified_units,
                                              ExceptionState& exception_state) {
  if (IsImmutable()) {
    ThrowReadOnly(exception_state);
    return;
  }
  if (!IsValidLengthUnit(unit_type)) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "Cannot set value with unknown or invalid units (" +
                                String::Number(unit_type) + ").");
    return;
  }
  Target()->NewValueSpecifiedUnits(ToCSSUnitType(unit_type),
                                   value_in_specified_units);
  CommitChange();
}

void SVGLengthTearOff::convertToSpecifiedUnits(
    unsigned short unit_type,
    ExceptionState& exception_state) {
  if (IsImmutable()) {
    ThrowReadOnly(exception_state);
    return;
  }
  if (!IsValidLengthUnit(unit_type)) {
    exception_state.ThrowDOMException(
        kNotSupportedError, "Cannot convert to unknown or invalid units (" +
                                String::Number(unit_type) + ").");
    return;
  }
  if ((Target()->IsRelative() ||
       CSSPrimitiveValue::IsRelativeUnit(ToCSSUnitType(unit_type))) &&
      !CanResolveRelativeUnits(contextElement())) {
    exception_state.ThrowDOMException(kNotSupportedError,
                                      "Could not resolve relative length.");
    return;
  }
  SVGLengthContext length_context(contextElement());
  Target()->ConvertToSpecifiedUnits(ToCSSUnitType(unit_type), length_context);
  CommitChange();
}

SVGLengthTearOff::SVGLengthTearOff(SVGLength* target,
                                   SVGElement* context_element,
                                   PropertyIsAnimValType property_is_anim_val,
                                   const QualifiedName& attribute_name)
    : SVGPropertyTearOff<SVGLength>(target,
                                    context_element,
                                    property_is_anim_val,
                                    attribute_name) {}

SVGLengthTearOff* SVGLengthTearOff::CreateDetached() {
  return Create(SVGLength::Create(), nullptr, kPropertyIsNotAnimVal,
                QualifiedName::Null());
}

}  // namespace blink
