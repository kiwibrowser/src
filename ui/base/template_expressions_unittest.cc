// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/template_expressions.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

// Tip: ui_base_unittests --gtest_filter='TemplateExpressionsTest.*' to run
// these tests.

TEST(TemplateExpressionsTest, ReplaceTemplateExpressionsPieces) {
  TemplateReplacements substitutions;
  substitutions["test"] = "word";
  substitutions["5"] = "number";

  EXPECT_EQ("", ReplaceTemplateExpressions("", substitutions));
  EXPECT_EQ("word", ReplaceTemplateExpressions("$i18n{test}", substitutions));
  EXPECT_EQ("number ", ReplaceTemplateExpressions("$i18n{5} ", substitutions));
  EXPECT_EQ("multiple: word, number.",
            ReplaceTemplateExpressions("multiple: $i18n{test}, $i18n{5}.",
                                       substitutions));
}

TEST(TemplateExpressionsTest,
     ReplaceTemplateExpressionsConsecutiveDollarSignsPieces) {
  TemplateReplacements substitutions;
  substitutions["a"] = "x";
  EXPECT_EQ("$ $$ $$$", ReplaceTemplateExpressions("$ $$ $$$", substitutions));
  EXPECT_EQ("$x", ReplaceTemplateExpressions("$$i18n{a}", substitutions));
  EXPECT_EQ("$$x", ReplaceTemplateExpressions("$$$i18n{a}", substitutions));
  EXPECT_EQ("$i1812", ReplaceTemplateExpressions("$i1812", substitutions));
}

TEST(TemplateExpressionsTest, ReplaceTemplateExpressionsEscaping) {
  static TemplateReplacements substitutions;
  substitutions["punctuationSample"] = "a\"b'c<d>e&f";
  substitutions["htmlSample"] = "<div>hello</div>";
  EXPECT_EQ(
      "a&quot;b&#39;c&lt;d&gt;e&amp;f",
      ReplaceTemplateExpressions("$i18n{punctuationSample}", substitutions));
  EXPECT_EQ("&lt;div&gt;hello&lt;/div&gt;",
            ReplaceTemplateExpressions("$i18n{htmlSample}", substitutions));
  EXPECT_EQ(
      "multiple: &lt;div&gt;hello&lt;/div&gt;, a&quot;b&#39;c&lt;d&gt;e&amp;f.",
      ReplaceTemplateExpressions(
          "multiple: $i18n{htmlSample}, $i18n{punctuationSample}.",
          substitutions));
}

TEST(TemplateExpressionsTest, ReplaceTemplateExpressionsRaw) {
  static TemplateReplacements substitutions;
  substitutions["rawSample"] = "<a href=\"example.com\">hello</a>";
  EXPECT_EQ("<a href=\"example.com\">hello</a>",
            ReplaceTemplateExpressions("$i18nRaw{rawSample}", substitutions));
}

TEST(TemplateExpressionsTest, ReplaceTemplateExpressionsPolymerQuoting) {
  static TemplateReplacements substitutions;
  substitutions["singleSample"] = "don't do it";
  substitutions["doubleSample"] = "\"moo\" said the cow";
  // This resolves |Call('don\'t do it')| to Polymer, which is presented as
  // |don't do it| to the user.
  EXPECT_EQ("<div>[[Call('don\\'t do it')]]",
            ReplaceTemplateExpressions(
                "<div>[[Call('$i18nPolymer{singleSample}')]]", substitutions));
  // This resolves |Call('\"moo\" said the cow')| to Polymer, which is
  // presented as |"moo" said the cow| to the user.
  EXPECT_EQ("<div>[[Call('&quot;moo&quot; said the cow')]]",
            ReplaceTemplateExpressions(
                "<div>[[Call('$i18nPolymer{doubleSample}')]]", substitutions));
}

TEST(TemplateExpressionsTest, ReplaceTemplateExpressionsPolymerMixed) {
  static TemplateReplacements substitutions;
  substitutions["punctuationSample"] = "a\"b'c<d>e&f,g";
  substitutions["htmlSample"] = "<div>hello</div>";
  EXPECT_EQ("a&quot;b\\'c<d>e&f\\,g",
            ReplaceTemplateExpressions("$i18nPolymer{punctuationSample}",
                                       substitutions));
  EXPECT_EQ("<div>hello</div>", ReplaceTemplateExpressions(
                                    "$i18nPolymer{htmlSample}", substitutions));
  EXPECT_EQ("multiple: <div>hello</div>, a&quot;b\\'c<d>e&f\\,g.",
            ReplaceTemplateExpressions("multiple: $i18nPolymer{htmlSample}, "
                                       "$i18nPolymer{punctuationSample}.",
                                       substitutions));
}

}  // namespace ui
