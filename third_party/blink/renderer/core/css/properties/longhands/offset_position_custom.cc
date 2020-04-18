// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/offset_position.h"

#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

using namespace CSSPropertyParserHelpers;

const CSSValue* OffsetPosition::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueAuto)
    return ConsumeIdent(range);
  CSSValue* value = ConsumePosition(range, context, UnitlessQuirk::kForbid,
                                    base::Optional<WebFeature>());

  // Count when we receive a valid position other than 'auto'.
  if (value && value->IsValuePair())
    context.Count(WebFeature::kCSSOffsetInEffect);
  return value;
}

const CSSValue* OffsetPosition::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForPosition(style.OffsetPosition(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
