// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/origin_manifest/origin_manifest.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
struct TestCase {
  std::string policy;
  OriginManifest::ContentSecurityPolicyType disposition;
  OriginManifest::ActivationType activation_type;
  unsigned long expected_baseline_only_size;
  unsigned long expected_include_fallback_size;
} const tests[]{{"policy", OriginManifest::ContentSecurityPolicyType::kReport,
                 OriginManifest::ActivationType::kFallback, 0, 1},
                {"policy", OriginManifest::ContentSecurityPolicyType::kReport,
                 OriginManifest::ActivationType::kBaseline, 1, 1},
                {"policy", OriginManifest::ContentSecurityPolicyType::kEnforce,
                 OriginManifest::ActivationType::kFallback, 0, 1},
                {"policy", OriginManifest::ContentSecurityPolicyType::kEnforce,
                 OriginManifest::ActivationType::kBaseline, 1, 1}};
}  // namespace

class OriginManifestTest : public testing::Test {};

TEST_F(OriginManifestTest, AddAndGetSingleCSPs) {
  // Add all possible configurations and check their existence
  for (const auto& test : tests) {
    OriginManifest om;
    om.AddContentSecurityPolicy(test.policy, test.disposition,
                                test.activation_type);

    const std::vector<OriginManifest::ContentSecurityPolicy> baseline_only =
        om.GetContentSecurityPolicies(
            OriginManifest::FallbackDisposition::kBaselineOnly);
    const std::vector<OriginManifest::ContentSecurityPolicy> include_fallbacks =
        om.GetContentSecurityPolicies(
            OriginManifest::FallbackDisposition::kIncludeFallbacks);

    ASSERT_EQ(baseline_only.size(), test.expected_baseline_only_size);
    if (test.expected_baseline_only_size > 0) {
      EXPECT_EQ(baseline_only[0].policy, test.policy);
      EXPECT_EQ(baseline_only[0].disposition, test.disposition);
    }

    ASSERT_EQ(include_fallbacks.size(), test.expected_include_fallback_size);
    if (test.expected_include_fallback_size > 0) {
      EXPECT_EQ(include_fallbacks[0].policy, test.policy);
      EXPECT_EQ(include_fallbacks[0].disposition, test.disposition);
    }
  }
}

TEST_F(OriginManifestTest, AddAndGetMultipleCSPs) {
  unsigned long expected_final_baseline_only_size = 0;
  unsigned long expected_final_include_fallback_size = 0;

  // You can add arbitrarily many CSPs despite having the same policy value
  OriginManifest om;

  for (const auto& test : tests) {
    om.AddContentSecurityPolicy(test.policy, test.disposition,
                                test.activation_type);
    expected_final_baseline_only_size += test.expected_baseline_only_size;
    expected_final_include_fallback_size += test.expected_include_fallback_size;
  }
  const std::vector<OriginManifest::ContentSecurityPolicy> baseline_only =
      om.GetContentSecurityPolicies(
          OriginManifest::FallbackDisposition::kBaselineOnly);
  const std::vector<OriginManifest::ContentSecurityPolicy> include_fallbacks =
      om.GetContentSecurityPolicies(
          OriginManifest::FallbackDisposition::kIncludeFallbacks);

  EXPECT_EQ(baseline_only.size(), expected_final_baseline_only_size);
  EXPECT_EQ(include_fallbacks.size(), expected_final_include_fallback_size);
}

}  // namespace blink
