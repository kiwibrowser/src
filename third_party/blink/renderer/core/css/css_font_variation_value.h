// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_FONT_VARIATION_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_FONT_VARIATION_VALUE_H_

#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace cssvalue {

class CSSFontVariationValue : public CSSValue {
 public:
  static CSSFontVariationValue* Create(const AtomicString& tag, float value) {
    return new CSSFontVariationValue(tag, value);
  }

  const AtomicString& Tag() const { return tag_; }
  float Value() const { return value_; }
  String CustomCSSText() const;

  bool Equals(const CSSFontVariationValue&) const;

  void TraceAfterDispatch(blink::Visitor* visitor) {
    CSSValue::TraceAfterDispatch(visitor);
  }

 private:
  CSSFontVariationValue(const AtomicString& tag, float value);

  AtomicString tag_;
  const float value_;
};

DEFINE_CSS_VALUE_TYPE_CASTS(CSSFontVariationValue, IsFontVariationValue());

}  // namespace cssvalue
}  // namespace blink

#endif
