// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_UNSUPPORTED_STYLE_VALUE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_UNSUPPORTED_STYLE_VALUE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/css/cssom/css_style_value.h"

namespace blink {

// CSSUnsupportedStyleValue is the internal representation of a base
// CSSStyleValue that is returned when we do not yet support a CSS Typed OM type
// for a given CSS Value. It is tied to a specific CSS property and is only
// considered valid for that property.
class CORE_EXPORT CSSUnsupportedStyleValue final : public CSSStyleValue {
 public:
  static CSSUnsupportedStyleValue* Create(CSSPropertyID property,
                                          const String& css_text) {
    return new CSSUnsupportedStyleValue(property, css_text);
  }
  static CSSUnsupportedStyleValue* Create(CSSPropertyID property,
                                          const CSSValue& value) {
    return new CSSUnsupportedStyleValue(property, value.CssText());
  }

  StyleValueType GetType() const override {
    return StyleValueType::kUnknownType;
  }
  CSSPropertyID GetProperty() const { return property_; }
  const CSSValue* ToCSSValue() const override {
    NOTREACHED();
    return nullptr;
  }

  String toString() const final { return CSSText(); }

 private:
  CSSUnsupportedStyleValue(CSSPropertyID property, const String& css_text)
      : property_(property) {
    SetCSSText(css_text);
  }

  const CSSPropertyID property_;
  DISALLOW_COPY_AND_ASSIGN(CSSUnsupportedStyleValue);
};

DEFINE_TYPE_CASTS(CSSUnsupportedStyleValue,
                  CSSStyleValue,
                  value,
                  value->GetType() ==
                      CSSStyleValue::StyleValueType::kUnknownType,
                  value.GetType() ==
                      CSSStyleValue::StyleValueType::kUnknownType);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSSOM_CSS_UNSUPPORTED_STYLE_VALUE_H_
