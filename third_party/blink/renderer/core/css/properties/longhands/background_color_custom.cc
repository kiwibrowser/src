// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/background_color.h"

#include "third_party/blink/renderer/core/css/css_color_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_mode.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* BackgroundColor::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSPropertyParserHelpers::ConsumeColor(
      range, context.Mode(), IsQuirksModeBehavior(context.Mode()));
}

const blink::Color BackgroundColor::ColorIncludingFallback(
    bool visited_link,
    const ComputedStyle& style) const {
  StyleColor style_color = visited_link ? style.VisitedLinkBackgroundColor()
                                        : style.BackgroundColor();
  blink::Color result =
      visited_link ? style.VisitedLinkColor() : style.GetColor();
  if (!style_color.IsCurrentColor())
    result = style_color.GetColor();

  // FIXME: Technically someone could explicitly specify the color transparent,
  // but for now we'll just assume that if the background color is transparent
  // that it wasn't set. Note that it's weird that we're returning unvisited
  // info for a visited link, but given our restriction that the alpha values
  // have to match, it makes more sense to return the unvisited background color
  // if specified than it does to return black. This behavior matches what
  // Firefox 4 does as well.
  if (visited_link && result == blink::Color::kTransparent)
    return ColorIncludingFallback(false, style);

  return result;
}

const CSSValue* BackgroundColor::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return allow_visited_style ? cssvalue::CSSColorValue::Create(
                                   style.VisitedDependentColor(*this).Rgb())
                             : ComputedStyleUtils::CurrentColorOrValidColor(
                                   style, style.BackgroundColor());
}

}  // namespace CSSLonghand
}  // namespace blink
