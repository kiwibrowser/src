// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_experiment.h"

#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/mock_entropy_provider.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/cache_type.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace disk_cache {

class SimpleExperimentTest : public testing::Test {
 protected:
  void SetUp() override {
    field_trial_list_ = std::make_unique<base::FieldTrialList>(
        std::make_unique<base::MockEntropyProvider>());
  }
  void ConfigureSizeFieldTrial(bool enabled, base::Optional<uint32_t> param) {
    const std::string kTrialName = "SimpleSizeTrial";
    const std::string kGroupName = "GroupFoo";  // Value not used

    scoped_refptr<base::FieldTrial> trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    if (param) {
      std::map<std::string, std::string> params;
      params[kSizeMultiplierParam] = base::UintToString(param.value());
      base::FieldTrialParamAssociator::GetInstance()->AssociateFieldTrialParams(
          kTrialName, kGroupName, params);
    }

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        kSimpleSizeExperiment.name, base::FeatureList::OVERRIDE_ENABLE_FEATURE,
        trial.get());
    scoped_feature_list_.InitWithFeatureList(std::move(feature_list));
  }

  std::unique_ptr<base::FieldTrialList> field_trial_list_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(SimpleExperimentTest, NoExperiment) {
  SimpleExperiment experiment = GetSimpleExperiment(net::DISK_CACHE);
  EXPECT_EQ(SimpleExperimentType::NONE, experiment.type);
  EXPECT_EQ(0u, experiment.param);
}

TEST_F(SimpleExperimentTest, SizeTrialMissingParam) {
  base::test::ScopedFeatureList scoped_feature_list;
  ConfigureSizeFieldTrial(true, base::Optional<uint32_t>());

  SimpleExperiment experiment = GetSimpleExperiment(net::DISK_CACHE);
  EXPECT_EQ(SimpleExperimentType::NONE, experiment.type);
  EXPECT_EQ(0u, experiment.param);
}

TEST_F(SimpleExperimentTest, SizeTrialProperlyConfigured) {
  const uint32_t kParam = 125u;
  base::test::ScopedFeatureList scoped_feature_list;
  ConfigureSizeFieldTrial(true, base::Optional<uint32_t>(kParam));

  SimpleExperiment experiment = GetSimpleExperiment(net::DISK_CACHE);
  EXPECT_EQ(SimpleExperimentType::SIZE, experiment.type);
  EXPECT_EQ(kParam, experiment.param);
}

TEST_F(SimpleExperimentTest, SizeTrialProperlyConfiguredWrongCacheType) {
  const uint32_t kParam = 125u;
  base::test::ScopedFeatureList scoped_feature_list;
  ConfigureSizeFieldTrial(true, base::Optional<uint32_t>(kParam));

  SimpleExperiment experiment = GetSimpleExperiment(net::APP_CACHE);
  EXPECT_EQ(SimpleExperimentType::NONE, experiment.type);
  EXPECT_EQ(0u, experiment.param);
}

}  // namespace disk_cache
