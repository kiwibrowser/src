// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/loading_stats_collector.h"

#include <vector>

#include "base/test/histogram_tester.h"
#include "chrome/browser/predictors/loading_test_util.h"
#include "chrome/browser/predictors/preconnect_manager.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;

namespace predictors {

namespace {
const char kInitialUrl[] = "http://www.google.com/cats";
const char kRedirectedUrl[] = "http://www.google.fr/chats";
const char kRedirectedUrl2[] = "http://www.google.de/katzen";
}

using RedirectStatus = ResourcePrefetchPredictor::RedirectStatus;

class LoadingStatsCollectorTest : public testing::Test {
 public:
  LoadingStatsCollectorTest();
  ~LoadingStatsCollectorTest() override;
  void SetUp() override;

  void TestRedirectStatusHistogram(const std::string& initial_url,
                                   const std::string& prediction_url,
                                   const std::string& navigation_url,
                                   RedirectStatus expected_status);

 protected:
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<StrictMock<MockResourcePrefetchPredictor>> mock_predictor_;
  std::unique_ptr<LoadingStatsCollector> stats_collector_;
  std::unique_ptr<base::HistogramTester> histogram_tester_;
};

LoadingStatsCollectorTest::LoadingStatsCollectorTest()
    : profile_(std::make_unique<TestingProfile>()) {}

LoadingStatsCollectorTest::~LoadingStatsCollectorTest() = default;

void LoadingStatsCollectorTest::SetUp() {
  LoadingPredictorConfig config;
  PopulateTestConfig(&config);
  profile_ = std::make_unique<TestingProfile>();
  mock_predictor_ = std::make_unique<StrictMock<MockResourcePrefetchPredictor>>(
      config, profile_.get());
  stats_collector_ =
      std::make_unique<LoadingStatsCollector>(mock_predictor_.get(), config);
  histogram_tester_ = std::make_unique<base::HistogramTester>();
  content::RunAllTasksUntilIdle();
}

void LoadingStatsCollectorTest::TestRedirectStatusHistogram(
    const std::string& initial_url,
    const std::string& prediction_url,
    const std::string& navigation_url,
    RedirectStatus expected_status) {
  // Prediction setting.
  // We need at least one resource for prediction.
  const std::string& script_url = "https://cdn.google.com/script.js";
  PreconnectPrediction prediction = CreatePreconnectPrediction(
      GURL(prediction_url).host(), initial_url != prediction_url,
      {{GURL(script_url).GetOrigin(), 1}});
  EXPECT_CALL(*mock_predictor_, PredictPreconnectOrigins(GURL(initial_url), _))
      .WillOnce(DoAll(SetArgPointee<1>(prediction), Return(true)));

  // Navigation simulation.
  URLRequestSummary script =
      CreateURLRequestSummary(SessionID::FromSerializedValue(1), navigation_url,
                              script_url, content::RESOURCE_TYPE_SCRIPT);
  PageRequestSummary summary =
      CreatePageRequestSummary(navigation_url, initial_url, {script});

  stats_collector_->RecordPageRequestSummary(summary);

  // Histogram check.
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectLearningRedirectStatus,
      static_cast<int>(expected_status), 1);
}

TEST_F(LoadingStatsCollectorTest, TestPreconnectPrecisionRecallHistograms) {
  const std::string main_frame_url = "http://google.com/?query=cats";
  auto gen = [](int index) {
    return base::StringPrintf("http://cdn%d.google.com/script.js", index);
  };

  // Predicts 4 origins: 2 useful, 2 useless.
  PreconnectPrediction prediction =
      CreatePreconnectPrediction(GURL(main_frame_url).host(), false,
                                 {{GURL(main_frame_url).GetOrigin(), 1},
                                  {GURL(gen(1)).GetOrigin(), 1},
                                  {GURL(gen(2)).GetOrigin(), 1},
                                  {GURL(gen(3)).GetOrigin(), 0}});
  EXPECT_CALL(*mock_predictor_,
              PredictPreconnectOrigins(GURL(main_frame_url), _))
      .WillOnce(DoAll(SetArgPointee<1>(prediction), Return(true)));

  // Simulate a page load with 2 resources, one we know, one we don't, plus we
  // know the main frame origin.
  URLRequestSummary script =
      CreateURLRequestSummary(SessionID::FromSerializedValue(1), main_frame_url,
                              gen(1), content::RESOURCE_TYPE_SCRIPT);
  URLRequestSummary new_script =
      CreateURLRequestSummary(SessionID::FromSerializedValue(1), main_frame_url,
                              gen(100), content::RESOURCE_TYPE_SCRIPT);
  PageRequestSummary summary = CreatePageRequestSummary(
      main_frame_url, main_frame_url, {script, new_script});

  stats_collector_->RecordPageRequestSummary(summary);

  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectLearningRecall, 66, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectLearningPrecision, 50, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectLearningCount, 4, 1);
}

TEST_F(LoadingStatsCollectorTest, TestRedirectStatusNoRedirect) {
  TestRedirectStatusHistogram(kInitialUrl, kInitialUrl, kInitialUrl,
                              RedirectStatus::NO_REDIRECT);
}

TEST_F(LoadingStatsCollectorTest, TestRedirectStatusNoRedirectButPredicted) {
  TestRedirectStatusHistogram(kInitialUrl, kRedirectedUrl, kInitialUrl,
                              RedirectStatus::NO_REDIRECT_BUT_PREDICTED);
}

TEST_F(LoadingStatsCollectorTest, TestRedirectStatusRedirectNotPredicted) {
  TestRedirectStatusHistogram(kInitialUrl, kInitialUrl, kRedirectedUrl,
                              RedirectStatus::REDIRECT_NOT_PREDICTED);
}

TEST_F(LoadingStatsCollectorTest, TestRedirectStatusRedirectWrongPredicted) {
  TestRedirectStatusHistogram(kInitialUrl, kRedirectedUrl, kRedirectedUrl2,
                              RedirectStatus::REDIRECT_WRONG_PREDICTED);
}

TEST_F(LoadingStatsCollectorTest,
       TestRedirectStatusRedirectCorrectlyPredicted) {
  TestRedirectStatusHistogram(kInitialUrl, kRedirectedUrl, kRedirectedUrl,
                              RedirectStatus::REDIRECT_CORRECTLY_PREDICTED);
}

TEST_F(LoadingStatsCollectorTest, TestPreconnectHistograms) {
  const std::string main_frame_url("http://google.com/?query=cats");
  const SessionID kTabId = SessionID::FromSerializedValue(1);
  auto gen = [](int index) {
    return base::StringPrintf("http://cdn%d.google.com/script.js", index);
  };
  EXPECT_CALL(*mock_predictor_,
              PredictPreconnectOrigins(GURL(main_frame_url), _))
      .WillOnce(Return(false));

  {
    // Initialize PreconnectStats.

    // These two are hits.
    PreconnectedRequestStats origin1(GURL(gen(1)).GetOrigin(), false, true);
    PreconnectedRequestStats origin2(GURL(gen(2)).GetOrigin(), true, false);
    // And these two are misses.
    PreconnectedRequestStats origin3(GURL(gen(3)).GetOrigin(), false, false);
    PreconnectedRequestStats origin4(GURL(gen(4)).GetOrigin(), true, true);

    auto stats = std::make_unique<PreconnectStats>(GURL(main_frame_url));
    stats->requests_stats = {origin1, origin2, origin3, origin4};

    stats_collector_->RecordPreconnectStats(std::move(stats));
  }

  {
    // Simulate a page load with 3 origins.
    URLRequestSummary script1 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(1), content::RESOURCE_TYPE_SCRIPT);
    URLRequestSummary script2 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(2), content::RESOURCE_TYPE_SCRIPT);
    URLRequestSummary script100 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(100), content::RESOURCE_TYPE_SCRIPT);
    PageRequestSummary summary = CreatePageRequestSummary(
        main_frame_url, main_frame_url, {script1, script2, script100});

    stats_collector_->RecordPageRequestSummary(summary);
  }

  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreresolveHitsPercentage, 50, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectHitsPercentage, 50, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreresolveCount, 4, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectCount, 2, 1);
}

// Tests that preconnect histograms won't be recorded if preconnect stats are
// empty.
TEST_F(LoadingStatsCollectorTest, TestPreconnectHistogramsEmpty) {
  const std::string main_frame_url = "http://google.com";
  const SessionID kTabId = SessionID::FromSerializedValue(1);
  auto stats = std::make_unique<PreconnectStats>(GURL(main_frame_url));
  stats_collector_->RecordPreconnectStats(std::move(stats));

  EXPECT_CALL(*mock_predictor_,
              PredictPreconnectOrigins(GURL(main_frame_url), _))
      .WillOnce(Return(false));

  URLRequestSummary script = CreateURLRequestSummary(
      kTabId, main_frame_url, "http://cdn.google.com/script.js",
      content::RESOURCE_TYPE_SCRIPT);
  PageRequestSummary summary =
      CreatePageRequestSummary(main_frame_url, main_frame_url, {script});
  stats_collector_->RecordPageRequestSummary(summary);

  // No histograms should be recorded.
  histogram_tester_->ExpectTotalCount(
      internal::kLoadingPredictorPreresolveHitsPercentage, 0);
  histogram_tester_->ExpectTotalCount(
      internal::kLoadingPredictorPreconnectHitsPercentage, 0);
  histogram_tester_->ExpectTotalCount(
      internal::kLoadingPredictorPreresolveCount, 0);
  histogram_tester_->ExpectTotalCount(
      internal::kLoadingPredictorPreconnectCount, 0);
}

// Tests that the preconnect won't divide by zero if preconnect stats contain
// preresolve attempts only.
TEST_F(LoadingStatsCollectorTest, TestPreconnectHistogramsPreresolvesOnly) {
  const std::string main_frame_url("http://google.com/?query=cats");
  const SessionID kTabId = SessionID::FromSerializedValue(1);
  auto gen = [](int index) {
    return base::StringPrintf("http://cdn%d.google.com/script.js", index);
  };
  EXPECT_CALL(*mock_predictor_,
              PredictPreconnectOrigins(GURL(main_frame_url), _))
      .WillOnce(Return(false));

  {
    // Initialize PreconnectStats.

    // These two are hits.
    PreconnectedRequestStats origin1(GURL(gen(1)).GetOrigin(), false, false);
    PreconnectedRequestStats origin2(GURL(gen(2)).GetOrigin(), true, false);
    // And these two are misses.
    PreconnectedRequestStats origin3(GURL(gen(3)).GetOrigin(), false, false);
    PreconnectedRequestStats origin4(GURL(gen(4)).GetOrigin(), true, false);

    auto stats = std::make_unique<PreconnectStats>(GURL(main_frame_url));
    stats->requests_stats = {origin1, origin2, origin3, origin4};

    stats_collector_->RecordPreconnectStats(std::move(stats));
  }

  {
    // Simulate a page load with 3 origins.
    URLRequestSummary script1 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(1), content::RESOURCE_TYPE_SCRIPT);
    URLRequestSummary script2 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(2), content::RESOURCE_TYPE_SCRIPT);
    URLRequestSummary script100 = CreateURLRequestSummary(
        kTabId, main_frame_url, gen(100), content::RESOURCE_TYPE_SCRIPT);
    PageRequestSummary summary = CreatePageRequestSummary(
        main_frame_url, main_frame_url, {script1, script2, script100});

    stats_collector_->RecordPageRequestSummary(summary);
  }

  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreresolveHitsPercentage, 50, 1);
  // Can't really report a hits percentage if there were no events.
  histogram_tester_->ExpectTotalCount(
      internal::kLoadingPredictorPreconnectHitsPercentage, 0);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreresolveCount, 4, 1);
  histogram_tester_->ExpectUniqueSample(
      internal::kLoadingPredictorPreconnectCount, 0, 1);
}

}  // namespace predictors
