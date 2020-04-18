// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/baseline_shift.h"

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* BaselineShift::ParseSingleValue(
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&) const {
  CSSValueID id = range.Peek().Id();
  if (id == CSSValueBaseline || id == CSSValueSub || id == CSSValueSuper)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return CSSPropertyParserHelpers::ConsumeLengthOrPercent(
      range, kSVGAttributeMode, kValueRangeAll);
}

const CSSValue* BaselineShift::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle& svg_style,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  switch (svg_style.BaselineShift()) {
    case BS_SUPER:
      return CSSIdentifierValue::Create(CSSValueSuper);
    case BS_SUB:
      return CSSIdentifierValue::Create(CSSValueSub);
    case BS_LENGTH:
      return ComputedStyleUtils::ZoomAdjustedPixelValueForLength(
          svg_style.BaselineShiftValue(), style);
  }
  NOTREACHED();
  return nullptr;
}

}  // namespace CSSLonghand
}  // namespace blink
