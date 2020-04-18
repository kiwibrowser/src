// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/perspective_origin.h"

#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* PerspectiveOrigin::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return ConsumePosition(range, context,
                         CSSPropertyParserHelpers::UnitlessQuirk::kForbid,
                         base::Optional<WebFeature>());
}

bool PerspectiveOrigin::IsLayoutDependent(const ComputedStyle* style,
                                          LayoutObject* layout_object) const {
  return layout_object && layout_object->IsBox();
}

const CSSValue* PerspectiveOrigin::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  if (layout_object) {
    LayoutRect box;
    if (layout_object->IsBox())
      box = ToLayoutBox(layout_object)->BorderBoxRect();

    return CSSValuePair::Create(
        ZoomAdjustedPixelValue(
            MinimumValueForLength(style.PerspectiveOriginX(), box.Width()),
            style),
        ZoomAdjustedPixelValue(
            MinimumValueForLength(style.PerspectiveOriginY(), box.Height()),
            style),
        CSSValuePair::kKeepIdenticalValues);
  } else {
    return CSSValuePair::Create(
        ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.PerspectiveOriginX(), style),
        ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
            style.PerspectiveOriginY(), style),
        CSSValuePair::kKeepIdenticalValues);
  }
}

}  // namespace CSSLonghand
}  // namespace blink
