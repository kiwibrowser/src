// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/border_spacing.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/zoom_adjusted_pixel_value.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool BorderSpacing::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* horizontal_spacing =
      ConsumeLength(range, context.Mode(), kValueRangeNonNegative,
                    CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
  if (!horizontal_spacing)
    return false;
  CSSValue* vertical_spacing = horizontal_spacing;
  if (!range.AtEnd()) {
    vertical_spacing =
        ConsumeLength(range, context.Mode(), kValueRangeNonNegative,
                      CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
  }
  if (!vertical_spacing || !range.AtEnd())
    return false;
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitBorderHorizontalSpacing, CSSPropertyBorderSpacing,
      *horizontal_spacing, important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitBorderVerticalSpacing, CSSPropertyBorderSpacing,
      *vertical_spacing, important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  return true;
}

const CSSValue* BorderSpacing::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  CSSValueList* list = CSSValueList::CreateSpaceSeparated();
  list->Append(*ZoomAdjustedPixelValue(style.HorizontalBorderSpacing(), style));
  list->Append(*ZoomAdjustedPixelValue(style.VerticalBorderSpacing(), style));
  return list;
}

}  // namespace CSSShorthand
}  // namespace blink
