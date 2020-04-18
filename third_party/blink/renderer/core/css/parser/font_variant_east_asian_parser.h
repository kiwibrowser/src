// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_EAST_ASIAN_PARSER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_EAST_ASIAN_PARSER_H_

#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_token_range.h"
#include "third_party/blink/renderer/core/css/parser/css_property_parser_helpers.h"

namespace blink {

class FontVariantEastAsianParser {
  STACK_ALLOCATED();

 public:
  FontVariantEastAsianParser()
      : saw_east_asian_form_value_(false),
        saw_east_asian_width_value_(false),
        saw_ruby_value_(false),
        result_(CSSValueList::CreateSpaceSeparated()) {}

  enum class ParseResult { kConsumedValue, kDisallowedValue, kUnknownValue };

  ParseResult ConsumeEastAsian(CSSParserTokenRange& range) {
    CSSValueID value_id = range.Peek().Id();
    switch (value_id) {
      case CSSValueJis78:
      case CSSValueJis83:
      case CSSValueJis90:
      case CSSValueJis04:
      case CSSValueSimplified:
      case CSSValueTraditional:
        if (saw_east_asian_form_value_)
          return ParseResult::kDisallowedValue;
        saw_east_asian_form_value_ = true;
        break;
      case CSSValueFullWidth:
      case CSSValueProportionalWidth:
        if (saw_east_asian_width_value_)
          return ParseResult::kDisallowedValue;
        saw_east_asian_width_value_ = true;
        break;
      case CSSValueRuby:
        if (saw_ruby_value_)
          return ParseResult::kDisallowedValue;
        saw_ruby_value_ = true;
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
  bool saw_east_asian_form_value_;
  bool saw_east_asian_width_value_;
  bool saw_ruby_value_;
  Member<CSSValueList> result_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_PARSER_FONT_VARIANT_EAST_ASIAN_PARSER_H_
