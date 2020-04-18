// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/scale.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Scale::ParseSingleValue(CSSParserTokenRange& range,
                                        const CSSParserContext& context,
                                        const CSSParserLocalContext&) const {
  DCHECK(RuntimeEnabledFeatures::CSSIndependentTransformPropertiesEnabled());

  CSSValueID id = range.Peek().Id();
  if (id == CSSValueNone)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValue* scale =
      CSSPropertyParserHelpers::ConsumeNumber(range, kValueRangeAll);
  if (!scale)
    return nullptr;
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*scale);
  scale = CSSPropertyParserHelpers::ConsumeNumber(range, kValueRangeAll);
  if (scale) {
    list->Append(*scale);
    scale = CSSPropertyParserHelpers::ConsumeNumber(range, kValueRangeAll);
    if (scale)
      list->Append(*scale);
  }

  return list;
}

const CSSValue* Scale::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (!style.Scale())
    return CSSIdentifierValue::Create(CSSValueNone);
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*CSSPrimitiveValue::Create(
      style.Scale()->X(), CSSPrimitiveValue::UnitType::kNumber));
  list->Append(*CSSPrimitiveValue::Create(
      style.Scale()->Y(), CSSPrimitiveValue::UnitType::kNumber));
  if (style.Scale()->Z() != 1) {
    list->Append(*CSSPrimitiveValue::Create(
        style.Scale()->Z(), CSSPrimitiveValue::UnitType::kNumber));
  }
  return list;
}

}  // namespace CSSLonghand
}  // namespace blink
