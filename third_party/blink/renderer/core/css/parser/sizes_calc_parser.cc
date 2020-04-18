// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/sizes_calc_parser.h"

#include "third_party/blink/renderer/core/css/media_values.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token.h"

namespace blink {

SizesCalcParser::SizesCalcParser(CSSParserTokenRange range,
                                 MediaValues* media_values)
    : media_values_(media_values), result_(0) {
  is_valid_ = CalcToReversePolishNotation(range) && Calculate();
}

float SizesCalcParser::Result() const {
  DCHECK(is_valid_);
  return result_;
}

static bool OperatorPriority(UChar cc, bool& high_priority) {
  if (cc == '+' || cc == '-')
    high_priority = false;
  else if (cc == '*' || cc == '/')
    high_priority = true;
  else
    return false;
  return true;
}

bool SizesCalcParser::HandleOperator(Vector<CSSParserToken>& stack,
                                     const CSSParserToken& token) {
  // If the token is not an operator, then return. Else determine the
  // precedence of the new operator (op1).
  bool incoming_operator_priority;
  if (!OperatorPriority(token.Delimiter(), incoming_operator_priority))
    return false;

  while (!stack.IsEmpty()) {
    // While there is an operator (op2) at the top of the stack,
    // determine its precedence, and...
    const CSSParserToken& top_of_stack = stack.back();
    if (top_of_stack.GetType() != kDelimiterToken)
      break;
    bool stack_operator_priority;
    if (!OperatorPriority(top_of_stack.Delimiter(), stack_operator_priority))
      return false;
    // ...if op1 is left-associative (all currently supported
    // operators are) and its precedence is equal to that of op2, or
    // op1 has precedence less than that of op2, ...
    if (incoming_operator_priority && !stack_operator_priority)
      break;
    // ...pop op2 off the stack and add it to the output queue.
    AppendOperator(top_of_stack);
    stack.pop_back();
  }
  // Push op1 onto the stack.
  stack.push_back(token);
  return true;
}

void SizesCalcParser::AppendNumber(const CSSParserToken& token) {
  SizesCalcValue value;
  value.value = token.NumericValue();
  value_list_.push_back(value);
}

bool SizesCalcParser::AppendLength(const CSSParserToken& token) {
  SizesCalcValue value;
  double result = 0;
  if (!media_values_->ComputeLength(token.NumericValue(), token.GetUnitType(),
                                    result))
    return false;
  value.value = result;
  value.is_length = true;
  value_list_.push_back(value);
  return true;
}

void SizesCalcParser::AppendOperator(const CSSParserToken& token) {
  SizesCalcValue value;
  value.operation = token.Delimiter();
  value_list_.push_back(value);
}

bool SizesCalcParser::CalcToReversePolishNotation(CSSParserTokenRange range) {
  // This method implements the shunting yard algorithm, to turn the calc syntax
  // into a reverse polish notation.
  // http://en.wikipedia.org/wiki/Shunting-yard_algorithm

  Vector<CSSParserToken> stack;
  while (!range.AtEnd()) {
    const CSSParserToken& token = range.Consume();
    switch (token.GetType()) {
      case kNumberToken:
        AppendNumber(token);
        break;
      case kDimensionToken:
        if (!CSSPrimitiveValue::IsLength(token.GetUnitType()) ||
            !AppendLength(token))
          return false;
        break;
      case kDelimiterToken:
        if (!HandleOperator(stack, token))
          return false;
        break;
      case kFunctionToken:
        if (!EqualIgnoringASCIICase(token.Value(), "calc"))
          return false;
        // "calc(" is the same as "("
        FALLTHROUGH;
      case kLeftParenthesisToken:
        // If the token is a left parenthesis, then push it onto the stack.
        stack.push_back(token);
        break;
      case kRightParenthesisToken:
        // If the token is a right parenthesis:
        // Until the token at the top of the stack is a left parenthesis, pop
        // operators off the stack onto the output queue.
        while (!stack.IsEmpty() &&
               stack.back().GetType() != kLeftParenthesisToken &&
               stack.back().GetType() != kFunctionToken) {
          AppendOperator(stack.back());
          stack.pop_back();
        }
        // If the stack runs out without finding a left parenthesis, then there
        // are mismatched parentheses.
        if (stack.IsEmpty())
          return false;
        // Pop the left parenthesis from the stack, but not onto the output
        // queue.
        stack.pop_back();
        break;
      case kWhitespaceToken:
      case kEOFToken:
        break;
      case kCommentToken:
        NOTREACHED();
        FALLTHROUGH;
      case kCDOToken:
      case kCDCToken:
      case kAtKeywordToken:
      case kHashToken:
      case kUrlToken:
      case kBadUrlToken:
      case kPercentageToken:
      case kIncludeMatchToken:
      case kDashMatchToken:
      case kPrefixMatchToken:
      case kSuffixMatchToken:
      case kSubstringMatchToken:
      case kColumnToken:
      case kUnicodeRangeToken:
      case kIdentToken:
      case kCommaToken:
      case kColonToken:
      case kSemicolonToken:
      case kLeftBraceToken:
      case kLeftBracketToken:
      case kRightBraceToken:
      case kRightBracketToken:
      case kStringToken:
      case kBadStringToken:
        return false;
    }
  }

  // When there are no more tokens to read:
  // While there are still operator tokens in the stack:
  while (!stack.IsEmpty()) {
    // If the operator token on the top of the stack is a parenthesis, then
    // there are unclosed parentheses.
    CSSParserTokenType type = stack.back().GetType();
    if (type != kLeftParenthesisToken && type != kFunctionToken) {
      // Pop the operator onto the output queue.
      AppendOperator(stack.back());
    }
    stack.pop_back();
  }
  return true;
}

static bool OperateOnStack(Vector<SizesCalcValue>& stack, UChar operation) {
  if (stack.size() < 2)
    return false;
  SizesCalcValue right_operand = stack.back();
  stack.pop_back();
  SizesCalcValue left_operand = stack.back();
  stack.pop_back();
  bool is_length;
  switch (operation) {
    case '+':
      if (right_operand.is_length != left_operand.is_length)
        return false;
      is_length = (right_operand.is_length && left_operand.is_length);
      stack.push_back(
          SizesCalcValue(left_operand.value + right_operand.value, is_length));
      break;
    case '-':
      if (right_operand.is_length != left_operand.is_length)
        return false;
      is_length = (right_operand.is_length && left_operand.is_length);
      stack.push_back(
          SizesCalcValue(left_operand.value - right_operand.value, is_length));
      break;
    case '*':
      if (right_operand.is_length && left_operand.is_length)
        return false;
      is_length = (right_operand.is_length || left_operand.is_length);
      stack.push_back(
          SizesCalcValue(left_operand.value * right_operand.value, is_length));
      break;
    case '/':
      if (right_operand.is_length || right_operand.value == 0)
        return false;
      stack.push_back(SizesCalcValue(left_operand.value / right_operand.value,
                                     left_operand.is_length));
      break;
    default:
      return false;
  }
  return true;
}

bool SizesCalcParser::Calculate() {
  Vector<SizesCalcValue> stack;
  for (const auto& value : value_list_) {
    if (value.operation == 0) {
      stack.push_back(value);
    } else {
      if (!OperateOnStack(stack, value.operation))
        return false;
    }
  }
  if (stack.size() == 1 && stack.back().is_length) {
    result_ = std::max(clampTo<float>(stack.back().value), (float)0.0);
    return true;
  }
  return false;
}

}  // namespace blink
