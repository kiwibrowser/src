// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/previews/previews_service.h"

#include <initializer_list>
#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_params.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_features.h"
#include "components/previews/content/previews_io_data.h"
#include "components/previews/content/previews_optimization_guide.h"
#include "components/previews/content/previews_ui_service.h"
#include "components/previews/core/previews_features.h"
#include "components/variations/variations_associated_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// This test class intercepts the |is_enabled_callback| and is used to test its
// validity.
class TestPreviewsIOData : public previews::PreviewsIOData {
 public:
  TestPreviewsIOData()
      : previews::PreviewsIOData(content::BrowserThread::GetTaskRunnerForThread(
                                     content::BrowserThread::UI),
                                 content::BrowserThread::GetTaskRunnerForThread(
                                     content::BrowserThread::UI)) {}
  ~TestPreviewsIOData() override {}

  void Initialize(
      base::WeakPtr<previews::PreviewsUIService> previews_ui_service,
      std::unique_ptr<previews::PreviewsOptOutStore> previews_opt_out_store,
      std::unique_ptr<previews::PreviewsOptimizationGuide> previews_opt_guide,
      const previews::PreviewsIsEnabledCallback& is_enabled_callback) override {
    enabled_callback_ = is_enabled_callback;
  }

  bool IsPreviewEnabled(previews::PreviewsType type) {
    return enabled_callback_.Run(type);
  }

 private:
  previews::PreviewsIsEnabledCallback enabled_callback_;
};

// Class to test the validity of the callback passed to PreviewsIOData from
// PreviewsService.
class PreviewsServiceTest : public testing::Test {
 public:
  PreviewsServiceTest()

      : field_trial_list_(nullptr), scoped_feature_list_() {}

  void SetUp() override {
    io_data_ = std::make_unique<TestPreviewsIOData>();

    service_ = std::make_unique<PreviewsService>();
    base::FilePath file_path;
    service_->Initialize(io_data_.get(),
                         nullptr /* optimization_guide_service */,
                         content::BrowserThread::GetTaskRunnerForThread(
                             content::BrowserThread::UI),
                         file_path);
    scoped_feature_list_.InitWithFeatures(
        {previews::features::kPreviews},
        {data_reduction_proxy::features::kDataReductionProxyDecidesTransform});
  }

  void TearDown() override { variations::testing::ClearAllVariationParams(); }

  ~PreviewsServiceTest() override {}

  TestPreviewsIOData* io_data() const { return io_data_.get(); }

 private:
  content::TestBrowserThreadBundle threads_;
  base::FieldTrialList field_trial_list_;
  std::unique_ptr<TestPreviewsIOData> io_data_;
  std::unique_ptr<PreviewsService> service_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

}  // namespace

TEST_F(PreviewsServiceTest, TestOfflineFieldTrialNotSet) {
  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::OFFLINE));
}

TEST_F(PreviewsServiceTest, TestOfflineFeatureDisabled) {
  std::unique_ptr<base::FeatureList> feature_list =
      std::make_unique<base::FeatureList>();

  // The feature is explicitly enabled on the command-line.
  feature_list->InitializeFromCommandLine("", "OfflinePreviews");
  base::FeatureList::ClearInstanceForTesting();
  base::FeatureList::SetInstance(std::move(feature_list));

  EXPECT_FALSE(io_data()->IsPreviewEnabled(previews::PreviewsType::OFFLINE));
  base::FeatureList::ClearInstanceForTesting();
}

TEST_F(PreviewsServiceTest, TestClientLoFiFeatureEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews,
       previews::features::kClientLoFi} /* enabled features */,
      {data_reduction_proxy::features::
           kDataReductionProxyDecidesTransform} /* disabled features */);

  base::FieldTrialList::CreateFieldTrial("PreviewsClientLoFi", "Enabled");
  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::LOFI));
}

TEST_F(PreviewsServiceTest, TestClientLoFiAndServerLoFiEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews, previews::features::kClientLoFi,
       data_reduction_proxy::features::
           kDataReductionProxyDecidesTransform} /* enabled features */,
      {} /* disabled features */);

  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::LOFI));
}

TEST_F(PreviewsServiceTest, TestClientLoFiAndServerLoFiNotEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews} /* enabled features */,
      {previews::features::kClientLoFi,
       data_reduction_proxy::features::
           kDataReductionProxyDecidesTransform} /* disabled features */);
  EXPECT_FALSE(io_data()->IsPreviewEnabled(previews::PreviewsType::LOFI));
}

TEST_F(PreviewsServiceTest, TestLitePageNotEnabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews} /* enabled features */,
      {data_reduction_proxy::features::
           kDataReductionProxyDecidesTransform} /* disabled features */);
  EXPECT_FALSE(io_data()->IsPreviewEnabled(previews::PreviewsType::LITE_PAGE));
}

TEST_F(PreviewsServiceTest, TestServerLoFiProxyDecidesTransform) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews,
       data_reduction_proxy::features::kDataReductionProxyDecidesTransform},
      {});
  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::LOFI));
}

TEST_F(PreviewsServiceTest, TestLitePageProxyDecidesTransform) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeatures(
      {previews::features::kPreviews,
       data_reduction_proxy::features::kDataReductionProxyDecidesTransform},
      {});
  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::LITE_PAGE));
}

TEST_F(PreviewsServiceTest, TestNoScriptPreviewsEnabledByFeature) {
  EXPECT_FALSE(io_data()->IsPreviewEnabled(previews::PreviewsType::NOSCRIPT));
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      previews::features::kNoScriptPreviews);
  EXPECT_TRUE(io_data()->IsPreviewEnabled(previews::PreviewsType::NOSCRIPT));
}
