// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/object_position.h"

#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* ObjectPosition::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return ConsumePosition(range, context,
                         CSSPropertyParserHelpers::UnitlessQuirk::kForbid,
                         base::Optional<WebFeature>());
}

const CSSValue* ObjectPosition::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return CSSValuePair::Create(
      ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          style.ObjectPosition().X(), style),
      ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          style.ObjectPosition().Y(), style),
      CSSValuePair::kKeepIdenticalValues);
}

}  // namespace CSSLonghand
}  // namespace blink
