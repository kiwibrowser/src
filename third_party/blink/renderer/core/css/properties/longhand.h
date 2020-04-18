// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_LONGHAND_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_LONGHAND_H_

#include "third_party/blink/renderer/core/css/properties/css_property.h"

#include "third_party/blink/renderer/core/css/css_initial_value.h"
#include "third_party/blink/renderer/platform/graphics/color.h"

namespace blink {

class CSSValue;
class StyleResolverState;

class Longhand : public CSSProperty {
 public:
  // Parses and consumes a longhand property value from the token range.
  // Returns nullptr if the input is invalid.
  virtual const CSSValue* ParseSingleValue(CSSParserTokenRange&,
                                           const CSSParserContext&,
                                           const CSSParserLocalContext&) const {
    return nullptr;
  }
  virtual void ApplyInitial(StyleResolverState&) const { NOTREACHED(); }
  virtual void ApplyInherit(StyleResolverState&) const { NOTREACHED(); }
  virtual void ApplyValue(StyleResolverState&, const CSSValue&) const {
    NOTREACHED();
  }
  virtual const blink::Color ColorIncludingFallback(bool, const ComputedStyle&)
      const {
    NOTREACHED();
    return Color();
  }
  bool IsLonghand() const override { return true; }
  virtual const CSSValue* InitialValue() const {
    return CSSInitialValue::Create();
  }

 protected:
  constexpr Longhand() : CSSProperty() {}
};

DEFINE_TYPE_CASTS(Longhand,
                  CSSProperty,
                  longhand,
                  longhand->IsLonghand(),
                  longhand.IsLonghand());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_LONGHAND_H_
