// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/perspective.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_local_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/zoom_adjusted_pixel_value.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Perspective::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& localContext) const {
  if (range.Peek().Id() == CSSValueNone)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  CSSPrimitiveValue* parsed_value = CSSPropertyParserHelpers::ConsumeLength(
      range, context.Mode(), kValueRangeAll);
  bool use_legacy_parsing = localContext.UseAliasParsing();
  if (!parsed_value && use_legacy_parsing) {
    double perspective;
    if (!CSSPropertyParserHelpers::ConsumeNumberRaw(range, perspective))
      return nullptr;
    context.Count(WebFeature::kUnitlessPerspectiveInPerspectiveProperty);
    parsed_value = CSSPrimitiveValue::Create(
        perspective, CSSPrimitiveValue::UnitType::kPixels);
  }
  if (parsed_value &&
      (parsed_value->IsCalculated() || parsed_value->GetDoubleValue() > 0))
    return parsed_value;
  return nullptr;
}

const CSSValue* Perspective::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (!style.HasPerspective())
    return CSSIdentifierValue::Create(CSSValueNone);
  return ZoomAdjustedPixelValue(style.Perspective(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
