// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/border_image.h"

#include "third_party/blink/renderer/core/css/css_initial_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/css/properties/longhand.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool BorderImage::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  CSSValue* source = nullptr;
  CSSValue* slice = nullptr;
  CSSValue* width = nullptr;
  CSSValue* outset = nullptr;
  CSSValue* repeat = nullptr;

  if (!CSSParsingUtils::ConsumeBorderImageComponents(
          range, context, source, slice, width, outset, repeat,
          CSSParsingUtils::DefaultFill::kNoFill)) {
    return false;
  }

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBorderImageSource, CSSPropertyBorderImage,
      source ? *source
             : *ToLonghand(&GetCSSPropertyBorderImageSource())->InitialValue(),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBorderImageSlice, CSSPropertyBorderImage,
      slice ? *slice
            : *ToLonghand(&GetCSSPropertyBorderImageSlice())->InitialValue(),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBorderImageWidth, CSSPropertyBorderImage,
      width ? *width
            : *ToLonghand(&GetCSSPropertyBorderImageWidth())->InitialValue(),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBorderImageOutset, CSSPropertyBorderImage,
      outset ? *outset
             : *ToLonghand(&GetCSSPropertyBorderImageOutset())->InitialValue(),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyBorderImageRepeat, CSSPropertyBorderImage,
      repeat ? *repeat
             : *ToLonghand(&GetCSSPropertyBorderImageRepeat())->InitialValue(),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);

  return true;
}

const CSSValue* BorderImage::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForNinePieceImage(style.BorderImage(), style);
}

}  // namespace CSSShorthand
}  // namespace blink
