// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/background_position.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool BackgroundPosition::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* result_x = nullptr;
  CSSValue* result_y = nullptr;

  if (!CSSParsingUtils::ConsumeBackgroundPosition(
          range, context, CSSPropertyParserHelpers::UnitlessQuirk::kAllow,
          result_x, result_y) ||
      !range.AtEnd())
    return false;

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBackgroundPositionX, CSSPropertyBackgroundPosition, *result_x,
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBackgroundPositionY, CSSPropertyBackgroundPosition, *result_y,
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  return true;
}

const CSSValue* BackgroundPosition::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::BackgroundPositionOrWebkitMaskPosition(
      *this, style, &style.BackgroundLayers());
}

}  // namespace CSSShorthand
}  // namespace blink
