// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/offset_rotate.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* OffsetRotate::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeOffsetRotate(range, context);
}
const CSSValue* OffsetRotate::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  if (style.OffsetRotate().type == OffsetRotationType::kAuto)
    list->Append(*CSSIdentifierValue::Create(CSSValueAuto));
  list->Append(*CSSPrimitiveValue::Create(
      style.OffsetRotate().angle, CSSPrimitiveValue::UnitType::kDegrees));
  return list;
}

}  // namespace CSSLonghand
}  // namespace blink
