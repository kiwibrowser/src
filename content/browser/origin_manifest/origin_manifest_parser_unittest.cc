// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "content/browser/origin_manifest/origin_manifest_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
namespace origin_manifest_parser {

class OriginManifestParserTest : public testing::Test {};

// Note that we do not initialize the persistent store here since that
// is subject to testing the persistent store itself.

TEST_F(OriginManifestParserTest, ParsingSuccess) {
  // basically it should only create an Origin Manifest on valid JSON
  struct TestCase {
    std::string json;
    bool equalsNull;
  } cases[] = {
      {"", true},
      {"{ \"I am not valid\"", true},
      {"{ }", false},
      {"{ \"valid\": \"JSON\", \"but\": [\"unknown\", \"structure\"] }", false},
      {"{ \"tls\": {}, \"tls\": {} }", false}};

  for (const auto& test : cases) {
    EXPECT_EQ(Parse(test.json).get() == nullptr, test.equalsNull);
  }
}

TEST_F(OriginManifestParserTest, ParseCSPWithUnexpectedSyntax) {
  // CSP value not a list
  std::string json = "{ \"content-security-policy\": 42 }";
  std::unique_ptr<blink::OriginManifest> om = Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            0ul);

  // not a list of objects but directly the CSP attributes
  json =
      "{ \"content-security-policy\": [ \"this is not the content\", \"you are "
      "looking for\" ] }";
  om = Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            0ul);

  // CSP alongside unknown attributes should be fine
  json =
      "{ "
      "\"unknown\": 42,"
      "\"content-security-policy\": ["
      "  { \"policy\": \"default-src 'none'\" },"
      "],"
      "\"hardcore\": \"unknown\","
      " }";
  om = Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            1ul);

  // containing unknown attributes should be fine
  json =
      "{ "
      "\"content-security-policy\": [ {"
      "  \"unknown\": 42,"
      "  \"policy\": \"default-src 'none'\","
      "  \"hardcore\": \"unknown\","
      " } ],"
      "}";
  om = Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            1ul);
  EXPECT_EQ(
      om->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)[0]
          .policy,
      "default-src 'none'");

  // multiple CSPs
  json =
      "{ \"content-security-policy\": ["
      "{ \"policy\": \"default-src 'none'\" },"
      "{ \"policy\": \"default-src 'none'\" }"
      "] }";
  om = Parse(json);
  ASSERT_FALSE(om.get() == nullptr);
  EXPECT_EQ(om->GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            2ul);
}

TEST_F(OriginManifestParserTest, ParseCSPWithMissingAttributes) {
  // invalid values are tested separately below, no need to re-test it here
  struct TestCase {
    std::string policy;
    std::string disposition;
    std::string allow_override;
    std::string expected_policy;
    blink::OriginManifest::ContentSecurityPolicyType expected_disposition;
    bool expected_allow_override;
  } cases[] = {
      {"", "", "", "",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, false},
      {"", "report-only", "true", "",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, true},
      {"", "", "true", "",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, true},
      {"", "report-only", "", "",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, false},
      {"default-src 'none'", "", "true", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, true},
      {"default-src 'none'", "", "", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kEnforce, false},
      {"default-src 'none'", "report-only", "", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, false},
      {"default-src 'none'", "report-only", "true", "default-src 'none'",
       blink::OriginManifest::ContentSecurityPolicyType::kReport, true},
  };

  for (const auto& test : cases) {
    std::ostringstream str_stream;
    str_stream << "{ \"content-security-policy\": [ {";
    if (test.policy.length() > 0)
      str_stream << "\"policy\": \"" << test.policy << "\", ";
    if (test.disposition.length() > 0)
      str_stream << "\"disposition\": \"" << test.disposition << "\", ";
    if (test.allow_override.length() > 0)
      str_stream << "\"allow-override\": " << test.allow_override << ", ";
    str_stream << "}, ] }";

    std::unique_ptr<blink::OriginManifest> om = Parse(str_stream.str());

    ASSERT_FALSE(om.get() == nullptr);
    std::vector<blink::OriginManifest::ContentSecurityPolicy>
        baseline_fallback = om->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);
    ASSERT_EQ(baseline_fallback.size(), 1ul);

    EXPECT_EQ(baseline_fallback[0].policy, test.expected_policy);
    EXPECT_EQ(baseline_fallback[0].disposition, test.expected_disposition);

    // if override is not allowed it is a fallback and baseline should be empty
    std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline =
        om->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kBaselineOnly);
    EXPECT_EQ(baseline.size(), (test.expected_allow_override) ? 1ul : 0ul);
  }
}

TEST_F(OriginManifestParserTest, GetDirectiveType) {
  DirectiveType type = GetDirectiveType("content-security-policy");
  EXPECT_EQ(type, DirectiveType::kContentSecurityPolicy);

  type = GetDirectiveType("asdf");
  EXPECT_EQ(type, DirectiveType::kUnknown);
}

TEST_F(OriginManifestParserTest, GetCSPDisposition) {
  blink::OriginManifest::ContentSecurityPolicyType type =
      GetCSPDisposition("enforce");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);

  type = GetCSPDisposition("report-only");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kReport);

  type = GetCSPDisposition("");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);

  type = GetCSPDisposition("asdf");
  EXPECT_EQ(type, blink::OriginManifest::ContentSecurityPolicyType::kEnforce);
}

TEST_F(OriginManifestParserTest, GetCSPActivationType) {
  // the function returns the right values
  EXPECT_EQ(GetCSPActivationType(true),
            blink::OriginManifest::ActivationType::kBaseline);
  EXPECT_EQ(GetCSPActivationType(false),
            blink::OriginManifest::ActivationType::kFallback);

  // check different manifest values in JSON
  struct TestCase {
    std::string value;
    unsigned long expected_baseline_only_size;
    unsigned long expected_include_fallbacks_size;
  } tests[]{
      {"true", 1, 1}, {"false", 0, 1}, {"42", 0, 1},
  };

  for (auto test : tests) {
    std::ostringstream str_stream;
    str_stream << "{ \"content-security-policy\": [ {"
               << "    \"allow-override\": " << test.value << ", "
               << "}, ] }";

    std::unique_ptr<blink::OriginManifest> om = Parse(str_stream.str());
    // more details are checked in origin manifest unit tests
    EXPECT_EQ(om->GetContentSecurityPolicies(
                    blink::OriginManifest::FallbackDisposition::kBaselineOnly)
                  .size(),
              test.expected_baseline_only_size);
    EXPECT_EQ(
        om->GetContentSecurityPolicies(
              blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
            .size(),
        test.expected_include_fallbacks_size);
  }
}

TEST_F(OriginManifestParserTest, ParseContentSecurityPolicy) {
  // Wrong structure
  blink::OriginManifest om;
  base::Value dict(base::Value::Type::DICTIONARY);
  ParseContentSecurityPolicy(&om, std::move(dict));
  EXPECT_EQ(om.GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            0ul);

  // empty list
  om = blink::OriginManifest();
  base::Value list(base::Value::Type::LIST);
  ParseContentSecurityPolicy(&om, std::move(list));
  EXPECT_EQ(om.GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            0ul);

  // list with completely unexpected element
  om = blink::OriginManifest();
  list = base::Value(base::Value::Type::LIST);
  list.GetList().push_back(base::Value(42));
  ParseContentSecurityPolicy(&om, std::move(list));
  EXPECT_EQ(om.GetContentSecurityPolicies(
                  blink::OriginManifest::FallbackDisposition::kIncludeFallbacks)
                .size(),
            0ul);

  // list with 1 valid, 2 invalid CSPs -> 3 policies (we don't validate CSPs)
  om = blink::OriginManifest();
  list = base::Value(base::Value::Type::LIST);

  std::string policy0 = "default-src 'none'";
  std::string policy1 = "";
  std::string policy2 = "asdf";

  base::Value csp0(base::Value::Type::DICTIONARY);
  csp0.SetKey("policy", base::Value(policy0));
  csp0.SetKey("disposition", base::Value("report-only"));
  csp0.SetKey("allow-override", base::Value(true));
  list.GetList().push_back(std::move(csp0));

  base::Value csp1(base::Value::Type::DICTIONARY);
  csp1.SetKey("policy", base::Value(policy1));
  csp1.SetKey("disposition", base::Value(23));
  csp1.SetKey("allow-override", base::Value(42));
  list.GetList().push_back(std::move(csp1));

  base::Value csp2(base::Value::Type::DICTIONARY);
  csp2.SetKey("policy", base::Value(policy2));
  list.GetList().push_back(std::move(csp2));

  ParseContentSecurityPolicy(&om, std::move(list));

  std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline =
      om.GetContentSecurityPolicies(
          blink::OriginManifest::FallbackDisposition::kBaselineOnly);
  ASSERT_EQ(baseline.size(), 1ul);
  EXPECT_TRUE(baseline[0].policy == policy0);

  // we know that every CSP s a dictionary and has a unique policy here.
  std::vector<blink::OriginManifest::ContentSecurityPolicy> baseline_fallback =
      om.GetContentSecurityPolicies(
          blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);
  ASSERT_EQ(baseline_fallback.size(), 3ul);
  std::vector<blink::OriginManifest::ContentSecurityPolicy>::iterator it;
  for (it = baseline_fallback.begin(); it != baseline_fallback.end(); ++it) {
    if (it->policy == policy0)
      break;
  }
  ASSERT_FALSE(it == baseline_fallback.end());
  baseline_fallback.erase(it);
  if (baseline_fallback[0].policy == policy1)
    EXPECT_TRUE(baseline_fallback[1].policy == policy2);
  else
    EXPECT_TRUE(baseline_fallback[0].policy == policy1);
}

TEST_F(OriginManifestParserTest, CaseSensitiveKeys) {
  struct TestCase {
    std::string policy;
    std::string expected_equivalent_policy;
  } tests[]{
      {"{ \"Content-Security-Policy\": [{ \"policy\": \"asdf\"}] }", "{}"},
      {"{ \"content-security-policy\": [{ \"PoLiCy\": \"asdf\"}] }",
       "{ \"content-security-policy\": [{}] }"},
      {"{ \"content-security-policy\": [{ \"DiSpOsItIoN\": \"enforce\" }] }",
       "{ \"content-security-policy\": [{}] }"},
      {"{ \"content-security-policy\": [{ \"disposition\": \"EnFoRcE\" }] }",
       "{ \"content-security-policy\": [{}] }"},
      {"{ \"content-security-policy\": [{ \"disposition\": \"RePoRt-OnLy\" }] "
       "}",
       "{ \"content-security-policy\": [{}] }"},
      {"{ \"content-security-policy\": [{ \"AlLoW-OvErRiDe\": true }] }",
       "{ \"content-security-policy\": [{}] }"},
  };

  for (const auto& test : tests) {
    std::unique_ptr<blink::OriginManifest> bad_case = Parse(test.policy);
    std::unique_ptr<blink::OriginManifest> expected =
        Parse(test.expected_equivalent_policy);

    ASSERT_TRUE(bad_case.get() != nullptr);
    ASSERT_TRUE(expected.get() != nullptr);

    std::vector<blink::OriginManifest::ContentSecurityPolicy> bad_case_csps =
        bad_case->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);
    std::vector<blink::OriginManifest::ContentSecurityPolicy> expected_csps =
        expected->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);

    ASSERT_EQ(bad_case_csps.size(), expected_csps.size());

    for (unsigned long i = 0; i < bad_case_csps.size(); ++i) {
      EXPECT_EQ(bad_case_csps[i].policy, expected_csps[i].policy);
      EXPECT_EQ(bad_case_csps[i].disposition, expected_csps[i].disposition);
    }
  }
}

TEST_F(OriginManifestParserTest, MultiDefinitions) {
  struct TestCase {
    std::string policy;
    std::string expected_equivalent_policy;
  } tests[]{
      {"{ \"content-security-policy\": [{ \"policy\": \"first\"}], "
       "  \"content-security-policy\": [{ \"policy\": \"second\"}] }",
       "{ \"content-security-policy\": [{ \"policy\": \"second\"}] }"},

      {"{ \"content-security-policy\": [{ \"policy\": \"first\", \"policy\": "
       "\"second\"}] }",
       "{ \"content-security-policy\": [{ \"policy\": \"second\"}] }"},
  };

  for (const auto& test : tests) {
    std::unique_ptr<blink::OriginManifest> multi = Parse(test.policy);
    std::unique_ptr<blink::OriginManifest> expected =
        Parse(test.expected_equivalent_policy);

    ASSERT_TRUE(multi.get() != nullptr);
    ASSERT_TRUE(expected.get() != nullptr);

    std::vector<blink::OriginManifest::ContentSecurityPolicy> multi_csps =
        multi->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);
    std::vector<blink::OriginManifest::ContentSecurityPolicy> expected_csps =
        expected->GetContentSecurityPolicies(
            blink::OriginManifest::FallbackDisposition::kIncludeFallbacks);

    ASSERT_EQ(multi_csps.size(), expected_csps.size());

    for (unsigned long i = 0; i < multi_csps.size(); ++i) {
      EXPECT_EQ(multi_csps[i].policy, expected_csps[i].policy);
      EXPECT_EQ(multi_csps[i].disposition, expected_csps[i].disposition);
    }
  }
}

}  // namespace origin_manifest_parser
}  // namespace content
