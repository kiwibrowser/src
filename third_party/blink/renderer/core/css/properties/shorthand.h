// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_SHORTHAND_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_SHORTHAND_H_

#include "third_party/blink/renderer/core/css/properties/css_property.h"

namespace blink {

class CSSPropertyValue;

class Shorthand : public CSSProperty {
 public:
  // Parses and consumes entire shorthand value from the token range and adds
  // all constituent parsed longhand properties to the 'properties' set.
  // Returns false if the input is invalid.
  virtual bool ParseShorthand(
      bool important,
      CSSParserTokenRange&,
      const CSSParserContext&,
      const CSSParserLocalContext&,
      HeapVector<CSSPropertyValue, 256>& properties) const {
    NOTREACHED();
    return false;
  }
  bool IsShorthand() const override { return true; }

 protected:
  constexpr Shorthand() : CSSProperty() {}
};

DEFINE_TYPE_CASTS(Shorthand,
                  CSSProperty,
                  shorthand,
                  shorthand->IsShorthand(),
                  shorthand.IsShorthand());

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PROPERTIES_SHORTHAND_H_
