// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data_use_observer.h"

#include <map>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_test_utils.h"
#include "components/data_use_measurement/core/data_use.h"
#include "components/data_use_measurement/core/data_use_ascriber.h"
#include "components/data_use_measurement/core/data_use_recorder.h"
#include "components/data_use_measurement/core/url_request_classifier.h"
#include "components/previews/core/previews_experiments.h"
#include "components/previews/core/previews_features.h"
#include "components/previews/core/previews_user_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

namespace {

class MockDataUseAscriber : public data_use_measurement::DataUseAscriber {
 public:
  MOCK_METHOD1(
      GetOrCreateDataUseRecorder,
      data_use_measurement::DataUseRecorder*(net::URLRequest* request));
  MOCK_METHOD1(
      GetDataUseRecorder,
      data_use_measurement::DataUseRecorder*(const net::URLRequest& request));
  MOCK_CONST_METHOD0(
      CreateURLRequestClassifier,
      std::unique_ptr<data_use_measurement::URLRequestClassifier>());
};

const int kInflationPercent = 50;
const int kInflationBytes = 1000;

}  // namespace

namespace data_reduction_proxy {

class DataReductionProxyDataUseObserverTest : public testing::Test {
 public:
  DataReductionProxyDataUseObserverTest() {}

  void SetUp() override {
    std::map<std::string, std::string> parameters = {
        {"NoScriptInflationPercent", base::IntToString(kInflationPercent)},
        {"NoScriptInflationBytes", base::IntToString(kInflationBytes)}};
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        previews::features::kNoScriptPreviews, parameters);
    test_context_ = DataReductionProxyTestContext::Builder()
                        .WithConfigClient()
                        .WithMockDataReductionProxyService()
                        .Build();
    ascriber_ = std::make_unique<MockDataUseAscriber>();
    data_use_observer_ = std::make_unique<DataReductionProxyDataUseObserver>(
        test_context_->io_data(), ascriber_.get());
  }

  MockDataReductionProxyService* mock_drp_service() {
    return test_context_->mock_data_reduction_proxy_service();
  }

  void SetPreviewsUserData(data_use_measurement::DataUse* data_use,
                           previews::PreviewsUserData* previews_user_data) {
    data_use->SetUserData(
        data_use_observer_->GetDataUsePreviewsUserDataKeyForTesting(),
        previews_user_data->DeepCopy());
  }

  void DidFinishLoad(data_use_measurement::DataUse* data_use) {
    data_use_observer_->OnPageDidFinishLoad(data_use);
  }

  void RunUntilIdle() { test_context_->RunUntilIdle(); }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  base::MessageLoopForIO message_loop_;
  std::unique_ptr<DataReductionProxyTestContext> test_context_;
  std::unique_ptr<MockDataUseAscriber> ascriber_;
  std::unique_ptr<DataReductionProxyDataUseObserver> data_use_observer_;

  DISALLOW_COPY_AND_ASSIGN(DataReductionProxyDataUseObserverTest);
};

TEST_F(DataReductionProxyDataUseObserverTest,
       OnPageDidFinishLoadNotNoScriptPreview) {
  std::unique_ptr<data_use_measurement::DataUse> data_use =
      std::make_unique<data_use_measurement::DataUse>(
          data_use_measurement::DataUse::TrafficType::USER_TRAFFIC);
  data_use->IncrementTotalBytes(500000, 300);
  data_use->set_url(GURL("https://testsite.com"));

  // Verify with no committed preview.
  previews::PreviewsUserData previews_user_data(3 /* page_id */);
  SetPreviewsUserData(data_use.get(), &previews_user_data);
  EXPECT_CALL(*mock_drp_service(), UpdateContentLengths(_, _, _, _, _))
      .Times(0);
  EXPECT_CALL(*mock_drp_service(), UpdateDataUseForHost(_, _, _)).Times(0);
  DidFinishLoad(data_use.get());
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(mock_drp_service());

  // Now verify with LOFI as committed preview.
  previews_user_data.SetCommittedPreviewsType(previews::PreviewsType::LOFI);
  SetPreviewsUserData(data_use.get(), &previews_user_data);
  EXPECT_CALL(*mock_drp_service(), UpdateContentLengths(_, _, _, _, _))
      .Times(0);
  EXPECT_CALL(*mock_drp_service(), UpdateDataUseForHost(_, _, _)).Times(0);
  DidFinishLoad(data_use.get());
  RunUntilIdle();
}

TEST_F(DataReductionProxyDataUseObserverTest,
       OnPageDidFinishLoadHasCommittedNoScriptPreview) {
  std::unique_ptr<data_use_measurement::DataUse> data_use =
      std::make_unique<data_use_measurement::DataUse>(
          data_use_measurement::DataUse::TrafficType::USER_TRAFFIC);
  data_use->IncrementTotalBytes(500000, 300);
  data_use->set_url(GURL("https://testsite.com"));
  previews::PreviewsUserData previews_user_data(7 /* page_id */);
  previews_user_data.SetCommittedPreviewsType(previews::PreviewsType::NOSCRIPT);
  SetPreviewsUserData(data_use.get(), &previews_user_data);
  int inflation_value = 500000 * kInflationPercent / 100 + kInflationBytes;
  EXPECT_EQ(251000, inflation_value);
  EXPECT_CALL(*mock_drp_service(),
              UpdateContentLengths(0, inflation_value, true, _, _))
      .Times(1);
  EXPECT_CALL(*mock_drp_service(),
              UpdateDataUseForHost(0, inflation_value, "testsite.com"))
      .Times(1);
  DidFinishLoad(data_use.get());
  RunUntilIdle();
}

TEST_F(DataReductionProxyDataUseObserverTest,
       OnPageDidFinishLoadHasCommittedNoScriptPreviewWithInflationPercent) {
  std::unique_ptr<data_use_measurement::DataUse> data_use =
      std::make_unique<data_use_measurement::DataUse>(
          data_use_measurement::DataUse::TrafficType::USER_TRAFFIC);
  data_use->IncrementTotalBytes(500000, 300);
  data_use->set_url(GURL("https://testsite.com"));
  previews::PreviewsUserData previews_user_data(7 /* page_id */);
  previews_user_data.SetDataSavingsInflationPercent(80);
  previews_user_data.SetCommittedPreviewsType(previews::PreviewsType::NOSCRIPT);
  SetPreviewsUserData(data_use.get(), &previews_user_data);
  int inflation_value = 500000 * 80 / 100;
  EXPECT_EQ(400000, inflation_value);
  EXPECT_CALL(*mock_drp_service(),
              UpdateContentLengths(0, inflation_value, true, _, _))
      .Times(1);
  EXPECT_CALL(*mock_drp_service(),
              UpdateDataUseForHost(0, inflation_value, "testsite.com"))
      .Times(1);
  DidFinishLoad(data_use.get());
  RunUntilIdle();
}

}  // namespace data_reduction_proxy
