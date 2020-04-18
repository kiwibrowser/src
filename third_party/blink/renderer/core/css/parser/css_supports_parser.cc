// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/css_supports_parser.h"

#include "third_party/blink/renderer/core/css/parser/css_parser_impl.h"

namespace blink {

CSSSupportsParser::SupportsResult CSSSupportsParser::SupportsCondition(
    CSSParserTokenRange range,
    CSSParserImpl& parser,
    SupportsParsingMode parsing_mode) {
  range.ConsumeWhitespace();
  CSSSupportsParser supports_parser(parser);
  SupportsResult result = supports_parser.ConsumeCondition(range);
  if (parsing_mode != kForWindowCSS || result != kInvalid)
    return result;
  // window.CSS.supports requires to parse as-if it was wrapped in parenthesis.
  // The only wrapped production that wouldn't have parsed above is the
  // declaration condition production.
  return supports_parser.ConsumeDeclarationCondition(range);
}

enum ClauseType { kUnresolved, kConjunction, kDisjunction };

CSSSupportsParser::SupportsResult CSSSupportsParser::ConsumeCondition(
    CSSParserTokenRange range) {
  if (range.Peek().GetType() == kIdentToken)
    return ConsumeNegation(range);

  bool result;
  ClauseType clause_type = kUnresolved;

  while (true) {
    SupportsResult next_result = ConsumeConditionInParenthesis(range);
    if (next_result == kInvalid)
      return kInvalid;
    bool next_supported = next_result;
    if (clause_type == kUnresolved)
      result = next_supported;
    else if (clause_type == kConjunction)
      result &= next_supported;
    else
      result |= next_supported;

    if (range.AtEnd())
      break;
    if (range.ConsumeIncludingWhitespace().GetType() != kWhitespaceToken)
      return kInvalid;
    if (range.AtEnd())
      break;

    const CSSParserToken& token = range.Consume();
    if (token.GetType() != kIdentToken)
      return kInvalid;
    if (clause_type == kUnresolved)
      clause_type = token.Value().length() == 3 ? kConjunction : kDisjunction;
    if ((clause_type == kConjunction &&
         !EqualIgnoringASCIICase(token.Value(), "and")) ||
        (clause_type == kDisjunction &&
         !EqualIgnoringASCIICase(token.Value(), "or")))
      return kInvalid;

    if (range.ConsumeIncludingWhitespace().GetType() != kWhitespaceToken)
      return kInvalid;
  }
  return result ? kSupported : kUnsupported;
}

CSSSupportsParser::SupportsResult CSSSupportsParser::ConsumeNegation(
    CSSParserTokenRange range) {
  DCHECK_EQ(range.Peek().GetType(), kIdentToken);
  if (!EqualIgnoringASCIICase(range.Consume().Value(), "not"))
    return kInvalid;
  if (range.ConsumeIncludingWhitespace().GetType() != kWhitespaceToken)
    return kInvalid;
  SupportsResult result = ConsumeConditionInParenthesis(range);
  range.ConsumeWhitespace();
  if (!range.AtEnd() || result == kInvalid)
    return kInvalid;
  return result ? kUnsupported : kSupported;
}

CSSSupportsParser::SupportsResult
CSSSupportsParser::ConsumeConditionInParenthesis(CSSParserTokenRange& range) {
  if (range.Peek().GetType() == kFunctionToken) {
    range.ConsumeComponentValue();
    return kUnsupported;
  }
  if (range.Peek().GetType() != kLeftParenthesisToken)
    return kInvalid;
  CSSParserTokenRange inner_range = range.ConsumeBlock();
  inner_range.ConsumeWhitespace();
  SupportsResult result = ConsumeCondition(inner_range);
  if (result != kInvalid)
    return result;
  return ConsumeDeclarationCondition(inner_range);
}

CSSSupportsParser::SupportsResult
CSSSupportsParser::ConsumeDeclarationCondition(CSSParserTokenRange& range) {
  if (range.Peek().GetType() != kIdentToken)
    return kUnsupported;
  return parser_.SupportsDeclaration(range) ? kSupported : kUnsupported;
}

}  // namespace blink
