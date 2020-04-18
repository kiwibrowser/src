// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/css/parser/css_lazy_parsing_state.h"
#include "third_party/blink/renderer/core/css/parser/css_parser.h"
#include "third_party/blink/renderer/core/css/parser/css_parser_context.h"
#include "third_party/blink/renderer/core/css/style_rule.h"
#include "third_party/blink/renderer/core/css/style_sheet_contents.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class CSSLazyParsingTest : public testing::Test {
 public:
  bool HasParsedProperties(StyleRule* rule) {
    return rule->HasParsedProperties();
  }

  StyleRule* RuleAt(StyleSheetContents* sheet, size_t index) {
    return ToStyleRule(sheet->ChildRules()[index]);
  }

 protected:
  HistogramTester histogram_tester_;
  Persistent<StyleSheetContents> cached_contents_;
};

TEST_F(CSSLazyParsingTest, Simple) {
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);
  StyleSheetContents* style_sheet = StyleSheetContents::Create(context);

  String sheet_text = "body { background-color: red; }";
  CSSParser::ParseSheet(context, style_sheet, sheet_text,
                        true /* lazy parse */);
  StyleRule* rule = RuleAt(style_sheet, 0);
  EXPECT_FALSE(HasParsedProperties(rule));
  rule->Properties();
  EXPECT_TRUE(HasParsedProperties(rule));
}

// Avoid parsing rules with ::before or ::after to avoid causing
// collectFeatures() when we trigger parsing for attr();
TEST_F(CSSLazyParsingTest, DontLazyParseBeforeAfter) {
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);
  StyleSheetContents* style_sheet = StyleSheetContents::Create(context);

  String sheet_text =
      "p::before { content: 'foo' } p .class::after { content: 'bar' } ";
  CSSParser::ParseSheet(context, style_sheet, sheet_text,
                        true /* lazy parse */);

  EXPECT_TRUE(HasParsedProperties(RuleAt(style_sheet, 0)));
  EXPECT_TRUE(HasParsedProperties(RuleAt(style_sheet, 1)));
}

// Test for crbug.com/664115 where |shouldConsiderForMatchingRules| would flip
// from returning false to true if the lazy property was parsed. This is a
// dangerous API because callers will expect the set of matching rules to be
// identical if the stylesheet is not mutated.
TEST_F(CSSLazyParsingTest, ShouldConsiderForMatchingRulesDoesntChange1) {
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);
  StyleSheetContents* style_sheet = StyleSheetContents::Create(context);

  String sheet_text = "p::first-letter { ,badness, } ";
  CSSParser::ParseSheet(context, style_sheet, sheet_text,
                        true /* lazy parse */);

  StyleRule* rule = RuleAt(style_sheet, 0);
  EXPECT_FALSE(HasParsedProperties(rule));
  EXPECT_TRUE(
      rule->ShouldConsiderForMatchingRules(false /* includeEmptyRules */));

  // Parse the rule.
  rule->Properties();

  // Now, we should still consider this for matching rules even if it is empty.
  EXPECT_TRUE(HasParsedProperties(rule));
  EXPECT_TRUE(
      rule->ShouldConsiderForMatchingRules(false /* includeEmptyRules */));
}

// Test the same thing as above, with a property that does not get lazy parsed,
// to ensure that we perform the optimization where possible.
TEST_F(CSSLazyParsingTest, ShouldConsiderForMatchingRulesSimple) {
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);
  StyleSheetContents* style_sheet = StyleSheetContents::Create(context);

  String sheet_text = "p::before { ,badness, } ";
  CSSParser::ParseSheet(context, style_sheet, sheet_text,
                        true /* lazy parse */);

  StyleRule* rule = RuleAt(style_sheet, 0);
  EXPECT_TRUE(HasParsedProperties(rule));
  EXPECT_FALSE(
      rule->ShouldConsiderForMatchingRules(false /* includeEmptyRules */));
}

// Regression test for crbug.com/660290 where we change the underlying owning
// document from the StyleSheetContents without changing the UseCounter. This
// test ensures that the new UseCounter is used when doing new parsing work.
TEST_F(CSSLazyParsingTest, ChangeDocuments) {
  std::unique_ptr<DummyPageHolder> dummy_holder =
      DummyPageHolder::Create(IntSize(500, 500));
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext,
      CSSParserContext::kLiveProfile, &dummy_holder->GetDocument());
  cached_contents_ = StyleSheetContents::Create(context);
  {
    CSSStyleSheet* sheet =
        CSSStyleSheet::Create(cached_contents_, dummy_holder->GetDocument());
    DCHECK(sheet);

    String sheet_text = "body { background-color: red; } p { color: orange;  }";
    CSSParser::ParseSheet(context, cached_contents_, sheet_text,
                          true /* lazy parse */);

    // Parse the first property set with the first document as owner.
    StyleRule* rule = RuleAt(cached_contents_, 0);
    EXPECT_FALSE(HasParsedProperties(rule));
    rule->Properties();
    EXPECT_TRUE(HasParsedProperties(rule));

    EXPECT_EQ(&dummy_holder->GetDocument(),
              cached_contents_->SingleOwnerDocument());
    UseCounter& use_counter1 =
        dummy_holder->GetDocument().GetPage()->GetUseCounter();
    EXPECT_TRUE(use_counter1.IsCounted(CSSPropertyBackgroundColor));
    EXPECT_FALSE(use_counter1.IsCounted(CSSPropertyColor));

    // Change owner document.
    cached_contents_->UnregisterClient(sheet);
    dummy_holder.reset();
  }
  // Ensure no stack references to oilpan objects.
  ThreadState::Current()->CollectAllGarbage();

  std::unique_ptr<DummyPageHolder> dummy_holder2 =
      DummyPageHolder::Create(IntSize(500, 500));
  CSSStyleSheet* sheet2 =
      CSSStyleSheet::Create(cached_contents_, dummy_holder2->GetDocument());

  EXPECT_EQ(&dummy_holder2->GetDocument(),
            cached_contents_->SingleOwnerDocument());

  // Parse the second property set with the second document as owner.
  StyleRule* rule2 = RuleAt(cached_contents_, 1);
  EXPECT_FALSE(HasParsedProperties(rule2));
  rule2->Properties();
  EXPECT_TRUE(HasParsedProperties(rule2));

  UseCounter& use_counter2 =
      dummy_holder2->GetDocument().GetPage()->GetUseCounter();
  EXPECT_TRUE(sheet2);
  EXPECT_TRUE(use_counter2.IsCounted(CSSPropertyColor));
  EXPECT_FALSE(use_counter2.IsCounted(CSSPropertyBackgroundColor));
}

TEST_F(CSSLazyParsingTest, SimpleRuleUsagePercent) {
  CSSParserContext* context = CSSParserContext::Create(
      kHTMLStandardMode, SecureContextMode::kInsecureContext);
  StyleSheetContents* style_sheet = StyleSheetContents::Create(context);

  std::string usage_metric = "Style.LazyUsage.Percent";
  std::string total_rules_metric = "Style.TotalLazyRules";
  std::string total_rules_full_usage_metric = "Style.TotalLazyRules.FullUsage";
  histogram_tester_.ExpectTotalCount(usage_metric, 0);

  String sheet_text =
      "body { background-color: red; }"
      "p { color: blue; }"
      "a { color: yellow; }"
      "#id { color: blue; }"
      "div { color: grey; }";
  CSSParser::ParseSheet(context, style_sheet, sheet_text,
                        true /* lazy parse */);

  histogram_tester_.ExpectTotalCount(total_rules_metric, 1);
  histogram_tester_.ExpectUniqueSample(total_rules_metric, 5, 1);

  // Only log the full usage metric when all the rules have been actually
  // parsed.
  histogram_tester_.ExpectTotalCount(total_rules_full_usage_metric, 0);

  histogram_tester_.ExpectTotalCount(usage_metric, 1);
  histogram_tester_.ExpectUniqueSample(usage_metric,
                                       CSSLazyParsingState::kUsageGe0, 1);

  RuleAt(style_sheet, 0)->Properties();
  histogram_tester_.ExpectTotalCount(usage_metric, 2);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageGt10, 1);

  RuleAt(style_sheet, 1)->Properties();
  histogram_tester_.ExpectTotalCount(usage_metric, 3);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageGt25, 1);

  RuleAt(style_sheet, 2)->Properties();
  histogram_tester_.ExpectTotalCount(usage_metric, 4);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageGt50, 1);

  RuleAt(style_sheet, 3)->Properties();
  histogram_tester_.ExpectTotalCount(usage_metric, 5);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageGt75, 1);

  // Only log the full usage metric when all the rules have been actually
  // parsed.
  histogram_tester_.ExpectTotalCount(total_rules_full_usage_metric, 0);

  // Parsing the last rule bumps both Gt90 and All buckets.
  RuleAt(style_sheet, 4)->Properties();
  histogram_tester_.ExpectTotalCount(usage_metric, 7);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageGt90, 1);
  histogram_tester_.ExpectBucketCount(usage_metric,
                                      CSSLazyParsingState::kUsageAll, 1);

  histogram_tester_.ExpectTotalCount(total_rules_full_usage_metric, 1);
  histogram_tester_.ExpectUniqueSample(total_rules_full_usage_metric, 5, 1);
}

}  // namespace blink
