// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/clip.h"

#include "third_party/blink/renderer/core/css/css_quad_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace {

CSSValue* ConsumeClipComponent(CSSParserTokenRange& range,
                               CSSParserMode css_parser_mode) {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);
  return CSSPropertyParserHelpers::ConsumeLength(
      range, css_parser_mode, kValueRangeAll,
      CSSPropertyParserHelpers::UnitlessQuirk::kAllow);
}

}  // namespace
namespace CSSLonghand {

const CSSValue* Clip::ParseSingleValue(CSSParserTokenRange& range,
                                       const CSSParserContext& context,
                                       const CSSParserLocalContext&) const {
  if (range.Peek().Id() == CSSValueAuto)
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  if (range.Peek().FunctionId() != CSSValueRect)
    return nullptr;

  CSSParserTokenRange args = CSSPropertyParserHelpers::ConsumeFunction(range);
  // rect(t, r, b, l) || rect(t r b l)
  CSSValue* top = ConsumeClipComponent(args, context.Mode());
  if (!top)
    return nullptr;
  bool needs_comma =
      CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(args);
  CSSValue* right = ConsumeClipComponent(args, context.Mode());
  if (!right ||
      (needs_comma &&
       !CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(args)))
    return nullptr;
  CSSValue* bottom = ConsumeClipComponent(args, context.Mode());
  if (!bottom ||
      (needs_comma &&
       !CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(args)))
    return nullptr;
  CSSValue* left = ConsumeClipComponent(args, context.Mode());
  if (!left || !args.AtEnd())
    return nullptr;
  return CSSQuadValue::Create(top, right, bottom, left,
                              CSSQuadValue::kSerializeAsRect);
}

const CSSValue* Clip::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  if (style.HasAutoClip())
    return CSSIdentifierValue::Create(CSSValueAuto);
  CSSValue* top = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
      style.Clip().Top(), style);
  CSSValue* right = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
      style.Clip().Right(), style);
  CSSValue* bottom = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
      style.Clip().Bottom(), style);
  CSSValue* left = ComputedStyleUtils::ZoomAdjustedPixelValueOrAuto(
      style.Clip().Left(), style);
  return CSSQuadValue::Create(top, right, bottom, left,
                              CSSQuadValue::kSerializeAsRect);
}

}  // namespace CSSLonghand
}  // namespace blink
