// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/place_content.h"

#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/css/properties/longhand.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/core/style_property_shorthand.h"

namespace blink {
namespace CSSShorthand {

bool PlaceContent::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  DCHECK_EQ(shorthandForProperty(CSSPropertyPlaceContent).length(), 2u);

  CSSParserTokenRange range_copy = range;
  const CSSValue* align_content_value =
      ToLonghand(GetCSSPropertyAlignContent())
          .ParseSingleValue(range, context, local_context);
  if (!align_content_value)
    return false;

  if (range.AtEnd())
    range = range_copy;

  const CSSValue* justify_content_value =
      ToLonghand(GetCSSPropertyJustifyContent())
          .ParseSingleValue(range, context, local_context);
  if (!justify_content_value || !range.AtEnd())
    return false;

  DCHECK(align_content_value);
  DCHECK(justify_content_value);

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyAlignContent, CSSPropertyPlaceContent, *align_content_value,
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyJustifyContent, CSSPropertyPlaceContent,
      *justify_content_value, important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);

  return true;
}

const CSSValue* PlaceContent::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  // TODO (jfernandez): The spec states that we should return the specified
  // value.
  return ComputedStyleUtils::ValuesForShorthandProperty(
      placeContentShorthand(), style, layout_object, styled_node,
      allow_visited_style);
}

}  // namespace CSSShorthand
}  // namespace blink
