// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/shorthands/font_variant.h"

#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/parser/font_variant_east_asian_parser.h"
#include "third_party/blink/renderer/core/css/parser/font_variant_ligatures_parser.h"
#include "third_party/blink/renderer/core/css/parser/font_variant_numeric_parser.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace CSSShorthand {

bool FontVariant::ParseShorthand(
    bool important,
    CSSParserTokenRange& range,
    const CSSParserContext&,
    const CSSParserLocalContext&,
    HeapVector<CSSPropertyValue, 256>& properties) const {
  if (CSSPropertyParserHelpers::IdentMatches<CSSValueNormal, CSSValueNone>(
          range.Peek().Id())) {
    CSSPropertyParserHelpers::AddProperty(
        CSSPropertyFontVariantLigatures, CSSPropertyFontVariant,
        *CSSPropertyParserHelpers::ConsumeIdent(range), important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
    CSSPropertyParserHelpers::AddProperty(
        CSSPropertyFontVariantCaps, CSSPropertyFontVariant,
        *CSSIdentifierValue::Create(CSSValueNormal), important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
    CSSPropertyParserHelpers::AddProperty(
        CSSPropertyFontVariantNumeric, CSSPropertyFontVariant,
        *CSSIdentifierValue::Create(CSSValueNormal), important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
    CSSPropertyParserHelpers::AddProperty(
        CSSPropertyFontVariantEastAsian, CSSPropertyFontVariant,
        *CSSIdentifierValue::Create(CSSValueNormal), important,
        CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
    return range.AtEnd();
  }

  CSSIdentifierValue* caps_value = nullptr;
  FontVariantLigaturesParser ligatures_parser;
  FontVariantNumericParser numeric_parser;
  FontVariantEastAsianParser east_asian_parser;
  do {
    FontVariantLigaturesParser::ParseResult ligatures_parse_result =
        ligatures_parser.ConsumeLigature(range);
    FontVariantNumericParser::ParseResult numeric_parse_result =
        numeric_parser.ConsumeNumeric(range);
    FontVariantEastAsianParser::ParseResult east_asian_parse_result =
        east_asian_parser.ConsumeEastAsian(range);
    if (ligatures_parse_result ==
            FontVariantLigaturesParser::ParseResult::kConsumedValue ||
        numeric_parse_result ==
            FontVariantNumericParser::ParseResult::kConsumedValue ||
        east_asian_parse_result ==
            FontVariantEastAsianParser::ParseResult::kConsumedValue)
      continue;

    if (ligatures_parse_result ==
            FontVariantLigaturesParser::ParseResult::kDisallowedValue ||
        numeric_parse_result ==
            FontVariantNumericParser::ParseResult::kDisallowedValue ||
        east_asian_parse_result ==
            FontVariantEastAsianParser::ParseResult::kDisallowedValue)
      return false;

    CSSValueID id = range.Peek().Id();
    switch (id) {
      case CSSValueSmallCaps:
      case CSSValueAllSmallCaps:
      case CSSValuePetiteCaps:
      case CSSValueAllPetiteCaps:
      case CSSValueUnicase:
      case CSSValueTitlingCaps:
        // Only one caps value permitted in font-variant grammar.
        if (caps_value)
          return false;
        caps_value = CSSPropertyParserHelpers::ConsumeIdent(range);
        break;
      default:
        return false;
    }
  } while (!range.AtEnd());

  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyFontVariantLigatures, CSSPropertyFontVariant,
      *ligatures_parser.FinalizeValue(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyFontVariantNumeric, CSSPropertyFontVariant,
      *numeric_parser.FinalizeValue(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyFontVariantEastAsian, CSSPropertyFontVariant,
      *east_asian_parser.FinalizeValue(), important,
      CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit, properties);
  CSSPropertyParserHelpers::AddProperty(
      CSSPropertyFontVariantCaps, CSSPropertyFontVariant,
      caps_value ? *caps_value : *CSSIdentifierValue::Create(CSSValueNormal),
      important, CSSPropertyParserHelpers::IsImplicitProperty::kNotImplicit,
      properties);
  return true;
}

const CSSValue* FontVariant::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject* layout_object,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValuesForFontVariantProperty(
      style, layout_object, styled_node, allow_visited_style);
}

}  // namespace CSSShorthand
}  // namespace blink
