// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/justify_items.h"

#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* JustifyItems::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSParserTokenRange range_copy = range;
  // justify-items property does not allow the 'auto' value.
  if (CSSPropertyParserHelpers::IdentMatches<CSSValueAuto>(range.Peek().Id()))
    return nullptr;
  CSSIdentifierValue* legacy =
      CSSPropertyParserHelpers::ConsumeIdent<CSSValueLegacy>(range_copy);
  CSSIdentifierValue* position_keyword =
      CSSPropertyParserHelpers::ConsumeIdent<CSSValueCenter, CSSValueLeft,
                                             CSSValueRight>(range_copy);
  if (!legacy)
    legacy = CSSPropertyParserHelpers::ConsumeIdent<CSSValueLegacy>(range_copy);
  if (legacy) {
    range = range_copy;
    if (position_keyword) {
      context.Count(WebFeature::kCSSLegacyAlignment);
      return CSSValuePair::Create(legacy, position_keyword,
                                  CSSValuePair::kDropIdenticalValues);
    }
    return legacy;
  }

  return CSSParsingUtils::ConsumeSelfPositionOverflowPosition(
      range, CSSParsingUtils::IsSelfPositionOrLeftOrRightKeyword);
}

const CSSValue* JustifyItems::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForItemPositionWithOverflowAlignment(
      style.JustifyItems().GetPosition() == ItemPosition::kAuto
          ? ComputedStyleInitialValues::InitialDefaultAlignment()
          : style.JustifyItems());
}

}  // namespace CSSLonghand
}  // namespace blink
