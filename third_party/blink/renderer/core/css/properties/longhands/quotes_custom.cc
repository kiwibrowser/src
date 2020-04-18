// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/quotes.h"

#include "third_party/blink/renderer/core/css/css_string_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Quotes::ParseSingleValue(CSSParserTokenRange& range,
                                         const CSSParserContext& context,
                                         const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueNone)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  CSSValueList* values = CSSValueList::CreateSpaceSeparated();
  while (!range.AtEnd()) {
    CSSStringValue* parsed_value =
        CSSPropertyParserHelpers::ConsumeString(range);
    if (!parsed_value)
      return nullptr;
    values->Append(*parsed_value);
  }
  if (values->length() && values->length() % 2 == 0)
    return values;
  return nullptr;
}

const CSSValue* Quotes::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (!style.Quotes()) {
    // TODO(ramya.v): We should return the quote values that we're actually
    // using.
    return nullptr;
  }
  if (style.Quotes()->size()) {
    CSSValueList* list = CSSValueList::CreateSpaceSeparated();
    for (int i = 0; i < style.Quotes()->size(); i++) {
      list->Append(*CSSStringValue::Create(style.Quotes()->GetOpenQuote(i)));
      list->Append(*CSSStringValue::Create(style.Quotes()->GetCloseQuote(i)));
    }
    return list;
  }
  return CSSIdentifierValue::Create(CSSValueNone);
}

}  // namespace CSSLonghand
}  // namespace blink
