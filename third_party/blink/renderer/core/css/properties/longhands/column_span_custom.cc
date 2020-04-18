// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/column_span.h"

#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* ColumnSpan::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSPropertyParserHelpers::ConsumeIdent<CSSValueAll, CSSValueNone>(
      range);
}

const CSSValue* ColumnSpan::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return CSSIdentifierValue::Create(static_cast<unsigned>(style.GetColumnSpan())
                                        ? CSSValueAll
                                        : CSSValueNone);
}

}  // namespace CSSLonghand
}  // namespace blink
