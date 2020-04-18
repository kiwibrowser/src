// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/scroll_snap_type.h"

#include "third_party/blink/renderer/core/css/css_value_pair.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* ScrollSnapType::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueID axis_id = range.Peek().Id();
  if (axis_id != CSSValueNone && axis_id != CSSValueX && axis_id != CSSValueY &&
      axis_id != CSSValueBlock && axis_id != CSSValueInline &&
      axis_id != CSSValueBoth)
    return nullptr;
  CSSValue* axis_value = CSSPropertyParserHelpers::ConsumeIdent(range);
  if (range.AtEnd() || axis_id == CSSValueNone)
    return axis_value;

  CSSValueID strictness_id = range.Peek().Id();
  if (strictness_id != CSSValueProximity && strictness_id != CSSValueMandatory)
    return axis_value;
  CSSValue* strictness_value = CSSPropertyParserHelpers::ConsumeIdent(range);
  CSSValuePair* pair = CSSValuePair::Create(axis_value, strictness_value,
                                            CSSValuePair::kDropIdenticalValues);
  return pair;
}

const CSSValue* ScrollSnapType::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForScrollSnapType(style.GetScrollSnapType(),
                                                    style);
}

}  // namespace CSSLonghand
}  // namespace blink
