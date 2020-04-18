// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_LIGATURES_PARSER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_LIGATURES_PARSER_H_

#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"

namespace blink {

class FontVariantLigaturesParser {
  STACK_ALLOCATED();

 public:
  FontVariantLigaturesParser()
      : saw_common_ligatures_value_(false),
        saw_discretionary_ligatures_value_(false),
        saw_historical_ligatures_value_(false),
        saw_contextual_ligatures_value_(false),
        result_(CSSValueList::CreateSpaceSeparated()) {}

  enum class ParseResult { kConsumedValue, kDisallowedValue, kUnknownValue };

  ParseResult ConsumeLigature(CSSParserTokenRange& range) {
    CSSValueID value_id = range.Peek().Id();
    switch (value_id) {
      case CSSValueNoCommonLigatures:
      case CSSValueCommonLigatures:
        if (saw_common_ligatures_value_)
          return ParseResult::kDisallowedValue;
        saw_common_ligatures_value_ = true;
        break;
      case CSSValueNoDiscretionaryLigatures:
      case CSSValueDiscretionaryLigatures:
        if (saw_discretionary_ligatures_value_)
          return ParseResult::kDisallowedValue;
        saw_discretionary_ligatures_value_ = true;
        break;
      case CSSValueNoHistoricalLigatures:
      case CSSValueHistoricalLigatures:
        if (saw_historical_ligatures_value_)
          return ParseResult::kDisallowedValue;
        saw_historical_ligatures_value_ = true;
        break;
      case CSSValueNoContextual:
      case CSSValueContextual:
        if (saw_contextual_ligatures_value_)
          return ParseResult::kDisallowedValue;
        saw_contextual_ligatures_value_ = true;
        break;
      default:
        return ParseResult::kUnknownValue;
    }
    result_->Append(*CSSPropertyParserHelpers::ConsumeIdent(range));
    return ParseResult::kConsumedValue;
  }

  CSSValue* FinalizeValue() {
    if (!result_->length())
      return CSSIdentifierValue::Create(CSSValueNormal);
    return result_.Release();
  }

 private:
  bool saw_common_ligatures_value_;
  bool saw_discretionary_ligatures_value_;
  bool saw_historical_ligatures_value_;
  bool saw_contextual_ligatures_value_;
  Member<CSSValueList> result_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_LIGATURES_PARSER_H_
