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

#include "third_party/blink/renderer/core/svg/svg_integer_optional_integer.h"

#include "third_party/blink/renderer/core/svg/svg_animation_element.h"
#include "third_party/blink/renderer/core/svg/svg_parser_utilities.h"

namespace blink {

SVGIntegerOptionalInteger::SVGIntegerOptionalInteger(SVGInteger* first_integer,
                                                     SVGInteger* second_integer)
    : first_integer_(first_integer), second_integer_(second_integer) {}

void SVGIntegerOptionalInteger::Trace(blink::Visitor* visitor) {
  visitor->Trace(first_integer_);
  visitor->Trace(second_integer_);
  SVGPropertyBase::Trace(visitor);
}

SVGIntegerOptionalInteger* SVGIntegerOptionalInteger::Clone() const {
  return SVGIntegerOptionalInteger::Create(first_integer_->Clone(),
                                           second_integer_->Clone());
}

SVGPropertyBase* SVGIntegerOptionalInteger::CloneForAnimation(
    const String& value) const {
  SVGIntegerOptionalInteger* clone =
      Create(SVGInteger::Create(0), SVGInteger::Create(0));
  clone->SetValueAsString(value);
  return clone;
}

String SVGIntegerOptionalInteger::ValueAsString() const {
  if (first_integer_->Value() == second_integer_->Value()) {
    return String::Number(first_integer_->Value());
  }

  return String::Number(first_integer_->Value()) + " " +
         String::Number(second_integer_->Value());
}

SVGParsingError SVGIntegerOptionalInteger::SetValueAsString(
    const String& value) {
  float x, y;
  SVGParsingError parse_status;
  if (!ParseNumberOptionalNumber(value, x, y)) {
    parse_status = SVGParseStatus::kExpectedInteger;
    x = y = 0;
  }

  first_integer_->SetValue(clampTo<int>(x));
  second_integer_->SetValue(clampTo<int>(y));
  return parse_status;
}

void SVGIntegerOptionalInteger::Add(SVGPropertyBase* other, SVGElement*) {
  SVGIntegerOptionalInteger* other_integer_optional_integer =
      ToSVGIntegerOptionalInteger(other);

  first_integer_->SetValue(
      first_integer_->Value() +
      other_integer_optional_integer->first_integer_->Value());
  second_integer_->SetValue(
      second_integer_->Value() +
      other_integer_optional_integer->second_integer_->Value());
}

void SVGIntegerOptionalInteger::CalculateAnimatedValue(
    SVGAnimationElement* animation_element,
    float percentage,
    unsigned repeat_count,
    SVGPropertyBase* from,
    SVGPropertyBase* to,
    SVGPropertyBase* to_at_end_of_duration,
    SVGElement*) {
  DCHECK(animation_element);

  SVGIntegerOptionalInteger* from_integer = ToSVGIntegerOptionalInteger(from);
  SVGIntegerOptionalInteger* to_integer = ToSVGIntegerOptionalInteger(to);
  SVGIntegerOptionalInteger* to_at_end_of_duration_integer =
      ToSVGIntegerOptionalInteger(to_at_end_of_duration);

  float x = first_integer_->Value();
  float y = second_integer_->Value();
  animation_element->AnimateAdditiveNumber(
      percentage, repeat_count, from_integer->FirstInteger()->Value(),
      to_integer->FirstInteger()->Value(),
      to_at_end_of_duration_integer->FirstInteger()->Value(), x);
  animation_element->AnimateAdditiveNumber(
      percentage, repeat_count, from_integer->SecondInteger()->Value(),
      to_integer->SecondInteger()->Value(),
      to_at_end_of_duration_integer->SecondInteger()->Value(), y);
  first_integer_->SetValue(clampTo<int>(roundf(x)));
  second_integer_->SetValue(clampTo<int>(roundf(y)));
}

float SVGIntegerOptionalInteger::CalculateDistance(SVGPropertyBase* other,
                                                   SVGElement*) {
  // FIXME: Distance calculation is not possible for SVGIntegerOptionalInteger
  // right now. We need the distance for every single value.
  return -1;
}

}  // namespace blink
