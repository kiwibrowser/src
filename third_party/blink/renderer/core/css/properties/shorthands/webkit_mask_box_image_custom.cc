// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/webkit_mask_box_image.h"

#include "third_party/blink/renderer/core/css/css_initial_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool WebkitMaskBoxImage::ParseShorthand(
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
          CSSParsingUtils::DefaultFill::kFill)) {
    return false;
  }

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskBoxImageSource, CSSPropertyWebkitMaskBoxImage,
      source ? *source : *CSSInitialValue::Create(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskBoxImageSlice, CSSPropertyWebkitMaskBoxImage,
      slice ? *slice : *CSSInitialValue::Create(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskBoxImageWidth, CSSPropertyWebkitMaskBoxImage,
      width ? *width : *CSSInitialValue::Create(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskBoxImageOutset, CSSPropertyWebkitMaskBoxImage,
      outset ? *outset : *CSSInitialValue::Create(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyWebkitMaskBoxImageRepeat, CSSPropertyWebkitMaskBoxImage,
      repeat ? *repeat : *CSSInitialValue::Create(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);

  return true;
}

const CSSValue* WebkitMaskBoxImage::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForNinePieceImage(style.MaskBoxImage(),
                                                    style);
}

}  // namespace CSSShorthand
}  // namespace blink
