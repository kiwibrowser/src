// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/border_top_color.h"

#include "third_party/blink/renderer/core/css/css_color_value.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

class CSSParserContext;
class CSSParserLocalContext;
class CSSParserTokenRange;

namespace CSSLonghand {

const CSSValue* BorderTopColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return CSSParsingUtils::ConsumeBorderColorSide(range, context, local_context);
}

const blink::Color BorderTopColor::ColorIncludingFallback(
    bool visited_link,
    const ComputedStyle& style) const {
  StyleColor result =
      visited_link ? style.VisitedLinkBorderTopColor() : style.BorderTopColor();
  EBorderStyle border_style = style.BorderTopStyle();
  return ComputedStyleUtils::BorderSideColor(style, result, border_style,
                                             visited_link);
}

const CSSValue* BorderTopColor::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return allow_visited_style
             ? cssvalue::CSSColorValue::Create(
                   style.VisitedDependentColor(*this).Rgb())
             : ComputedStyleUtils::ComputedStyleUtils::CurrentColorOrValidColor(
                   style, style.BorderTopColor());
}

}  // namespace CSSLonghand
}  // namespace blink
