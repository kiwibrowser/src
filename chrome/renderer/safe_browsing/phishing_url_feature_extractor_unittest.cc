// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/phishing_url_feature_extractor.h"

#include <string>
#include <vector>
#include "chrome/renderer/safe_browsing/features.h"
#include "chrome/renderer/safe_browsing/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ::testing::ElementsAre;

namespace safe_browsing {

class PhishingUrlFeatureExtractorTest : public ::testing::Test {
 protected:
  PhishingUrlFeatureExtractor extractor_;

  void SplitStringIntoLongAlphanumTokens(const std::string& full,
                                         std::vector<std::string>* tokens) {
    PhishingUrlFeatureExtractor::SplitStringIntoLongAlphanumTokens(full,
                                                                   tokens);
  }
};

TEST_F(PhishingUrlFeatureExtractorTest, ExtractFeatures) {
  std::string url = "http://123.0.0.1/mydocuments/a.file.html";
  FeatureMap expected_features;
  expected_features.AddBooleanFeature(features::kUrlHostIsIpAddress);
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("mydocuments"));
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("file"));
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("html"));

  FeatureMap features;
  ASSERT_TRUE(extractor_.ExtractFeatures(GURL(url), &features));
  ExpectFeatureMapsAreEqual(features, expected_features);

  url = "http://www.www.cnn.co.uk/sports/sports/index.html?shouldnotappear";
  expected_features.Clear();
  expected_features.AddBooleanFeature(features::kUrlTldToken +
                                      std::string("co.uk"));
  expected_features.AddBooleanFeature(features::kUrlDomainToken +
                                      std::string("cnn"));
  expected_features.AddBooleanFeature(features::kUrlOtherHostToken +
                                      std::string("www"));
  expected_features.AddBooleanFeature(features::kUrlNumOtherHostTokensGTOne);
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("sports"));
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("index"));
  expected_features.AddBooleanFeature(features::kUrlPathToken +
                                      std::string("html"));

  features.Clear();
  ASSERT_TRUE(extractor_.ExtractFeatures(GURL(url), &features));
  ExpectFeatureMapsAreEqual(features, expected_features);

  url = "http://justadomain.com/";
  expected_features.Clear();
  expected_features.AddBooleanFeature(features::kUrlTldToken +
                                      std::string("com"));
  expected_features.AddBooleanFeature(features::kUrlDomainToken +
                                      std::string("justadomain"));

  features.Clear();
  ASSERT_TRUE(extractor_.ExtractFeatures(GURL(url), &features));
  ExpectFeatureMapsAreEqual(features, expected_features);

  url = "http://witharef.com/#abc";
  expected_features.Clear();
  expected_features.AddBooleanFeature(features::kUrlTldToken +
                                      std::string("com"));
  expected_features.AddBooleanFeature(features::kUrlDomainToken +
                                      std::string("witharef"));

  features.Clear();
  ASSERT_TRUE(extractor_.ExtractFeatures(GURL(url), &features));
  ExpectFeatureMapsAreEqual(features, expected_features);

  url = "http://...www..lotsodots....com./";
  expected_features.Clear();
  expected_features.AddBooleanFeature(features::kUrlTldToken +
                                      std::string("com"));
  expected_features.AddBooleanFeature(features::kUrlDomainToken +
                                      std::string("lotsodots"));
  expected_features.AddBooleanFeature(features::kUrlOtherHostToken +
                                      std::string("www"));

  features.Clear();
  ASSERT_TRUE(extractor_.ExtractFeatures(GURL(url), &features));
  ExpectFeatureMapsAreEqual(features, expected_features);

  url = "http://unrecognized.tld/";
  EXPECT_FALSE(extractor_.ExtractFeatures(GURL(url), &features));

  url = "http://com/123";
  EXPECT_FALSE(extractor_.ExtractFeatures(GURL(url), &features));

  url = "http://.co.uk/";
  EXPECT_FALSE(extractor_.ExtractFeatures(GURL(url), &features));

  url = "file:///nohost.txt";
  EXPECT_FALSE(extractor_.ExtractFeatures(GURL(url), &features));

  url = "not:valid:at:all";
  EXPECT_FALSE(extractor_.ExtractFeatures(GURL(url), &features));
}

TEST_F(PhishingUrlFeatureExtractorTest, SplitStringIntoLongAlphanumTokens) {
  std::string full = "This.is/a_pretty\\unusual-!path,indeed";
  std::vector<std::string> long_tokens;
  SplitStringIntoLongAlphanumTokens(full, &long_tokens);
  EXPECT_THAT(long_tokens,
              ElementsAre("This", "pretty", "unusual", "path", "indeed"));

  long_tokens.clear();
  full = "...i-am_re/al&ly\\b,r,o|k=e:n///up%20";
  SplitStringIntoLongAlphanumTokens(full, &long_tokens);
  EXPECT_THAT(long_tokens, ElementsAre());
}

}  // namespace safe_browsing
