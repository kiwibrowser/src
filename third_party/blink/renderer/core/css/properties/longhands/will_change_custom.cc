// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/will_change.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* WillChange::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValueList* values = CSSValueList::CreateCommaSeparated();
  // Every comma-separated list of identifiers is a valid will-change value,
  // unless the list includes an explicitly disallowed identifier.
  while (true) {
    if (range.Peek().GetType() != kIdentToken)
      return nullptr;
    CSSPropertyID unresolved_property =
        UnresolvedCSSPropertyID(range.Peek().Value());
    if (unresolved_property != CSSPropertyInvalid &&
        unresolved_property != CSSPropertyVariable) {
#if DCHECK_IS_ON()
      DCHECK(CSSProperty::Get(resolveCSSPropertyID(unresolved_property))
                 .IsEnabled());
#endif
      // Now "all" is used by both CSSValue and CSSPropertyValue.
      // Need to return nullptr when currentValue is CSSPropertyAll.
      if (unresolved_property == CSSPropertyWillChange ||
          unresolved_property == CSSPropertyAll)
        return nullptr;
      values->Append(*CSSCustomIdentValue::Create(unresolved_property));
      range.ConsumeIncludingWhitespace();
    } else {
      switch (range.Peek().Id()) {
        case CSSValueNone:
        case CSSValueAll:
        case CSSValueAuto:
        case CSSValueDefault:
        case CSSValueInitial:
        case CSSValueInherit:
          return nullptr;
        case CSSValueContents:
        case CSSValueScrollPosition:
          values->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
          break;
        default:
          range.ConsumeIncludingWhitespace();
          break;
      }
    }

    if (range.AtEnd())
      break;
    if (!CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range))
      return nullptr;
  }

  return values;
}

const CSSValue* WillChange::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForWillChange(
      style.WillChangeProperties(), style.WillChangeContents(),
      style.WillChangeScrollPosition());
}

}  // namespace CSSLonghand
}  // namespace blink
