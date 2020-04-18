// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/scorer.h"

#include <stdint.h>

#include <memory>

#include "base/containers/hash_tables.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/format_macros.h"
#include "base/threading/thread.h"
#include "chrome/common/safe_browsing/client_model.pb.h"
#include "chrome/renderer/safe_browsing/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace safe_browsing {

class PhishingScorerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Setup a simple model.  Note that the scorer does not care about
    // how features are encoded so we use readable strings here to make
    // the test simpler to follow.
    model_.Clear();
    model_.add_hashes("feature1");
    model_.add_hashes("feature2");
    model_.add_hashes("feature3");
    model_.add_hashes("token one");
    model_.add_hashes("token two");

    ClientSideModel::Rule* rule;
    rule = model_.add_rule();
    rule->set_weight(0.5);

    rule = model_.add_rule();
    rule->add_feature(0);  // feature1
    rule->set_weight(2.0);

    rule = model_.add_rule();
    rule->add_feature(0);  // feature1
    rule->add_feature(1);  // feature2
    rule->set_weight(3.0);

    model_.add_page_term(3);  // token one
    model_.add_page_term(4);  // token two

    // These will be murmur3 hashes, but for this test it's not necessary
    // that the hashes correspond to actual words.
    model_.add_page_word(1000U);
    model_.add_page_word(2000U);
    model_.add_page_word(3000U);

    model_.set_max_words_per_term(2);
    model_.set_murmur_hash_seed(12345U);
    model_.set_max_shingles_per_page(10);
    model_.set_shingle_size(3);
  }

  ClientSideModel model_;
};

TEST_F(PhishingScorerTest, HasValidModel) {
  std::unique_ptr<Scorer> scorer;
  scorer.reset(Scorer::Create(model_.SerializeAsString()));
  EXPECT_TRUE(scorer.get() != NULL);

  // Invalid model string.
  scorer.reset(Scorer::Create("bogus string"));
  EXPECT_FALSE(scorer.get());

  // Mode is missing a required field.
  model_.clear_max_words_per_term();
  scorer.reset(Scorer::Create(model_.SerializePartialAsString()));
  EXPECT_FALSE(scorer.get());
}

TEST_F(PhishingScorerTest, PageTerms) {
  std::unique_ptr<Scorer> scorer(Scorer::Create(model_.SerializeAsString()));
  ASSERT_TRUE(scorer.get());

  // Use std::vector instead of base::hash_set for comparison.
  // On Android, EXPECT_THAT(..., ContainerEq(...)) doesn't support
  // std::hash_set, but std::vector works fine.
  std::vector<std::string> expected_page_terms;
  expected_page_terms.push_back("token one");
  expected_page_terms.push_back("token two");
  std::sort(expected_page_terms.begin(), expected_page_terms.end());

  base::hash_set<std::string> page_terms = scorer->page_terms();
  std::vector<std::string> page_terms_v(page_terms.begin(), page_terms.end());
  std::sort(page_terms_v.begin(), page_terms_v.end());

  EXPECT_THAT(page_terms_v, ::testing::ContainerEq(expected_page_terms));
}

TEST_F(PhishingScorerTest, PageWords) {
  std::unique_ptr<Scorer> scorer(Scorer::Create(model_.SerializeAsString()));
  ASSERT_TRUE(scorer.get());
  std::vector<uint32_t> expected_page_words;
  expected_page_words.push_back(1000U);
  expected_page_words.push_back(2000U);
  expected_page_words.push_back(3000U);
  std::sort(expected_page_words.begin(), expected_page_words.end());

  base::hash_set<uint32_t> page_words = scorer->page_words();
  std::vector<uint32_t> page_words_v(page_words.begin(), page_words.end());
  std::sort(page_words_v.begin(), page_words_v.end());

  EXPECT_THAT(page_words_v, ::testing::ContainerEq(expected_page_words));

  EXPECT_EQ(2U, scorer->max_words_per_term());
  EXPECT_EQ(12345U, scorer->murmurhash3_seed());
  EXPECT_EQ(10U, scorer->max_shingles_per_page());
  EXPECT_EQ(3U, scorer->shingle_size());
}

TEST_F(PhishingScorerTest, ComputeScore) {
  std::unique_ptr<Scorer> scorer(Scorer::Create(model_.SerializeAsString()));
  ASSERT_TRUE(scorer.get());

  // An empty feature map should match the empty rule.
  FeatureMap features;
  // The expected logodds is 0.5 (empty rule) => p = exp(0.5) / (exp(0.5) + 1)
  // => 0.62245933120185459
  EXPECT_DOUBLE_EQ(0.62245933120185459, scorer->ComputeScore(features));
  // Same if the feature does not match any rule.
  EXPECT_TRUE(features.AddBooleanFeature("not existing feature"));
  EXPECT_DOUBLE_EQ(0.62245933120185459, scorer->ComputeScore(features));

  // Feature 1 matches which means that the logodds will be:
  //   0.5 (empty rule) + 2.0 (rule weight) * 0.15 (feature weight) = 0.8
  //   => p = 0.6899744811276125
  EXPECT_TRUE(features.AddRealFeature("feature1", 0.15));
  EXPECT_DOUBLE_EQ(0.6899744811276125, scorer->ComputeScore(features));

  // Now, both feature 1 and feature 2 match.  Expected logodds:
  //   0.5 (empty rule) + 2.0 (rule weight) * 0.15 (feature weight) +
  //   3.0 (rule weight) * 0.15 (feature1 weight) * 1.0 (feature2) weight = 9.8
  //   => p = 0.99999627336071584
  EXPECT_TRUE(features.AddBooleanFeature("feature2"));
  EXPECT_DOUBLE_EQ(0.77729986117469119, scorer->ComputeScore(features));
}
}  // namespace safe_browsing
