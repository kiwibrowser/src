// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/display.h"

#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_layout_function_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSLonghand {

const CSSValue* Display::ParseSingleValue(CSSParserTokenRange& range,
                                          const CSSParserContext& context,
                                          const CSSParserLocalContext&) const {
  // NOTE: All the keyword values for the display property are handled by the
  // CSSParserFastPaths.
  if (!RuntimeEnabledFeatures::CSSLayoutAPIEnabled())
    return nullptr;

  if (!context.IsSecureContext())
    return nullptr;

  CSSValueID function = range.Peek().FunctionId();
  if (function != CSSValueLayout && function != CSSValueInlineLayout)
    return nullptr;

  CSSParserTokenRange range_copy = range;
  CSSParserTokenRange args =
      CSSPropertyParserHelpers::ConsumeFunction(range_copy);
  CSSCustomIdentValue* name =
      CSSPropertyParserHelpers::ConsumeCustomIdent(args);

  // If we didn't get a custom-ident or didn't exhaust the function arguments
  // return nothing.
  if (!name || !args.AtEnd())
    return nullptr;

  range = range_copy;
  return cssvalue::CSSLayoutFunctionValue::Create(
      name, /* is_inline */ function == CSSValueInlineLayout);
}

const CSSValue* Display::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node*,
    bool allow_visited_style) const {
  if (style.IsDisplayLayoutCustomBox()) {
    return cssvalue::CSSLayoutFunctionValue::Create(
        CSSCustomIdentValue::Create(style.DisplayLayoutCustomName()),
        style.IsDisplayInlineType());
  }

  return CSSIdentifierValue::Create(style.Display());
}

}  // namespace CSSLonghand
}  // namespace blink
