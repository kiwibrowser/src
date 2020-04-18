// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_property_value_set.h"
#include "third_party/blink/renderer/core/css/threaded/multi_threaded_test_util.h"
#include "third_party/blink/renderer/core/css_property_names.h"

namespace blink {

class CSSParserThreadedTest : public MultiThreadedTest {
 public:
  static void TestSingle(CSSPropertyID prop, const String& text) {
    const CSSValue* value = CSSParser::ParseSingleValue(
        prop, text,
        StrictCSSParserContext(SecureContextMode::kInsecureContext));
    ASSERT_TRUE(value);
    EXPECT_EQ(text, value->CssText());
  }

  static MutableCSSPropertyValueSet* TestValue(CSSPropertyID prop,
                                               const String& text) {
    MutableCSSPropertyValueSet* style =
        MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
    CSSParser::ParseValue(style, prop, text, true,
                          SecureContextMode::kInsecureContext);
    return style;
  }
};

TSAN_TEST_F(CSSParserThreadedTest, SinglePropertyFilter) {
  RunOnThreads([]() {
    TestSingle(CSSPropertyFilter, "sepia(50%)");
    TestSingle(CSSPropertyFilter, "blur(10px)");
    TestSingle(CSSPropertyFilter, "brightness(50%) invert(100%)");
  });
}

TSAN_TEST_F(CSSParserThreadedTest, SinglePropertyFont) {
  RunOnThreads([]() {
    TestSingle(CSSPropertyFontFamily, "serif");
    TestSingle(CSSPropertyFontFamily, "monospace");
    TestSingle(CSSPropertyFontFamily, "times");
    TestSingle(CSSPropertyFontFamily, "arial");

    TestSingle(CSSPropertyFontWeight, "normal");
    TestSingle(CSSPropertyFontWeight, "bold");

    TestSingle(CSSPropertyFontSize, "10px");
    TestSingle(CSSPropertyFontSize, "20em");
  });
}

TSAN_TEST_F(CSSParserThreadedTest, ValuePropertyFont) {
  RunOnThreads([]() {
    MutableCSSPropertyValueSet* v = TestValue(CSSPropertyFont, "15px arial");
    EXPECT_EQ(v->GetPropertyValue(CSSPropertyFontFamily), "arial");
    EXPECT_EQ(v->GetPropertyValue(CSSPropertyFontSize), "15px");
  });
}

TSAN_TEST_F(CSSParserThreadedTest, FontFaceDescriptor) {
  RunOnThreads([]() {
    CSSParserContext* ctx = CSSParserContext::Create(
        kCSSFontFaceRuleMode, SecureContextMode::kInsecureContext);
    const CSSValue* v = CSSParser::ParseFontFaceDescriptor(
        CSSPropertySrc, "url(myfont.ttf)", ctx);
    ASSERT_TRUE(v);
    EXPECT_EQ(v->CssText(), "url(\"myfont.ttf\")");
  });
}

}  // namespace blink
