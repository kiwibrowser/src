// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_syntax_descriptor.h"

#include "third_party/blink/renderer/core/css/css_custom_property_declaration.h"
#include "third_party/blink/renderer/core/css/css_uri_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/css_variable_reference_value.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_idioms.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/parser/css_variable_parser.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"

namespace blink {

void ConsumeWhitespace(const String& string, size_t& offset) {
  while (IsHTMLSpace(string[offset]))
    offset++;
}

bool ConsumeCharacterAndWhitespace(const String& string,
                                   char character,
                                   size_t& offset) {
  if (string[offset] != character)
    return false;
  offset++;
  ConsumeWhitespace(string, offset);
  return true;
}

CSSSyntaxType ParseSyntaxType(String type) {
  // TODO(timloh): Are these supposed to be case sensitive?
  if (type == "length")
    return CSSSyntaxType::kLength;
  if (type == "number")
    return CSSSyntaxType::kNumber;
  if (type == "percentage")
    return CSSSyntaxType::kPercentage;
  if (type == "length-percentage")
    return CSSSyntaxType::kLengthPercentage;
  if (type == "color")
    return CSSSyntaxType::kColor;
  if (type == "image")
    return CSSSyntaxType::kImage;
  if (type == "url")
    return CSSSyntaxType::kUrl;
  if (type == "integer")
    return CSSSyntaxType::kInteger;
  if (type == "angle")
    return CSSSyntaxType::kAngle;
  if (type == "time")
    return CSSSyntaxType::kTime;
  if (type == "resolution")
    return CSSSyntaxType::kResolution;
  if (type == "transform-list")
    return CSSSyntaxType::kTransformList;
  if (type == "custom-ident")
    return CSSSyntaxType::kCustomIdent;
  // Not an Ident, just used to indicate failure
  return CSSSyntaxType::kIdent;
}

bool ConsumeSyntaxType(const String& input,
                       size_t& offset,
                       CSSSyntaxType& type) {
  DCHECK_EQ(input[offset], '<');
  offset++;
  size_t type_start = offset;
  while (offset < input.length() && input[offset] != '>')
    offset++;
  if (offset == input.length())
    return false;
  type = ParseSyntaxType(input.Substring(type_start, offset - type_start));
  if (type == CSSSyntaxType::kIdent)
    return false;
  offset++;
  return true;
}

bool ConsumeSyntaxIdent(const String& input, size_t& offset, String& ident) {
  size_t ident_start = offset;
  while (IsNameCodePoint(input[offset]))
    offset++;
  if (offset == ident_start)
    return false;
  ident = input.Substring(ident_start, offset - ident_start);
  return !CSSPropertyParserHelpers::IsCSSWideKeyword(ident);
}

CSSSyntaxDescriptor::CSSSyntaxDescriptor(const String& input) {
  size_t offset = 0;
  ConsumeWhitespace(input, offset);

  if (ConsumeCharacterAndWhitespace(input, '*', offset)) {
    if (offset != input.length())
      return;
    syntax_components_.push_back(
        CSSSyntaxComponent(CSSSyntaxType::kTokenStream, g_empty_string, false));
    return;
  }

  do {
    CSSSyntaxType type;
    String ident;
    bool success;

    if (input[offset] == '<') {
      success = ConsumeSyntaxType(input, offset, type);
    } else {
      type = CSSSyntaxType::kIdent;
      success = ConsumeSyntaxIdent(input, offset, ident);
    }

    if (!success) {
      syntax_components_.clear();
      return;
    }

    bool repeatable = ConsumeCharacterAndWhitespace(input, '+', offset);
    // <transform-list> is already a space separated list,
    // <transform-list>+ is invalid.
    if (type == CSSSyntaxType::kTransformList && repeatable) {
      syntax_components_.clear();
      return;
    }
    ConsumeWhitespace(input, offset);
    syntax_components_.push_back(CSSSyntaxComponent(type, ident, repeatable));

  } while (ConsumeCharacterAndWhitespace(input, '|', offset));

  if (offset != input.length())
    syntax_components_.clear();
}

const CSSValue* ConsumeSingleType(const CSSSyntaxComponent& syntax,
                                  CSSParserTokenRange& range,
                                  const CSSParserContext* context) {
  using namespace CSSPropertyParserHelpers;

  switch (syntax.type_) {
    case CSSSyntaxType::kIdent:
      if (range.Peek().GetType() == kIdentToken &&
          range.Peek().Value() == syntax.string_) {
        range.ConsumeIncludingWhitespace();
        return CSSCustomIdentValue::Create(AtomicString(syntax.string_));
      }
      return nullptr;
    case CSSSyntaxType::kLength:
      return ConsumeLength(range, kHTMLStandardMode,
                           ValueRange::kValueRangeAll);
    case CSSSyntaxType::kNumber:
      return ConsumeNumber(range, ValueRange::kValueRangeAll);
    case CSSSyntaxType::kPercentage:
      return ConsumePercent(range, ValueRange::kValueRangeAll);
    case CSSSyntaxType::kLengthPercentage:
      return ConsumeLengthOrPercent(range, kHTMLStandardMode,
                                    ValueRange::kValueRangeAll);
    case CSSSyntaxType::kColor:
      return ConsumeColor(range, kHTMLStandardMode);
    case CSSSyntaxType::kImage:
      return ConsumeImage(range, context);
    case CSSSyntaxType::kUrl:
      return ConsumeUrl(range, context);
    case CSSSyntaxType::kInteger:
      return ConsumeInteger(range);
    case CSSSyntaxType::kAngle:
      return ConsumeAngle(range, context, base::Optional<WebFeature>());
    case CSSSyntaxType::kTime:
      return ConsumeTime(range, ValueRange::kValueRangeAll);
    case CSSSyntaxType::kResolution:
      return ConsumeResolution(range);
    case CSSSyntaxType::kTransformList:
      return ConsumeTransformList(range, *context);
    case CSSSyntaxType::kCustomIdent:
      return ConsumeCustomIdent(range);
    default:
      NOTREACHED();
      return nullptr;
  }
}

const CSSValue* ConsumeSyntaxComponent(const CSSSyntaxComponent& syntax,
                                       CSSParserTokenRange range,
                                       const CSSParserContext* context) {
  // CSS-wide keywords are already handled by the CSSPropertyParser
  if (syntax.repeatable_) {
    CSSValueList* list = CSSValueList::CreateSpaceSeparated();
    while (!range.AtEnd()) {
      const CSSValue* value = ConsumeSingleType(syntax, range, context);
      if (!value)
        return nullptr;
      list->Append(*value);
    }
    return list;
  }
  const CSSValue* result = ConsumeSingleType(syntax, range, context);
  if (!range.AtEnd())
    return nullptr;
  return result;
}

const CSSValue* CSSSyntaxDescriptor::Parse(CSSParserTokenRange range,
                                           const CSSParserContext* context,
                                           bool is_animation_tainted) const {
  if (IsTokenStream()) {
    return CSSVariableParser::ParseRegisteredPropertyValue(
        range, *context, false, is_animation_tainted);
  }
  range.ConsumeWhitespace();
  for (const CSSSyntaxComponent& component : syntax_components_) {
    if (const CSSValue* result =
            ConsumeSyntaxComponent(component, range, context))
      return result;
  }
  return CSSVariableParser::ParseRegisteredPropertyValue(range, *context, true,
                                                         is_animation_tainted);
}

}  // namespace blink
