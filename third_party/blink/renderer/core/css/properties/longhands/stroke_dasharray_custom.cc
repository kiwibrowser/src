// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/stroke_dasharray.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* StrokeDasharray::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueNone)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValueList* dashes = CSSValueList::CreateCommaSeparated();
  do {
    CSSPrimitiveValue* dash = CSSPropertyParserHelpers::ConsumeLengthOrPercent(
        range, kSVGAttributeMode, kValueRangeNonNegative);
    if (!dash ||
        (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range) &&
         range.AtEnd()))
      return nullptr;
    dashes->Append(*dash);
  } while (!range.AtEnd());
  return dashes;
}

const CSSValue* StrokeDasharray::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle& svg_style,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::StrokeDashArrayToCSSValueList(
      *svg_style.StrokeDashArray(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
