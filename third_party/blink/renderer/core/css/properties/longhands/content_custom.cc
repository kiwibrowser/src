// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/properties/longhands/content.h"

#include "third_party/blink/renderer/core/css/css_counter_value.h"
#include "third_party/blink/renderer/core/css/css_function_value.h"
#include "third_party/blink/renderer/core/css/css_string_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"
#include "third_party/blink/renderer/core/css/properties/computed_style_utils.h"
#include "third_party/blink/renderer/core/css_value_keywords.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {
namespace {

CSSValue* ConsumeAttr(CSSParserTokenRange args,
                      const CSSParserContext& context) {
  if (args.Peek().GetType() != kIdentToken)
    return nullptr;

  AtomicString attr_name =
      args.ConsumeIncludingWhitespace().Value().ToAtomicString();
  if (!args.AtEnd())
    return nullptr;

  if (context.IsHTMLDocument())
    attr_name = attr_name.LowerASCII();

  CSSFunctionValue* attr_value = CSSFunctionValue::Create(CSSValueAttr);
  attr_value->Append(*CSSCustomIdentValue::Create(attr_name));
  return attr_value;
}

CSSValue* ConsumeCounterContent(CSSParserTokenRange args, bool counters) {
  CSSCustomIdentValue* identifier =
      CSSPropertyParserHelpers::ConsumeCustomIdent(args);
  if (!identifier)
    return nullptr;

  CSSStringValue* separator = nullptr;
  if (!counters) {
    separator = CSSStringValue::Create(String());
  } else {
    if (!CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(args) ||
        args.Peek().GetType() != kStringToken)
      return nullptr;
    separator = CSSStringValue::Create(
        args.ConsumeIncludingWhitespace().Value().ToString());
  }

  CSSIdentifierValue* list_style = nullptr;
  if (CSSPropertyParserHelpers::ConsumeCommaIncludingWhitespace(args)) {
    CSSValueID id = args.Peek().Id();
    if ((id != CSSValueNone &&
         (id < CSSValueDisc || id > CSSValueKatakanaIroha)))
      return nullptr;
    list_style = CSSPropertyParserHelpers::ConsumeIdent(args);
  } else {
    list_style = CSSIdentifierValue::Create(CSSValueDecimal);
  }

  if (!args.AtEnd())
    return nullptr;
  return cssvalue::CSSCounterValue::Create(identifier, list_style, separator);
}

}  // namespace
namespace CSSLonghand {

const CSSValue* Content::ParseSingleValue(CSSParserTokenRange& range,
                                          const CSSParserContext& context,
                                          const CSSParserLocalContext&) const {
  if (CSSPropertyParserHelpers::IdentMatches<CSSValueNone, CSSValueNormal>(
          range.Peek().Id()))
    return CSSPropertyParserHelpers::ConsumeIdent(range);

  CSSValueList* values = CSSValueList::CreateSpaceSeparated();

  do {
    CSSValue* parsed_value =
        CSSPropertyParserHelpers::ConsumeImage(range, &context);
    if (!parsed_value) {
      parsed_value = CSSPropertyParserHelpers::ConsumeIdent<
          CSSValueOpenQuote, CSSValueCloseQuote, CSSValueNoOpenQuote,
          CSSValueNoCloseQuote>(range);
    }
    if (!parsed_value)
      parsed_value = CSSPropertyParserHelpers::ConsumeString(range);
    if (!parsed_value) {
      if (range.Peek().FunctionId() == CSSValueAttr) {
        parsed_value = ConsumeAttr(
            CSSPropertyParserHelpers::ConsumeFunction(range), context);
      } else if (range.Peek().FunctionId() == CSSValueCounter) {
        parsed_value = ConsumeCounterContent(
            CSSPropertyParserHelpers::ConsumeFunction(range), false);
      } else if (range.Peek().FunctionId() == CSSValueCounters) {
        parsed_value = ConsumeCounterContent(
            CSSPropertyParserHelpers::ConsumeFunction(range), true);
      }
      if (!parsed_value)
        return nullptr;
    }
    values->Append(*parsed_value);
  } while (!range.AtEnd());

  return values;
}

const CSSValue* Content::CSSValueFromComputedStyleInternal(
    const ComputedStyle& style,
    const SVGComputedStyle&,
    const LayoutObject*,
    Node* styled_node,
    bool allow_visited_style) const {
  return ComputedStyleUtils::ValueForContentData(style);
}

}  // namespace CSSLonghand
}  // namespace blink
