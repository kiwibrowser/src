// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/memory/ptr_util.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/net/prediction_options.h"
#include "chrome/browser/net/predictor.h"
#include "chrome/browser/predictors/loading_predictor_config.h"
#include "chrome/browser/predictors/resource_prefetch_common.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/variations/variations_params_manager.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

using chrome_browser_net::NetworkPredictionOptions;
using NetPredictor = chrome_browser_net::Predictor;
using net::NetworkChangeNotifier;

namespace {

class MockNetworkChangeNotifierWIFI : public NetworkChangeNotifier {
 public:
  ConnectionType GetCurrentConnectionType() const override {
    return NetworkChangeNotifier::CONNECTION_WIFI;
  }
};

class MockNetworkChangeNotifier4G : public NetworkChangeNotifier {
 public:
  ConnectionType GetCurrentConnectionType() const override {
    return NetworkChangeNotifier::CONNECTION_4G;
  }
};

}  // namespace

namespace predictors {

class LoadingPredictorConfigTest : public testing::Test {
 public:
  LoadingPredictorConfigTest();

  void SetPreference(NetworkPredictionOptions value) {
    profile_->GetPrefs()->SetInteger(prefs::kNetworkPredictionOptions, value);
  }

  bool IsNetPredictorEnabled() {
    std::unique_ptr<NetPredictor> predictor = base::WrapUnique(
        NetPredictor::CreatePredictor(true /* simple_shutdown */));
    bool is_enabled = predictor->PredictorEnabled();
    predictor->Shutdown();
    return is_enabled;
  }

 protected:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
};

LoadingPredictorConfigTest::LoadingPredictorConfigTest()
    : profile_(new TestingProfile()) {}

TEST_F(LoadingPredictorConfigTest, Enabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(predictors::kSpeculativePreconnectFeature);

  LoadingPredictorConfig config;
  EXPECT_TRUE(MaybeEnableSpeculativePreconnect(&config));

  EXPECT_TRUE(config.IsLearningEnabled());
  EXPECT_TRUE(config.should_disable_other_preconnects);
  EXPECT_FALSE(IsNetPredictorEnabled());
  EXPECT_TRUE(config.IsPreconnectEnabledForSomeOrigin(profile_.get()));
}

TEST_F(LoadingPredictorConfigTest, Disabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(predictors::kSpeculativePreconnectFeature);

  LoadingPredictorConfig config;
  EXPECT_FALSE(MaybeEnableSpeculativePreconnect(&config));

  EXPECT_FALSE(config.IsLearningEnabled());
  EXPECT_FALSE(config.should_disable_other_preconnects);
  EXPECT_TRUE(IsNetPredictorEnabled());
  EXPECT_FALSE(config.IsPreconnectEnabledForSomeOrigin(profile_.get()));
}

TEST_F(LoadingPredictorConfigTest, EnablePreconnectLearning) {
  variations::testing::VariationParamsManager params_manager(
      "dummy-trial", {{kModeParamName, kLearningMode}},
      {kSpeculativePreconnectFeatureName});

  LoadingPredictorConfig config;
  EXPECT_TRUE(MaybeEnableSpeculativePreconnect(&config));

  EXPECT_TRUE(config.IsLearningEnabled());
  EXPECT_TRUE(config.is_origin_learning_enabled);
  EXPECT_FALSE(config.should_disable_other_preconnects);
  EXPECT_TRUE(IsNetPredictorEnabled());
  EXPECT_FALSE(config.IsPreconnectEnabledForSomeOrigin(profile_.get()));
}

TEST_F(LoadingPredictorConfigTest, EnablePreconnect) {
  variations::testing::VariationParamsManager params_manager(
      "dummy-trial", {{kModeParamName, kPreconnectMode}},
      {kSpeculativePreconnectFeatureName});

  LoadingPredictorConfig config;
  EXPECT_TRUE(MaybeEnableSpeculativePreconnect(&config));

  EXPECT_TRUE(config.IsLearningEnabled());
  EXPECT_TRUE(config.should_disable_other_preconnects);
  EXPECT_FALSE(IsNetPredictorEnabled());
  EXPECT_TRUE(config.IsPreconnectEnabledForSomeOrigin(profile_.get()));
}

TEST_F(LoadingPredictorConfigTest, EnableNoPreconnect) {
  variations::testing::VariationParamsManager params_manager(
      "dummy-trial", {{kModeParamName, kNoPreconnectMode}},
      {kSpeculativePreconnectFeatureName});

  LoadingPredictorConfig config;
  EXPECT_FALSE(MaybeEnableSpeculativePreconnect(&config));

  EXPECT_FALSE(config.IsLearningEnabled());
  EXPECT_TRUE(config.should_disable_other_preconnects);
  EXPECT_FALSE(IsNetPredictorEnabled());
  EXPECT_FALSE(config.IsPreconnectEnabledForSomeOrigin(profile_.get()));
}

}  // namespace predictors
