// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/scroll_customization.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

class CSSParserLocalContext;
namespace blink {

namespace {

static bool ConsumePan(CSSParserTokenRange& range,
                       CSSValue** pan_x,
                       CSSValue** pan_y) {
  CSSValueID id = range.Peek().Id();
  if ((id == CSSValuePanX || id == CSSValuePanRight || id == CSSValuePanLeft) &&
      !*pan_x) {
    *pan_x = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else if ((id == CSSValuePanY || id == CSSValuePanDown ||
              id == CSSValuePanUp) &&
             !*pan_y) {
    *pan_y = CSSPropertyParserHelpers::ConsumeIdent(range);
  } else {
    return false;
  }
  return true;
}

}  // namespace
namespace CSSLonghand {

const CSSValue* ScrollCustomization::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueAuto || id == CSSValueNone) {
    list->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
    return list;
  }

  CSSValue* pan_x = nullptr;
  CSSValue* pan_y = nullptr;
  if (!ConsumePan(range, &pan_x, &pan_y))
    return nullptr;
  if (!range.AtEnd() && !ConsumePan(range, &pan_x, &pan_y))
    return nullptr;

  if (pan_x)
    list->Append(*pan_x);
  if (pan_y)
    list->Append(*pan_y);
  return list;
}

const CSSValue* ScrollCustomization::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ScrollCustomizationFlagsToCSSValue(
      style.ScrollCustomization());
}

}  // namespace CSSLonghand
}  // namespace blink
