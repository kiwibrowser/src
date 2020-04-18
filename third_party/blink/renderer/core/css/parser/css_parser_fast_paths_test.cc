// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/css_parser_fast_paths.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_color_value.h"
#include "third_party/blink/renderer/core/css/css_identifier_value.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"

namespace blink {

using namespace cssvalue;

TEST(CSSParserFastPathsTest, ParseKeyword) {
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyFloat, "left", kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsIdentifierValue());
  CSSIdentifierValue* identifier_value = ToCSSIdentifierValue(value);
  EXPECT_EQ(CSSValueLeft, identifier_value->GetValueID());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyFloat, "foo",
                                              kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
}

TEST(CSSParserFastPathsTest, ParseCSSWideKeywords) {
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyMarginTop, "inherit", kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsInheritedValue());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMarginRight, "InHeriT",
                                              kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsInheritedValue());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMarginBottom,
                                              "initial", kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsInitialValue());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMarginLeft, "IniTiaL",
                                              kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsInitialValue());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMarginTop, "unset",
                                              kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsUnsetValue());
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMarginLeft, "unsEt",
                                              kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  EXPECT_TRUE(value->IsUnsetValue());
  // Fast path doesn't handle short hands.
  value = CSSParserFastPaths::MaybeParseValue(CSSPropertyMargin, "initial",
                                              kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
}

TEST(CSSParserFastPathsTest, ParseTransform) {
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "translate(5.5px, 5px)", kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  ASSERT_TRUE(value->IsValueList());
  ASSERT_EQ("translate(5.5px, 5px)", value->CssText());

  value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "translate3d(5px, 5px, 10.1px)", kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  ASSERT_TRUE(value->IsValueList());
  ASSERT_EQ("translate3d(5px, 5px, 10.1px)", value->CssText());
}

TEST(CSSParserFastPathsTest, ParseComplexTransform) {
  // Random whitespace is on purpose.
  static const char* kComplexTransform =
      "translateX(5px) "
      "translateZ(20.5px)   "
      "translateY(10px) "
      "scale3d(0.5, 1, 0.7)   "
      "matrix3d(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)   ";
  static const char* kComplexTransformNormalized =
      "translateX(5px) "
      "translateZ(20.5px) "
      "translateY(10px) "
      "scale3d(0.5, 1, 0.7) "
      "matrix3d(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)";
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, kComplexTransform, kHTMLStandardMode);
  ASSERT_NE(nullptr, value);
  ASSERT_TRUE(value->IsValueList());
  ASSERT_EQ(kComplexTransformNormalized, value->CssText());
}

TEST(CSSParserFastPathsTest, ParseTransformNotFastPath) {
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "rotateX(1deg)", kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
  value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "translateZ(1px) rotateX(1deg)", kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
}

TEST(CSSParserFastPathsTest, ParseInvalidTransform) {
  CSSValue* value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "rotateX(1deg", kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
  value = CSSParserFastPaths::MaybeParseValue(
      CSSPropertyTransform, "translateZ(1px) (1px, 1px) rotateX(1deg",
      kHTMLStandardMode);
  ASSERT_EQ(nullptr, value);
}

TEST(CSSParserFastPathsTest, ParseColorWithLargeAlpha) {
  CSSValue* value = CSSParserFastPaths::ParseColor("rgba(0,0,0,1893205797.13)",
                                                   kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());
}

TEST(CSSParserFastPathsTest, ParseColorWithNewSyntax) {
  CSSValue* value =
      CSSParserFastPaths::ParseColor("rgba(0 0 0)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("rgba(0 0 0 / 1)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("rgba(0, 0, 0, 1)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("RGBA(0 0 0 / 1)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("RGB(0 0 0 / 1)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("rgba(0 0 0 0)", kHTMLStandardMode);
  EXPECT_EQ(nullptr, value);

  value = CSSParserFastPaths::ParseColor("rgba(0, 0 0 1)", kHTMLStandardMode);
  EXPECT_EQ(nullptr, value);

  value =
      CSSParserFastPaths::ParseColor("rgba(0, 0, 0 / 1)", kHTMLStandardMode);
  EXPECT_EQ(nullptr, value);

  value = CSSParserFastPaths::ParseColor("rgba(0 0 0, 1)", kHTMLStandardMode);
  EXPECT_EQ(nullptr, value);
}

TEST(CSSParserFastPathsTest, ParseColorWithDecimal) {
  CSSValue* value = CSSParserFastPaths::ParseColor("rgba(0.0, 0.0, 0.0, 1.0)",
                                                   kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value =
      CSSParserFastPaths::ParseColor("rgb(0.0, 0.0, 0.0)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value =
      CSSParserFastPaths::ParseColor("rgb(0.0 , 0.0,0.0)", kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kBlack, ToCSSColorValue(*value).Value());

  value = CSSParserFastPaths::ParseColor("rgb(254.5, 254.5, 254.5)",
                                         kHTMLStandardMode);
  EXPECT_NE(nullptr, value);
  EXPECT_TRUE(value->IsColorValue());
  EXPECT_EQ(Color::kWhite, ToCSSColorValue(*value).Value());
}

}  // namespace blink
