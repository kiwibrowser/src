// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/grid_auto_columns.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* GridAutoColumns::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  return CSSParsingUtils::ConsumeGridTrackList(
      range, context.Mode(), CSSParsingUtils::TrackListType::kGridAuto);
}

// Specs mention that getComputedStyle() should return the used value of the
// property instead of the computed one for grid-template-{rows|columns} but
// not for the grid-auto-{rows|columns} as things like grid-auto-columns:
// 2fr; cannot be resolved to a value in pixels as the '2fr' means very
// different things depending on the size of the explicit grid or the number
// of implicit tracks added to the grid. See
// http://lists.w3.org/Archives/Public/www-style/2013Nov/0014.html
const CSSValue* GridAutoColumns::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForGridTrackSizeList(kForColumns, style);
}

}  // namespace CSSLonghand
}  // namespace blink
