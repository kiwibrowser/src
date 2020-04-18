// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/vertical_align.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* VerticalAlign::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValue* parsed_value = CSSPropertyParserHelpers::ConsumeIdentRange(
      range, CSSValueBaseline, CSSValueWebkitBaselineMiddle);
  if (!parsed_value) {
    parsed_value = CSSPropertyParserHelpers::ConsumeLengthOrPercent(
        range, context.Mode(), kValueRangeAll,
        CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
  }
  return parsed_value;
}

const CSSValue* VerticalAlign::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  switch (style.VerticalAlign()) {
    case EVerticalAlign::kBaseline:
      return CSSIdentifierValue::Create(CSSValueBaseline);
    case EVerticalAlign::kMiddle:
      return CSSIdentifierValue::Create(CSSValueMiddle);
    case EVerticalAlign::kSub:
      return CSSIdentifierValue::Create(CSSValueSub);
    case EVerticalAlign::kSuper:
      return CSSIdentifierValue::Create(CSSValueSuper);
    case EVerticalAlign::kTextTop:
      return CSSIdentifierValue::Create(CSSValueTextTop);
    case EVerticalAlign::kTextBottom:
      return CSSIdentifierValue::Create(CSSValueTextBottom);
    case EVerticalAlign::kTop:
      return CSSIdentifierValue::Create(CSSValueTop);
    case EVerticalAlign::kBottom:
      return CSSIdentifierValue::Create(CSSValueBottom);
    case EVerticalAlign::kBaselineMiddle:
      return CSSIdentifierValue::Create(CSSValueWebkitBaselineMiddle);
    case EVerticalAlign::kLength:
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          style.GetVerticalAlignLength(), style);
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace CSSLonghand
}  // namespace blink
