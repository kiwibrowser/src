// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/css_style_declaration.h"

#include "third_party/blink/renderer/core/css/css_rule_list.h"
#include "third_party/blink/renderer/core/css/css_style_rule.h"
#include "third_party/blink/renderer/core/css/css_test_helper.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(CSSStyleDeclarationTest, getPropertyShorthand) {
  CSSTestHelper helper;

  helper.AddCSSRules("div { padding: var(--p); }");
  ASSERT_TRUE(helper.CssRules());
  ASSERT_EQ(1u, helper.CssRules()->length());
  ASSERT_EQ(CSSRule::kStyleRule, helper.CssRules()->item(0)->type());
  CSSStyleRule* style_rule = ToCSSStyleRule(helper.CssRules()->item(0));
  CSSStyleDeclaration* style = style_rule->style();
  ASSERT_TRUE(style);
  EXPECT_EQ(AtomicString(), style->GetPropertyShorthand("padding"));
}

}  // namespace blink
