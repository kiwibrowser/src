// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/mask_source_type.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* MaskSourceType::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext&,
    const CSSParserLocalContext&) const {
  return CSSPropertyParserHelpers::ConsumeCommaSeparatedList(
      CSSParsingUtils::ConsumeMaskSourceType, range);
}

static CSSValue* ValueForFillSourceType(EMaskSourceType type) {
  switch (type) {
    case EMaskSourceType::kAlpha:
      return CSSIdentifierValue::Create(CSSValueAlpha);
    case EMaskSourceType::kLuminance:
      return CSSIdentifierValue::Create(CSSValueLuminance);
  }
  NOTREACHED();
  return nullptr;
}

const CSSValue* MaskSourceType::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  CSSValueList* list = CSSValueList::CreateCommaSeparated();
  for (const FillLayer* curr_layer = &style.MaskLayers(); curr_layer;
       curr_layer = curr_layer->Next())
    list->Append(*ValueForFillSourceType(curr_layer->MaskSourceType()));
  return list;
}

}  // namespace CSSLonghand
}  // namespace blink
