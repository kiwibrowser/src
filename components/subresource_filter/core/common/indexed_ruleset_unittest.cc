// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/core/common/indexed_ruleset.h"

#include <memory>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "components/subresource_filter/core/common/first_party_origin.h"
#include "components/url_pattern_index/proto/rules.pb.h"
#include "components/url_pattern_index/url_pattern.h"
#include "components/url_pattern_index/url_rule_test_support.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace subresource_filter {

namespace proto = url_pattern_index::proto;
namespace testing = url_pattern_index::testing;
using testing::MakeUrlRule;
using url_pattern_index::UrlPattern;

class SubresourceFilterIndexedRulesetTest : public ::testing::Test {
 public:
  SubresourceFilterIndexedRulesetTest() { Reset(); }

 protected:
  bool ShouldAllow(base::StringPiece url,
                   base::StringPiece document_origin = nullptr,
                   proto::ElementType element_type = testing::kOther,
                   bool disable_generic_rules = false) const {
    DCHECK(matcher_);
    return !matcher_->ShouldDisallowResourceLoad(
        GURL(url), FirstPartyOrigin(testing::GetOrigin(document_origin)),
        element_type, disable_generic_rules);
  }

  bool MatchingRule(base::StringPiece url,
                    base::StringPiece document_origin = nullptr,
                    proto::ElementType element_type = testing::kOther,
                    bool disable_generic_rules = false) const {
    DCHECK(matcher_);
    return matcher_->MatchedUrlRule(
               GURL(url), FirstPartyOrigin(testing::GetOrigin(document_origin)),
               element_type, disable_generic_rules) != nullptr;
  }

  bool ShouldDeactivate(
      base::StringPiece document_url,
      base::StringPiece parent_document_origin = nullptr,
      proto::ActivationType activation_type = testing::kNoActivation) const {
    DCHECK(matcher_);
    return matcher_->ShouldDisableFilteringForDocument(
        GURL(document_url), testing::GetOrigin(parent_document_origin),
        activation_type);
  }

  bool AddUrlRule(const proto::UrlRule& rule) {
    return indexer_->AddUrlRule(rule);
  }

  bool AddSimpleRule(base::StringPiece url_pattern) {
    return AddUrlRule(
        MakeUrlRule(UrlPattern(url_pattern, testing::kSubstring)));
  }

  bool AddSimpleWhitelistRule(base::StringPiece url_pattern) {
    auto rule = MakeUrlRule(UrlPattern(url_pattern, testing::kSubstring));
    rule.set_semantics(proto::RULE_SEMANTICS_WHITELIST);
    return AddUrlRule(rule);
  }

  bool AddSimpleWhitelistRule(base::StringPiece url_pattern,
                              int32_t activation_types) {
    auto rule = MakeUrlRule(UrlPattern(url_pattern, testing::kSubstring));
    rule.set_semantics(proto::RULE_SEMANTICS_WHITELIST);
    rule.clear_element_types();
    rule.set_activation_types(activation_types);
    return AddUrlRule(rule);
  }

  void Finish() {
    indexer_->Finish();
    matcher_.reset(
        new IndexedRulesetMatcher(indexer_->data(), indexer_->size()));
  }

  void Reset() {
    matcher_.reset(nullptr);
    indexer_.reset(new RulesetIndexer);
  }

  std::unique_ptr<RulesetIndexer> indexer_;
  std::unique_ptr<IndexedRulesetMatcher> matcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterIndexedRulesetTest);
};

TEST_F(SubresourceFilterIndexedRulesetTest, EmptyRuleset) {
  Finish();
  EXPECT_TRUE(ShouldAllow(nullptr));
  EXPECT_TRUE(ShouldAllow("http://example.com"));
  EXPECT_TRUE(ShouldAllow("http://another.example.com?param=val"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, NoRuleApplies) {
  ASSERT_TRUE(AddSimpleRule("?filter_out="));
  ASSERT_TRUE(AddSimpleRule("&filter_out="));
  Finish();

  EXPECT_TRUE(ShouldAllow("http://example.com"));
  EXPECT_TRUE(ShouldAllow("http://example.com?filter_not"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, SimpleBlacklist) {
  ASSERT_TRUE(AddSimpleRule("?param="));
  Finish();

  EXPECT_TRUE(ShouldAllow("https://example.com"));
  EXPECT_FALSE(ShouldAllow("http://example.org?param=image1"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, SimpleWhitelist) {
  ASSERT_TRUE(AddSimpleWhitelistRule("example.com/?filter_out="));
  Finish();

  EXPECT_TRUE(ShouldAllow("https://example.com?filter_out=true"));
}

// Ensure patterns containing non-ascii characters are disallowed.
TEST_F(SubresourceFilterIndexedRulesetTest, NonAsciiPatterns) {
  // non-ascii character é.
  std::string non_ascii = base::WideToUTF8(L"\u00E9");
  ASSERT_FALSE(AddSimpleRule(non_ascii));
  Finish();

  EXPECT_TRUE(ShouldAllow("https://example.com/q=" + non_ascii));
}

// Ensure that specifying non-ascii characters in percent encoded form in
// patterns works.
TEST_F(SubresourceFilterIndexedRulesetTest, PercentEncodedPatterns) {
  // Percent encoded form of é.
  ASSERT_TRUE(AddSimpleRule("%C3%A9"));
  Finish();

  EXPECT_FALSE(
      ShouldAllow("https://example.com/q=" + base::WideToUTF8(L"\u00E9")));
}

// Ensures that specifying patterns in punycode works for matching IDN domains.
TEST_F(SubresourceFilterIndexedRulesetTest, IDNHosts) {
  // ҏӊԟҭв.com
  const std::string punycode = "xn--b1a9p8c1e8r.com";
  ASSERT_TRUE(AddSimpleRule(punycode));
  Finish();

  EXPECT_FALSE(ShouldAllow("https://" + punycode));
  EXPECT_FALSE(ShouldAllow(
      base::WideToUTF8(L"https://\x048f\x04ca\x051f\x04ad\x0432.com")));
}

// Ensure patterns containing non-ascii domains are disallowed.
TEST_F(SubresourceFilterIndexedRulesetTest, NonAsciiDomain) {
  const char* kUrl = "http://example.com";

  // ґғ.com
  std::string non_ascii_domain = base::WideToUTF8(L"\x0491\x0493.com");

  auto rule = MakeUrlRule(UrlPattern(kUrl, testing::kSubstring));
  testing::AddDomains({non_ascii_domain}, &rule);
  ASSERT_FALSE(AddUrlRule(rule));

  rule = MakeUrlRule(UrlPattern(kUrl, testing::kSubstring));
  std::string non_ascii_excluded_domain = "~" + non_ascii_domain;
  testing::AddDomains({non_ascii_excluded_domain}, &rule);
  ASSERT_FALSE(AddUrlRule(rule));

  Finish();
}

// Ensure patterns with percent encoded hosts match correctly.
TEST_F(SubresourceFilterIndexedRulesetTest, PercentEncodedHostPattern) {
  const char* kPercentEncodedHost = "http://%2C.com/";
  ASSERT_TRUE(AddSimpleRule(kPercentEncodedHost));
  Finish();

  EXPECT_FALSE(ShouldAllow("http://,.com/"));
  EXPECT_FALSE(ShouldAllow(kPercentEncodedHost));
}

// Verifies the behavior for rules having percent encoded domains.
TEST_F(SubresourceFilterIndexedRulesetTest, PercentEncodedDomain) {
  const char* kUrl = "http://example.com";
  std::string percent_encoded_host = "%2C.com";

  auto rule = MakeUrlRule(UrlPattern(kUrl, testing::kSubstring));
  testing::AddDomains({percent_encoded_host}, &rule);
  ASSERT_TRUE(AddUrlRule(rule));
  Finish();

  // Note: This should actually fail. However url_pattern_index lower cases all
  // domains. Hence it doesn't correctly deal with domains having escape
  // characters which are percent-encoded in upper case by Chrome's url parser.
  EXPECT_TRUE(ShouldAllow(kUrl, "http://" + percent_encoded_host));
  EXPECT_TRUE(ShouldAllow(kUrl, "http://,.com"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, SimpleBlacklistAndWhitelist) {
  ASSERT_TRUE(AddSimpleRule("?filter="));
  ASSERT_TRUE(AddSimpleWhitelistRule("whitelisted.com/?filter="));
  Finish();

  EXPECT_FALSE(ShouldAllow("http://blacklisted.com?filter=on"));
  EXPECT_TRUE(ShouldAllow("https://whitelisted.com/?filter=on"));
  EXPECT_TRUE(ShouldAllow("https://notblacklisted.com"));
}

TEST_F(SubresourceFilterIndexedRulesetTest,
       OneBlacklistAndOneDeactivationRule) {
  ASSERT_TRUE(AddSimpleRule("example.com"));
  ASSERT_TRUE(AddSimpleWhitelistRule("example.com", testing::kDocument));
  Finish();

  EXPECT_TRUE(
      ShouldDeactivate("https://example.com", nullptr, testing::kDocument));
  EXPECT_FALSE(
      ShouldDeactivate("https://xample.com", nullptr, testing::kDocument));
  EXPECT_FALSE(ShouldAllow("https://example.com"));
  EXPECT_TRUE(ShouldAllow("https://xample.com"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, MatchingEmptyRuleset) {
  Finish();
  EXPECT_FALSE(MatchingRule(nullptr));
  EXPECT_FALSE(MatchingRule("http://example.com"));
  EXPECT_FALSE(MatchingRule("http://another.example.com?param=val"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, MatchingNoRuleApplies) {
  ASSERT_TRUE(AddSimpleRule("?filter_out="));
  ASSERT_TRUE(AddSimpleRule("&filter_out="));
  Finish();

  EXPECT_FALSE(MatchingRule("http://example.com"));
  EXPECT_FALSE(MatchingRule("http://example.com?filter_not"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, MatchingSimpleBlacklist) {
  ASSERT_TRUE(AddSimpleRule("?param="));
  Finish();

  EXPECT_FALSE(MatchingRule("https://example.com"));
  EXPECT_TRUE(MatchingRule("http://example.org?param=image1"));
}

TEST_F(SubresourceFilterIndexedRulesetTest, MatchingSimpleWhitelist) {
  ASSERT_TRUE(AddSimpleWhitelistRule("example.com/?filter_out="));
  Finish();

  EXPECT_FALSE(MatchingRule("https://example.com?filter_out=true"));
}

TEST_F(SubresourceFilterIndexedRulesetTest,
       MatchingSimpleBlacklistAndWhitelist) {
  ASSERT_TRUE(AddSimpleRule("?filter="));
  ASSERT_TRUE(AddSimpleWhitelistRule("whitelisted.com/?filter="));
  Finish();

  EXPECT_TRUE(MatchingRule("http://blacklisted.com?filter=on"));
  EXPECT_TRUE(MatchingRule("https://whitelisted.com?filter=on"));
  EXPECT_FALSE(MatchingRule("https://notblacklisted.com"));
}

TEST_F(SubresourceFilterIndexedRulesetTest,
       MatchingOneBlacklistAndOneDeactivationRule) {
  ASSERT_TRUE(AddSimpleRule("example.com"));
  ASSERT_TRUE(AddSimpleWhitelistRule("example.com", testing::kDocument));
  Finish();
  EXPECT_TRUE(MatchingRule("https://example.com"));
  EXPECT_FALSE(MatchingRule("https://xample.com"));
}

}  // namespace subresource_filter
