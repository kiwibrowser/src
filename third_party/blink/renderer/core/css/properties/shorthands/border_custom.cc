// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/border.h"

#include "third_party/blink/renderer/core/css/css_initial_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool Border::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* width = nullptr;
  const CSSValue* style = nullptr;
  CSSValue* color = nullptr;

  while (!width || !style || !color) {
    if (!width) {
      width = CSSPropertyParserHelpers::ConsumeLineWidth(
          range, context.Mode(),
          CSSPropertyParserHelpers::UnitlessQuirk::kForbid);
      if (width)
        continue;
    }
    if (!style) {
      bool needs_legacy_parsing = false;
      style = CSSPropertyParserHelpers::ParseLonghand(
          CSSPropertyBorderLeftStyle, CSSPropertyBorder, context, range);
      DCHECK(!needs_legacy_parsing);
      if (style)
        continue;
    }
    if (!color) {
      color = CSSPropertyParserHelpers::ConsumeColor(range, context.Mode());
      if (color)
        continue;
    }
    break;
  }

  if (!width && !style && !color)
    return false;

  if (!width)
    width = CSSInitialValue::Create();
  if (!style)
    style = CSSInitialValue::Create();
  if (!color)
    color = CSSInitialValue::Create();

  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderWidth, *width, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderStyle, *style, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderColor, *color, important, properties);
  CSSPropertyParserHelpers::AddExpandedPropertyForValue(
      CSSPropertyBorderImage, *CSSInitialValue::Create(), important,
      properties);

  return range.AtEnd();
}

const CSSValue* Border::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  const CSSValue* value = GetCSSPropertyBorderTop().CSSValueFromComputedStyle(
      style, layout_object, styled_node, allow_visited_style);
  static const CSSProperty* kProperties[3] = {&GetCSSPropertyBorderRight(),
                                              &GetCSSPropertyBorderBottom(),
                                              &GetCSSPropertyBorderLeft()};
  for (size_t i = 0; i < arraysize(kProperties); ++i) {
    const CSSValue* value_for_side = kProperties[i]->CSSValueFromComputedStyle(
        style, layout_object, styled_node, allow_visited_style);
    if (!DataEquivalent(value, value_for_side)) {
      return nullptr;
    }
  }
  return value;
}

}  // namespace CSSShorthand
}  // namespace blink
