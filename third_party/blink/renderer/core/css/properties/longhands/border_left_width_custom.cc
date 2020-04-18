// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/border_left_width.h"

#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/css/zoom_adjusted_pixel_value.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* BorderLeftWidth::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext& local_context) const {
  return CSSParsingUtils::ParseBorderWidthSide(range, context, local_context);
}

const CSSValue* BorderLeftWidth::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ZoomAdjustedPixelValue(style.BorderLeftWidth(), style);
}

}  // namespace CSSLonghand
}  // namespace blink
