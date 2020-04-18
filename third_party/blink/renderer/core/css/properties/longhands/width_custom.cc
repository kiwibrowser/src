// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/width.h"

#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Width::ParseSingleValue(CSSParserTokenRange& range,
                                        const CSSParserContext& context,
                                        const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeWidthOrHeight(
      range, context, CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
}

bool Width::IsLayoutDependent(const ComputedStyle* style,
                              LayoutObject* layout_object) const {
  return layout_object && layout_object->IsBox();
}

const CSSValue* Width::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  if (ComputedStyleUtils::WidthOrHeightShouldReturnUsedValue(layout_object)) {
    return ZoomAdjustedPixelValue(
        ComputedStyleUtils::SizingBox(*layout_object).Width(), style);
  }
  return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(style.Width(),
                                                             style);
}

}  // namespace CSSLonghand
}  // namespace blink
