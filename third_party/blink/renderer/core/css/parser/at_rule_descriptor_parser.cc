// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/at_rule_descriptor_parser.h"

#include "third_party/blink/renderer/core/css/css_font_face_src_value.h"
#include "third_party/blink/renderer/core/css/css_unicode_range_value.h"
#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_mode.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"
#include "third_party/blink/renderer/core/css/parser/css_tokenizer.h"
#include "third_party/blink/renderer/core/css/properties/css_parsing_utils.h"
#include "third_party/blink/renderer/core/css/properties/css_property.h"

namespace blink {

namespace {

CSSValue* ConsumeFontVariantList(CSSParserTokenRange& range) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();
  do {
    if (range.Peek().Id() == CSSValueAll) {
      // FIXME: CSSPropertyParser::ParseFontVariant() implements
      // the old css3 draft:
      // http://www.w3.org/TR/2002/WD-css3-webfonts-20020802/#font-variant
      // 'all' is only allowed in @font-face and with no other values.
      if (values->length())
        return nullptr;
      return CSSPropertyParserHelpers::ConsumeIdent(range);
    }
    CSSIdentifierValue* font_variant =
        CSSParsingUtils::ConsumeFontVariantCSS21(range);
    if (font_variant)
      values->Append(*font_variant);
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));

  if (values->length())
    return values;

  return nullptr;
}

CSSIdentifierValue* ConsumeFontDisplay(CSSParserTokenRange& range) {
  return CSSPropertyParserHelpers::ConsumeIdent<CSSValueAuto, CSSValueBlock,
                                                CSSValueSwap, CSSValueFallback,
                                                CSSValueOptional>(range);
}

CSSValueList* ConsumeFontFaceUnicodeRange(CSSParserTokenRange& range) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();

  do {
    const CSSParserToken& token = range.ConsumeIncludingWhitespace();
    if (token.GetType() != kUnicodeRangeToken)
      return nullptr;

    UChar32 start = token.UnicodeRangeStart();
    UChar32 end = token.UnicodeRangeEnd();
    if (start > end)
      return nullptr;
    values->Append(*CSSUnicodeRangeValue::Create(start, end));
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));

  return values;
}

CSSValue* ConsumeFontFaceSrcURI(CSSParserTokenRange& range,
                                const CSSParserContext& context) {
  String url =
      CSSPropertyParserHelpers::ConsumeUrlAsStringView(range).ToString();
  if (url.IsNull())
    return nullptr;
  CSSFontFaceSrcValue* uri_value(CSSFontFaceSrcValue::Create(
      url, context.CompleteURL(url), context.GetReferrer(),
      context.ShouldCheckContentSecurityPolicy()));

  if (range.Peek().FunctionId() != CSSValueFormat)
    return uri_value;

  // FIXME: https://drafts.csswg.org/css-fonts says that format() contains a
  // comma-separated list of strings, but CSSFontFaceSrcValue stores only one
  // format. Allowing one format for now.
  CSSParserTokenRange args = CSSPropertyParserHelpers::ConsumeFunction(range);
  const CSSParserToken& arg = args.ConsumeIncludingWhitespace();
  if ((arg.GetType() != kStringToken) || !args.AtEnd())
    return nullptr;
  uri_value->SetFormat(arg.Value().ToString());
  return uri_value;
}

CSSValue* ConsumeFontFaceSrcLocal(CSSParserTokenRange& range,
                                  const CSSParserContext& context) {
  CSSParserTokenRange args = CSSPropertyParserHelpers::ConsumeFunction(range);
  ContentSecurityPolicyDisposition should_check_content_security_policy =
      context.ShouldCheckContentSecurityPolicy();
  if (args.Peek().GetType() == kStringToken) {
    const CSSParserToken& arg = args.ConsumeIncludingWhitespace();
    if (!args.AtEnd())
      return nullptr;
    return CSSFontFaceSrcValue::CreateLocal(
        arg.Value().ToString(), should_check_content_security_policy);
  }
  if (args.Peek().GetType() == kIdentToken) {
    String family_name = CSSParsingUtils::ConcatenateFamilyName(args);
    if (!args.AtEnd())
      return nullptr;
    return CSSFontFaceSrcValue::CreateLocal(
        family_name, should_check_content_security_policy);
  }
  return nullptr;
}

CSSValueList* ConsumeFontFaceSrc(CSSParserTokenRange& range,
                                 const CSSParserContext& context) {
  CSSValueList* values = CSSValueList::CreateCommaSeparated();

  range.ConsumeWhitespace();
  do {
    const CSSParserToken& token = range.Peek();
    CSSValue* parsed_value = nullptr;
    if (token.FunctionId() == CSSValueLocal)
      parsed_value = ConsumeFontFaceSrcLocal(range, context);
    else
      parsed_value = ConsumeFontFaceSrcURI(range, context);
    if (!parsed_value)
      return nullptr;
    values->Append(*parsed_value);
  } while (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(range));
  return values;
}

}  // namespace

CSSValue* AtRuleDescriptorParser::ParseFontFaceDescriptor(
    AtRuleDescriptorID id,
    CSSParserTokenRange& range,
    const CSSParserContext& context) {
  CSSValue* parsed_value = nullptr;
  range.ConsumeWhitespace();
  switch (id) {
    case AtRuleDescriptorID::FontFamily:
      if (CSSParsingUtils::ConsumeGenericFamily(range))
        return nullptr;
      parsed_value = CSSParsingUtils::ConsumeFamilyName(range);
      break;
    case AtRuleDescriptorID::Src:  // This is a list of urls or local
                                   // references.
      parsed_value = ConsumeFontFaceSrc(range, context);
      break;
    case AtRuleDescriptorID::UnicodeRange:
      parsed_value = ConsumeFontFaceUnicodeRange(range);
      break;
    case AtRuleDescriptorID::FontDisplay:
      parsed_value = ConsumeFontDisplay(range);
      break;
    case AtRuleDescriptorID::FontStretch:
      parsed_value =
          CSSParsingUtils::ConsumeFontStretch(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontStyle:
      parsed_value =
          CSSParsingUtils::ConsumeFontStyle(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontVariant:
      parsed_value = ConsumeFontVariantList(range);
      break;
    case AtRuleDescriptorID::FontWeight:
      parsed_value =
          CSSParsingUtils::ConsumeFontWeight(range, kCSSFontFaceRuleMode);
      break;
    case AtRuleDescriptorID::FontFeatureSettings:
      parsed_value = CSSParsingUtils::ConsumeFontFeatureSettings(range);
      break;
    default:
      break;
  }

  if (!parsed_value || !range.AtEnd())
    return nullptr;

  return parsed_value;
}

CSSValue* AtRuleDescriptorParser::ParseFontFaceDescriptor(
    AtRuleDescriptorID id,
    const String& string,
    const CSSParserContext& context) {
  CSSTokenizer tokenizer(string);
  Vector<CSSParserToken, 32> tokens = tokenizer.TokenizeToEOF();
  CSSParserTokenRange range = CSSParserTokenRange(tokens);
  return ParseFontFaceDescriptor(id, range, context);
}

CSSValue* AtRuleDescriptorParser::ParseFontFaceDeclaration(
    CSSParserTokenRange& range,
    const CSSParserContext& context) {
  DCHECK_EQ(range.Peek().GetType(), kIdentToken);
  const CSSParserToken& token = range.ConsumeIncludingWhitespace();
  AtRuleDescriptorID id = token.ParseAsAtRuleDescriptorID();

  if (range.Consume().GetType() != kColonToken)
    return nullptr;  // Parse error

  return ParseFontFaceDescriptor(id, range, context);
}

bool AtRuleDescriptorParser::ParseAtRule(
    AtRuleDescriptorID id,
    CSSParserTokenRange& range,
    const CSSParserContext& context,
    HeapVector<CSSPropertyValue, 256>& parsed_descriptors) {
  // TODO(meade): Handle other descriptor types here.
  CSSValue* result =
      AtRuleDescriptorParser::ParseFontFaceDescriptor(id, range, context);
  if (!result)
    return false;
  // Convert to CSSPropertyID for legacy compatibility,
  // TODO(crbug.com/752745): Refactor CSSParserImpl to avoid using
  // the CSSPropertyID.
  CSSPropertyID equivalent_property_id = AtRuleDescriptorIDAsCSSPropertyID(id);
  parsed_descriptors.push_back(
      CSSPropertyValue(CSSProperty::Get(equivalent_property_id), *result));
  return true;
}

}  // namespace blink
