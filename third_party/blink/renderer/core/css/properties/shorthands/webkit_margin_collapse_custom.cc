// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/webkit_margin_collapse.h"

#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_fast_paths.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"

namespace blink {
namespace CSSShorthand {

bool WebkitMarginCollapse::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValueID id = range.ConsumeIncludingWhitespace().Id();
  if (!CSSParserFastPaths::IsValidKeywordPropertyAndValue(
          CSSPropertyWebkitMarginBeforeCollapse, id, context.Mode()))
    return false;

  CSSValue* before_collapse = CSSIdentifierValue::Create(id);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMarginBeforeCollapse, CSSPropertyWebkitMarginCollapse,
      *before_collapse, important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);

  if (range.AtEnd()) {
    CSSPropertyParserHelpers::AddProperty(
        CSSPropertyWebkitMarginAfterCollapse, CSSPropertyWebkitMarginCollapse,
        *before_collapse, important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
    return true;
  }

  id = range.ConsumeIncludingWhitespace().Id();
  if (!CSSParserFastPaths::IsValidKeywordPropertyAndValue(
          CSSPropertyWebkitMarginAfterCollapse, id, context.Mode()))
    return false;
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMarginAfterCollapse, CSSPropertyWebkitMarginCollapse,
      *CSSIdentifierValue::Create(id), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  return true;
}

}  // namespace CSSShorthand
}  // namespace blink
