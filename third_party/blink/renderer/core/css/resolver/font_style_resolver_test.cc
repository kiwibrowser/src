// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/resolver/font_style_resolver.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"

namespace blink {

TEST(FontStyleResolverTest, Simple) {
  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(style, CSSPropertyFont, "15px Ahem", true,
                        SecureContextMode::kInsecureContext);

  FontDescription desc = FontStyleResolver::ComputeFont(*style, nullptr);

  EXPECT_EQ(desc.SpecifiedSize(), 15);
  EXPECT_EQ(desc.ComputedSize(), 15);
  EXPECT_EQ(desc.Family().Family(), "Ahem");
}

TEST(FontStyleResolverTest, InvalidSize) {
  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(style, CSSPropertyFont, "-1px Ahem", true,
                        SecureContextMode::kInsecureContext);

  FontDescription desc = FontStyleResolver::ComputeFont(*style, nullptr);

  EXPECT_EQ(desc.Family().Family(), nullptr);
  EXPECT_EQ(desc.SpecifiedSize(), 0);
  EXPECT_EQ(desc.ComputedSize(), 0);
}

TEST(FontStyleResolverTest, InvalidWeight) {
  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(style, CSSPropertyFont, "wrong 1px Ahem", true,
                        SecureContextMode::kInsecureContext);

  FontDescription desc = FontStyleResolver::ComputeFont(*style, nullptr);

  EXPECT_EQ(desc.Family().Family(), nullptr);
  EXPECT_EQ(desc.SpecifiedSize(), 0);
  EXPECT_EQ(desc.ComputedSize(), 0);
}

TEST(FontStyleResolverTest, InvalidEverything) {
  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(style, CSSPropertyFont, "wrong wrong wrong 1px Ahem",
                        true, SecureContextMode::kInsecureContext);

  FontDescription desc = FontStyleResolver::ComputeFont(*style, nullptr);

  EXPECT_EQ(desc.Family().Family(), nullptr);
  EXPECT_EQ(desc.SpecifiedSize(), 0);
  EXPECT_EQ(desc.ComputedSize(), 0);
}

TEST(FontStyleResolverTest, RelativeSize) {
  MutableCSSPropertyValueSet* style =
      MutableCSSPropertyValueSet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(style, CSSPropertyFont, "italic 2ex Ahem", true,
                        SecureContextMode::kInsecureContext);

  FontDescription desc = FontStyleResolver::ComputeFont(*style, nullptr);

  EXPECT_EQ(desc.Family().Family(), "Ahem");
  EXPECT_EQ(desc.SpecifiedSize(), 16);
  EXPECT_EQ(desc.ComputedSize(), 16);
}

}  // namespace blink
