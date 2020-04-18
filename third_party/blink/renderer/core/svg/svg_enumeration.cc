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

#include "third_party/blink/renderer/core/svg/svg_enumeration.h"

#include "third_party/blink/renderer/core/svg/svg_animation_element.h"

namespace blink {

DEFINE_SVG_PROPERTY_TYPE_CASTS(SVGEnumerationBase);

SVGEnumerationBase::~SVGEnumerationBase() = default;

SVGPropertyBase* SVGEnumerationBase::CloneForAnimation(
    const String& value) const {
  SVGEnumerationBase* svg_enumeration = Clone();
  svg_enumeration->SetValueAsString(value);
  return svg_enumeration;
}

String SVGEnumerationBase::ValueAsString() const {
  for (const auto& entry : entries_) {
    if (value_ == entry.first)
      return entry.second;
  }

  DCHECK_LT(value_, MaxInternalEnumValue());
  return g_empty_string;
}

void SVGEnumerationBase::SetValue(unsigned short value) {
  value_ = value;
  NotifyChange();
}

SVGParsingError SVGEnumerationBase::SetValueAsString(const String& string) {
  for (const auto& entry : entries_) {
    if (string == entry.second) {
      // 0 corresponds to _UNKNOWN enumeration values, and should not be
      // settable.
      DCHECK(entry.first);
      value_ = entry.first;
      NotifyChange();
      return SVGParseStatus::kNoError;
    }
  }

  NotifyChange();
  return SVGParseStatus::kExpectedEnumeration;
}

void SVGEnumerationBase::Add(SVGPropertyBase*, SVGElement*) {
  NOTREACHED();
}

void SVGEnumerationBase::CalculateAnimatedValue(
    SVGAnimationElement* animation_element,
    float percentage,
    unsigned repeat_count,
    SVGPropertyBase* from,
    SVGPropertyBase* to,
    SVGPropertyBase*,
    SVGElement*) {
  DCHECK(animation_element);
  unsigned short from_enumeration =
      animation_element->GetAnimationMode() == kToAnimation
          ? value_
          : ToSVGEnumerationBase(from)->Value();
  unsigned short to_enumeration = ToSVGEnumerationBase(to)->Value();

  animation_element->AnimateDiscreteType<unsigned short>(
      percentage, from_enumeration, to_enumeration, value_);
}

float SVGEnumerationBase::CalculateDistance(SVGPropertyBase*, SVGElement*) {
  // No paced animations for boolean.
  return -1;
}

}  // namespace blink
