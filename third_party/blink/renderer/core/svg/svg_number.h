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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_NUMBER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_NUMBER_H_

#include "third_party/blink/renderer/core/svg/properties/svg_property_helper.h"
#include "third_party/blink/renderer/core/svg/svg_parsing_error.h"

namespace blink {

class SVGNumberTearOff;

class SVGNumber : public SVGPropertyHelper<SVGNumber> {
 public:
  // SVGNumber has a tear-off type, but SVGAnimatedNumber uses primitive type.
  typedef SVGNumberTearOff TearOffType;
  typedef float PrimitiveType;

  static SVGNumber* Create(float value = 0.0f) { return new SVGNumber(value); }

  virtual SVGNumber* Clone() const;

  float Value() const { return value_; }
  void SetValue(float value) { value_ = value; }

  String ValueAsString() const override;
  virtual SVGParsingError SetValueAsString(const String&);

  void Add(SVGPropertyBase*, SVGElement*) override;
  void CalculateAnimatedValue(SVGAnimationElement*,
                              float percentage,
                              unsigned repeat_count,
                              SVGPropertyBase* from,
                              SVGPropertyBase* to,
                              SVGPropertyBase* to_at_end_of_duration_value,
                              SVGElement* context_element) override;
  float CalculateDistance(SVGPropertyBase* to,
                          SVGElement* context_element) override;

  static AnimatedPropertyType ClassType() { return kAnimatedNumber; }

 protected:
  explicit SVGNumber(float);

  template <typename CharType>
  SVGParsingError Parse(const CharType*& ptr, const CharType* end);

  float value_;
};

DEFINE_SVG_PROPERTY_TYPE_CASTS(SVGNumber);

// SVGNumber which also accepts percentage as its value.
// This is used for <stop> "offset"
// Spec: http://www.w3.org/TR/SVG11/pservers.html#GradientStops
//   offset = "<number> | <percentage>"
class SVGNumberAcceptPercentage final : public SVGNumber {
 public:
  static SVGNumberAcceptPercentage* Create(float value = 0) {
    return new SVGNumberAcceptPercentage(value);
  }

  SVGNumber* Clone() const override;
  SVGParsingError SetValueAsString(const String&) override;

 private:
  explicit SVGNumberAcceptPercentage(float);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_NUMBER_H_
