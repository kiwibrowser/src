// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/text_underline_position.h"

#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* TextUnderlinePosition::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  // auto | [ under || [ left | right ] ], but we only support auto | under
  // for now
  return CSSPropertyParserHelpers::ConsumeIdent<CSSValueAuto, CSSValueUnder>(
      range);
}

const CSSValue* TextUnderlinePosition::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return CSSIdentifierValue::Create(style.GetTextUnderlinePosition());
}

}  // namespace CSSLonghand
}  // namespace blink
