// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/feature_policy/feature_policy.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

// Origin strings used for tests
#define ORIGIN_A "https://example.com/"
#define ORIGIN_B "https://example.net/"
#define ORIGIN_C "https://example.org/"

class GURL;

namespace blink {

namespace {

const char* const kValidPolicies[] = {
    "",      // An empty policy.
    " ",     // An empty policy.
    ";;",    // Empty policies.
    ",,",    // Empty policies.
    " ; ;",  // Empty policies.
    " , ,",  // Empty policies.
    ",;,",   // Empty policies.
    "geolocation 'none'",
    "geolocation 'self'",
    "geolocation 'src'",  // Only valid for iframe allow attribute.
    "geolocation",        // Only valid for iframe allow attribute.
    "geolocation; fullscreen; payment",
    "geolocation *",
    "geolocation " ORIGIN_A "",
    "geolocation " ORIGIN_B "",
    "geolocation  " ORIGIN_A " " ORIGIN_B "",
    "geolocation 'none' " ORIGIN_A " " ORIGIN_B "",
    "geolocation " ORIGIN_A " 'none' " ORIGIN_B "",
    "geolocation 'none' 'none' 'none'",
    "geolocation " ORIGIN_A " *",
    "fullscreen  " ORIGIN_A "; payment 'self'",
    "fullscreen " ORIGIN_A "; payment *, geolocation 'self'"};

const char* const kInvalidPolicies[] = {
    "badfeaturename",
    "badfeaturename 'self'",
    "1.0",
    "geolocation data://badorigin",
    "geolocation https://bad;origin",
    "geolocation https:/bad,origin",
    "geolocation https://example.com, https://a.com",
    "geolocation *, payment data://badorigin",
    "geolocation ws://xn--fd\xbcwsw3taaaaaBaa333aBBBBBBJBBJBBBt"};

}  // namespace

class FeaturePolicyTest : public testing::Test {
 protected:
  FeaturePolicyTest() = default;

  ~FeaturePolicyTest() override = default;

  scoped_refptr<const SecurityOrigin> origin_a_ =
      SecurityOrigin::CreateFromString(ORIGIN_A);
  scoped_refptr<const SecurityOrigin> origin_b_ =
      SecurityOrigin::CreateFromString(ORIGIN_B);
  scoped_refptr<const SecurityOrigin> origin_c_ =
      SecurityOrigin::CreateFromString(ORIGIN_C);

  url::Origin expected_url_origin_a_ = url::Origin::Create(GURL(ORIGIN_A));
  url::Origin expected_url_origin_b_ = url::Origin::Create(GURL(ORIGIN_B));
  url::Origin expected_url_origin_c_ = url::Origin::Create(GURL(ORIGIN_C));

  const FeatureNameMap test_feature_name_map = {
      {"fullscreen", blink::mojom::FeaturePolicyFeature::kFullscreen},
      {"payment", blink::mojom::FeaturePolicyFeature::kPayment},
      {"geolocation", blink::mojom::FeaturePolicyFeature::kGeolocation}};
};

TEST_F(FeaturePolicyTest, ParseValidPolicy) {
  Vector<String> messages;
  for (const char* policy_string : kValidPolicies) {
    messages.clear();
    ParseFeaturePolicy(policy_string, origin_a_.get(), origin_b_.get(),
                       &messages, test_feature_name_map);
    EXPECT_EQ(0UL, messages.size());
  }
}

TEST_F(FeaturePolicyTest, ParseInvalidPolicy) {
  Vector<String> messages;
  for (const char* policy_string : kInvalidPolicies) {
    messages.clear();
    ParseFeaturePolicy(policy_string, origin_a_.get(), origin_b_.get(),
                       &messages, test_feature_name_map);
    EXPECT_NE(0UL, messages.size());
  }
}

TEST_F(FeaturePolicyTest, PolicyParsedCorrectly) {
  Vector<String> messages;

  // Empty policy.
  ParsedFeaturePolicy parsed_policy = ParseFeaturePolicy(
      "", origin_a_.get(), origin_b_.get(), &messages, test_feature_name_map);
  EXPECT_EQ(0UL, parsed_policy.size());

  // Simple policy with 'self'.
  parsed_policy =
      ParseFeaturePolicy("geolocation 'self'", origin_a_.get(), origin_b_.get(),
                         &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[0].origins.size());
  EXPECT_TRUE(
      parsed_policy[0].origins[0].IsSameOriginWith(expected_url_origin_a_));

  // Simple policy with *.
  parsed_policy =
      ParseFeaturePolicy("geolocation *", origin_a_.get(), origin_b_.get(),
                         &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());

  // Complicated policy.
  parsed_policy = ParseFeaturePolicy(
      "geolocation *; "
      "fullscreen https://example.net https://example.org; "
      "payment 'self'",
      origin_a_.get(), origin_b_.get(), &messages, test_feature_name_map);
  EXPECT_EQ(3UL, parsed_policy.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kFullscreen, parsed_policy[1].feature);
  EXPECT_FALSE(parsed_policy[1].matches_all_origins);
  EXPECT_FALSE(parsed_policy[1].matches_opaque_src);
  EXPECT_EQ(2UL, parsed_policy[1].origins.size());
  EXPECT_TRUE(
      parsed_policy[1].origins[0].IsSameOriginWith(expected_url_origin_b_));
  EXPECT_TRUE(
      parsed_policy[1].origins[1].IsSameOriginWith(expected_url_origin_c_));
  EXPECT_EQ(mojom::FeaturePolicyFeature::kPayment, parsed_policy[2].feature);
  EXPECT_FALSE(parsed_policy[2].matches_all_origins);
  EXPECT_FALSE(parsed_policy[2].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[2].origins.size());
  EXPECT_TRUE(
      parsed_policy[2].origins[0].IsSameOriginWith(expected_url_origin_a_));

  // Multiple policies.
  parsed_policy = ParseFeaturePolicy(
      "geolocation * https://example.net; "
      "fullscreen https://example.net none https://example.org,"
      "payment 'self' badorigin",
      origin_a_.get(), origin_b_.get(), &messages, test_feature_name_map);
  EXPECT_EQ(3UL, parsed_policy.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kFullscreen, parsed_policy[1].feature);
  EXPECT_FALSE(parsed_policy[1].matches_all_origins);
  EXPECT_FALSE(parsed_policy[1].matches_opaque_src);
  EXPECT_EQ(2UL, parsed_policy[1].origins.size());
  EXPECT_TRUE(
      parsed_policy[1].origins[0].IsSameOriginWith(expected_url_origin_b_));
  EXPECT_TRUE(
      parsed_policy[1].origins[1].IsSameOriginWith(expected_url_origin_c_));
  EXPECT_EQ(mojom::FeaturePolicyFeature::kPayment, parsed_policy[2].feature);
  EXPECT_FALSE(parsed_policy[2].matches_all_origins);
  EXPECT_FALSE(parsed_policy[2].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[2].origins.size());
  EXPECT_TRUE(
      parsed_policy[2].origins[0].IsSameOriginWith(expected_url_origin_a_));

  // Header policies with no optional origin lists.
  parsed_policy =
      ParseFeaturePolicy("geolocation;fullscreen;payment", origin_a_.get(),
                         nullptr, &messages, test_feature_name_map);
  EXPECT_EQ(3UL, parsed_policy.size());
  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[0].origins.size());
  EXPECT_TRUE(
      parsed_policy[0].origins[0].IsSameOriginWith(expected_url_origin_a_));
  EXPECT_EQ(mojom::FeaturePolicyFeature::kFullscreen, parsed_policy[1].feature);
  EXPECT_FALSE(parsed_policy[1].matches_all_origins);
  EXPECT_FALSE(parsed_policy[1].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[1].origins.size());
  EXPECT_TRUE(
      parsed_policy[1].origins[0].IsSameOriginWith(expected_url_origin_a_));
  EXPECT_EQ(mojom::FeaturePolicyFeature::kPayment, parsed_policy[2].feature);
  EXPECT_FALSE(parsed_policy[2].matches_all_origins);
  EXPECT_FALSE(parsed_policy[2].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[2].origins.size());
  EXPECT_TRUE(
      parsed_policy[2].origins[0].IsSameOriginWith(expected_url_origin_a_));
}

TEST_F(FeaturePolicyTest, PolicyParsedCorrectlyForOpaqueOrigins) {
  Vector<String> messages;

  scoped_refptr<SecurityOrigin> opaque_origin = SecurityOrigin::CreateUnique();

  // Empty policy.
  ParsedFeaturePolicy parsed_policy =
      ParseFeaturePolicy("", origin_a_.get(), opaque_origin.get(), &messages,
                         test_feature_name_map);
  EXPECT_EQ(0UL, parsed_policy.size());

  // Simple policy.
  parsed_policy =
      ParseFeaturePolicy("geolocation", origin_a_.get(), opaque_origin.get(),
                         &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_TRUE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());

  // Simple policy with 'src'.
  parsed_policy =
      ParseFeaturePolicy("geolocation 'src'", origin_a_.get(),
                         opaque_origin.get(), &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_TRUE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());

  // Simple policy with *.
  parsed_policy =
      ParseFeaturePolicy("geolocation *", origin_a_.get(), opaque_origin.get(),
                         &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_TRUE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(0UL, parsed_policy[0].origins.size());

  // Policy with explicit origins
  parsed_policy = ParseFeaturePolicy(
      "geolocation https://example.net https://example.org", origin_a_.get(),
      opaque_origin.get(), &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_FALSE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(2UL, parsed_policy[0].origins.size());
  EXPECT_TRUE(
      parsed_policy[0].origins[0].IsSameOriginWith(expected_url_origin_b_));
  EXPECT_TRUE(
      parsed_policy[0].origins[1].IsSameOriginWith(expected_url_origin_c_));

  // Policy with multiple origins, including 'src'.
  parsed_policy = ParseFeaturePolicy("geolocation https://example.net 'src'",
                                     origin_a_.get(), opaque_origin.get(),
                                     &messages, test_feature_name_map);
  EXPECT_EQ(1UL, parsed_policy.size());

  EXPECT_EQ(mojom::FeaturePolicyFeature::kGeolocation,
            parsed_policy[0].feature);
  EXPECT_FALSE(parsed_policy[0].matches_all_origins);
  EXPECT_TRUE(parsed_policy[0].matches_opaque_src);
  EXPECT_EQ(1UL, parsed_policy[0].origins.size());
  EXPECT_TRUE(
      parsed_policy[0].origins[0].IsSameOriginWith(expected_url_origin_b_));
}

}  // namespace blink
