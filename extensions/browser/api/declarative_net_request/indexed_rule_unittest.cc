// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/declarative_net_request/indexed_rule.h"

#include <memory>
#include <utility>

#include "base/format_macros.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/stringprintf.h"
#include "components/version_info/version_info.h"
#include "extensions/browser/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request.h"
#include "extensions/common/features/feature_channel.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace declarative_net_request {
namespace {

namespace flat_rule = url_pattern_index::flat;
namespace dnr_api = extensions::api::declarative_net_request;

std::unique_ptr<dnr_api::Rule> CreateGenericParsedRule() {
  auto rule = std::make_unique<dnr_api::Rule>();
  rule->id = kMinValidID;
  rule->condition.url_filter = std::make_unique<std::string>("filter");
  rule->action.type = dnr_api::RULE_ACTION_TYPE_BLACKLIST;
  return rule;
}

class IndexedRuleTest : public testing::Test {
 public:
  IndexedRuleTest() : channel_(::version_info::Channel::UNKNOWN) {}

 private:
  ScopedCurrentChannel channel_;

  DISALLOW_COPY_AND_ASSIGN(IndexedRuleTest);
};

TEST_F(IndexedRuleTest, IDParsing) {
  struct {
    const int id;
    const ParseResult expected_result;
  } cases[] = {
      {kMinValidID - 1, ParseResult::ERROR_INVALID_RULE_ID},
      {kMinValidID, ParseResult::SUCCESS},
      {kMinValidID + 1, ParseResult::SUCCESS},
  };
  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->id = cases[i].id;

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(base::checked_cast<uint32_t>(cases[i].id), indexed_rule.id);
  }
}

TEST_F(IndexedRuleTest, PriorityParsing) {
  struct {
    std::unique_ptr<int> priority;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const uint32_t expected_priority;
  } cases[] = {
      {std::make_unique<int>(kMinValidPriority - 1),
       ParseResult::ERROR_INVALID_REDIRECT_RULE_PRIORITY, kDefaultPriority},
      {std::make_unique<int>(kMinValidPriority), ParseResult::SUCCESS,
       kMinValidPriority},
      {std::make_unique<int>(kMinValidPriority + 1), ParseResult::SUCCESS,
       kMinValidPriority + 1},
      {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_RULE_PRIORITY,
       kDefaultPriority},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->priority = std::move(cases[i].priority);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->action.redirect_url =
        std::make_unique<std::string>("http://google.com");

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_priority, indexed_rule.priority);
  }

  // Ensure priority is ignored for non-redirect rules.
  {
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->priority = std::make_unique<int>(5);
    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);
    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(static_cast<uint32_t>(kDefaultPriority), indexed_rule.priority);
  }
}

TEST_F(IndexedRuleTest, OptionsParsing) {
  struct {
    const dnr_api::DomainType domain_type;
    const dnr_api::RuleActionType action_type;
    std::unique_ptr<bool> is_url_filter_case_sensitive;
    const uint8_t expected_options;
  } cases[] = {
      {dnr_api::DOMAIN_TYPE_NONE, dnr_api::RULE_ACTION_TYPE_BLACKLIST, nullptr,
       flat_rule::OptionFlag_APPLIES_TO_THIRD_PARTY |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY},
      {dnr_api::DOMAIN_TYPE_FIRSTPARTY, dnr_api::RULE_ACTION_TYPE_WHITELIST,
       std::make_unique<bool>(true),
       flat_rule::OptionFlag_IS_WHITELIST |
           flat_rule::OptionFlag_APPLIES_TO_FIRST_PARTY |
           flat_rule::OptionFlag_IS_MATCH_CASE},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->condition.domain_type = cases[i].domain_type;
    rule->action.type = cases[i].action_type;
    rule->condition.is_url_filter_case_sensitive =
        std::move(cases[i].is_url_filter_case_sensitive);

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(ParseResult::SUCCESS, result);
    EXPECT_EQ(cases[i].expected_options, indexed_rule.options);
  }
}

TEST_F(IndexedRuleTest, ResourceTypesParsing) {
  using ResourceTypeVec = std::vector<dnr_api::ResourceType>;

  struct {
    std::unique_ptr<ResourceTypeVec> resource_types;
    std::unique_ptr<ResourceTypeVec> excluded_resource_types;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const uint16_t expected_element_types;
  } cases[] = {
      {nullptr, nullptr, ParseResult::SUCCESS,
       flat_rule::ElementType_ANY & ~flat_rule::ElementType_MAIN_FRAME},
      {nullptr,
       std::make_unique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       ParseResult::SUCCESS,
       flat_rule::ElementType_ANY & ~flat_rule::ElementType_SCRIPT},
      {std::make_unique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_SCRIPT, dnr_api::RESOURCE_TYPE_IMAGE})),
       nullptr, ParseResult::SUCCESS,
       flat_rule::ElementType_SCRIPT | flat_rule::ElementType_IMAGE},
      {std::make_unique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_SCRIPT, dnr_api::RESOURCE_TYPE_IMAGE})),
       std::make_unique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       ParseResult::ERROR_RESOURCE_TYPE_DUPLICATED,
       flat_rule::ElementType_NONE},
      {nullptr,
       std::make_unique<ResourceTypeVec>(ResourceTypeVec(
           {dnr_api::RESOURCE_TYPE_MAIN_FRAME, dnr_api::RESOURCE_TYPE_SUB_FRAME,
            dnr_api::RESOURCE_TYPE_STYLESHEET, dnr_api::RESOURCE_TYPE_SCRIPT,
            dnr_api::RESOURCE_TYPE_IMAGE, dnr_api::RESOURCE_TYPE_FONT,
            dnr_api::RESOURCE_TYPE_OBJECT,
            dnr_api::RESOURCE_TYPE_XMLHTTPREQUEST, dnr_api::RESOURCE_TYPE_PING,
            dnr_api::RESOURCE_TYPE_CSP_REPORT, dnr_api::RESOURCE_TYPE_MEDIA,
            dnr_api::RESOURCE_TYPE_WEBSOCKET, dnr_api::RESOURCE_TYPE_OTHER})),
       ParseResult::ERROR_NO_APPLICABLE_RESOURCE_TYPES,
       flat_rule::ElementType_NONE},
      {std::make_unique<ResourceTypeVec>(ResourceTypeVec()),
       std::make_unique<ResourceTypeVec>(ResourceTypeVec()),
       ParseResult::ERROR_EMPTY_RESOURCE_TYPES_LIST,
       flat_rule::ElementType_NONE},
      {std::make_unique<ResourceTypeVec>(
           ResourceTypeVec({dnr_api::RESOURCE_TYPE_SCRIPT})),
       std::make_unique<ResourceTypeVec>(ResourceTypeVec()),
       ParseResult::SUCCESS, flat_rule::ElementType_SCRIPT},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->condition.resource_types = std::move(cases[i].resource_types);
    rule->condition.excluded_resource_types =
        std::move(cases[i].excluded_resource_types);

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_element_types, indexed_rule.element_types);
  }
}

TEST_F(IndexedRuleTest, UrlFilterParsing) {
  struct {
    std::unique_ptr<std::string> input_url_filter;

    // Only valid if |expected_result| is SUCCESS.
    const flat_rule::UrlPatternType expected_url_pattern_type;
    const flat_rule::AnchorType expected_anchor_left;
    const flat_rule::AnchorType expected_anchor_right;
    const std::string expected_url_pattern;

    const ParseResult expected_result;
  } cases[] = {
      {nullptr, flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_NONE,
       flat_rule::AnchorType_NONE, "", ParseResult::SUCCESS},
      {std::make_unique<std::string>(""), flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_NONE, flat_rule::AnchorType_NONE, "",
       ParseResult::ERROR_EMPTY_URL_FILTER},
      {std::make_unique<std::string>("|"), flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_BOUNDARY, flat_rule::AnchorType_NONE, "",
       ParseResult::SUCCESS},
      {std::make_unique<std::string>("||"), flat_rule::UrlPatternType_SUBSTRING,
       flat_rule::AnchorType_SUBDOMAIN, flat_rule::AnchorType_NONE, "",
       ParseResult::SUCCESS},
      {std::make_unique<std::string>("|||"),
       flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_SUBDOMAIN,
       flat_rule::AnchorType_BOUNDARY, "", ParseResult::SUCCESS},
      {std::make_unique<std::string>("|*|||"),
       flat_rule::UrlPatternType_WILDCARDED, flat_rule::AnchorType_BOUNDARY,
       flat_rule::AnchorType_BOUNDARY, "*||", ParseResult::SUCCESS},
      {std::make_unique<std::string>("|xyz|"),
       flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_BOUNDARY,
       flat_rule::AnchorType_BOUNDARY, "xyz", ParseResult::SUCCESS},
      {std::make_unique<std::string>("||x^yz"),
       flat_rule::UrlPatternType_WILDCARDED, flat_rule::AnchorType_SUBDOMAIN,
       flat_rule::AnchorType_NONE, "x^yz", ParseResult::SUCCESS},
      {std::make_unique<std::string>("||xyz|"),
       flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_SUBDOMAIN,
       flat_rule::AnchorType_BOUNDARY, "xyz", ParseResult::SUCCESS},
      {std::make_unique<std::string>("x*y|z"),
       flat_rule::UrlPatternType_WILDCARDED, flat_rule::AnchorType_NONE,
       flat_rule::AnchorType_NONE, "x*y|z", ParseResult::SUCCESS},
      {std::make_unique<std::string>("**^"),
       flat_rule::UrlPatternType_WILDCARDED, flat_rule::AnchorType_NONE,
       flat_rule::AnchorType_NONE, "**^", ParseResult::SUCCESS},
      {std::make_unique<std::string>("||google.com"),
       flat_rule::UrlPatternType_SUBSTRING, flat_rule::AnchorType_SUBDOMAIN,
       flat_rule::AnchorType_NONE, "google.com", ParseResult::SUCCESS},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->condition.url_filter = std::move(cases[i].input_url_filter);

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);
    if (result != ParseResult::SUCCESS)
      continue;

    EXPECT_EQ(cases[i].expected_result, result);
    EXPECT_EQ(cases[i].expected_url_pattern_type,
              indexed_rule.url_pattern_type);
    EXPECT_EQ(cases[i].expected_anchor_left, indexed_rule.anchor_left);
    EXPECT_EQ(cases[i].expected_anchor_right, indexed_rule.anchor_right);
    EXPECT_EQ(cases[i].expected_url_pattern, indexed_rule.url_pattern);
  }
}

TEST_F(IndexedRuleTest, DomainsParsing) {
  using DomainVec = std::vector<std::string>;
  struct {
    std::unique_ptr<DomainVec> domains;
    std::unique_ptr<DomainVec> excluded_domains;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const DomainVec expected_domains;
    const DomainVec expected_excluded_domains;
  } cases[] = {
      {nullptr, nullptr, ParseResult::SUCCESS, {}, {}},
      {std::make_unique<DomainVec>(DomainVec()),
       nullptr,
       ParseResult::ERROR_EMPTY_DOMAINS_LIST,
       {},
       {}},
      {nullptr,
       std::make_unique<DomainVec>(DomainVec()),
       ParseResult::SUCCESS,
       {},
       {}},
      {std::make_unique<DomainVec>(DomainVec({"a.com", "b.com", "a.com"})),
       std::make_unique<DomainVec>(
           DomainVec({"g.com", "XY.COM", "zzz.com", "a.com", "google.com"})),
       ParseResult::SUCCESS,
       {"a.com", "a.com", "b.com"},
       {"google.com", "zzz.com", "xy.com", "a.com", "g.com"}}};

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->condition.domains = std::move(cases[i].domains);
    rule->condition.excluded_domains = std::move(cases[i].excluded_domains);

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS) {
      EXPECT_EQ(cases[i].expected_domains, indexed_rule.domains);
      EXPECT_EQ(cases[i].expected_excluded_domains,
                indexed_rule.excluded_domains);
    }
  }
}

TEST_F(IndexedRuleTest, RedirectUrlParsing) {
  struct {
    std::unique_ptr<std::string> redirect_url;
    const ParseResult expected_result;
    // Only valid if |expected_result| is SUCCESS.
    const std::string expected_redirect_url;
  } cases[] = {{std::make_unique<std::string>(""),
                ParseResult::ERROR_EMPTY_REDIRECT_URL, ""},
               {nullptr, ParseResult::ERROR_EMPTY_REDIRECT_URL, ""},
               {std::make_unique<std::string>("http://google.com"),
                ParseResult::SUCCESS, "http://google.com"},
               {std::make_unique<std::string>("abc"),
                ParseResult::ERROR_INVALID_REDIRECT_URL, ""}};

  for (size_t i = 0; i < arraysize(cases); ++i) {
    SCOPED_TRACE(base::StringPrintf("Testing case[%" PRIuS "]", i));
    std::unique_ptr<dnr_api::Rule> rule = CreateGenericParsedRule();
    rule->action.redirect_url = std::move(cases[i].redirect_url);
    rule->action.type = dnr_api::RULE_ACTION_TYPE_REDIRECT;
    rule->priority = std::make_unique<int>(kMinValidPriority);

    IndexedRule indexed_rule;
    ParseResult result =
        IndexedRule::CreateIndexedRule(std::move(rule), &indexed_rule);

    EXPECT_EQ(cases[i].expected_result, result);
    if (result == ParseResult::SUCCESS)
      EXPECT_EQ(cases[i].expected_redirect_url, indexed_rule.redirect_url);
  }
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
