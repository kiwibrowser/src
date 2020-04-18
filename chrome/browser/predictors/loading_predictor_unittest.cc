// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/loading_predictor.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/test/histogram_tester.h"
#include "chrome/browser/predictors/loading_test_util.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;
using testing::StrictMock;
using testing::DoAll;
using testing::SetArgPointee;

namespace predictors {

namespace {

// First two are preconnectable, last one is not (see SetUp()).
const char kUrl[] = "http://www.google.com/cats";
const char kUrl2[] = "http://www.google.com/dogs";
const char kUrl3[] =
    "file://unknown.website/catsanddogs";  // Non http(s) scheme to avoid
                                           // preconnect to the main frame.

class MockPreconnectManager : public PreconnectManager {
 public:
  MockPreconnectManager(
      base::WeakPtr<Delegate> delegate,
      scoped_refptr<net::URLRequestContextGetter> context_getter);

  MOCK_METHOD2(StartProxy,
               void(const GURL& url,
                    const std::vector<PreconnectRequest>& requests));
  MOCK_METHOD1(StartPreresolveHost, void(const GURL& url));
  MOCK_METHOD1(StartPreresolveHosts,
               void(const std::vector<std::string>& hostnames));
  MOCK_METHOD2(StartPreconnectUrl,
               void(const GURL& url, bool allow_credentials));
  MOCK_METHOD1(Stop, void(const GURL& url));

  void Start(const GURL& url,
             std::vector<PreconnectRequest>&& requests) override {
    StartProxy(url, requests);
  }
};

MockPreconnectManager::MockPreconnectManager(
    base::WeakPtr<Delegate> delegate,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : PreconnectManager(delegate, context_getter) {}

}  // namespace

class LoadingPredictorTest : public testing::Test {
 public:
  LoadingPredictorTest();
  ~LoadingPredictorTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  virtual LoadingPredictorConfig CreateConfig();

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<LoadingPredictor> predictor_;
  StrictMock<MockResourcePrefetchPredictor>* mock_predictor_;
};

LoadingPredictorTest::LoadingPredictorTest()
    : profile_(std::make_unique<TestingProfile>()) {}

LoadingPredictorTest::~LoadingPredictorTest() = default;

void LoadingPredictorTest::SetUp() {
  auto config = CreateConfig();
  predictor_ = std::make_unique<LoadingPredictor>(config, profile_.get());

  auto mock = std::make_unique<StrictMock<MockResourcePrefetchPredictor>>(
      config, profile_.get());
  EXPECT_CALL(*mock, PredictPreconnectOrigins(GURL(kUrl), _))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock, PredictPreconnectOrigins(GURL(kUrl2), _))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*mock, PredictPreconnectOrigins(GURL(kUrl3), _))
      .WillRepeatedly(Return(false));

  mock_predictor_ = mock.get();
  predictor_->set_mock_resource_prefetch_predictor(std::move(mock));

  predictor_->StartInitialization();
  content::RunAllTasksUntilIdle();
}

void LoadingPredictorTest::TearDown() {
  predictor_->Shutdown();
}

LoadingPredictorConfig LoadingPredictorTest::CreateConfig() {
  LoadingPredictorConfig config;
  PopulateTestConfig(&config);
  return config;
}

class LoadingPredictorPreconnectTest : public LoadingPredictorTest {
 public:
  void SetUp() override;

 protected:
  LoadingPredictorConfig CreateConfig() override;
  StrictMock<MockPreconnectManager>* mock_preconnect_manager_;
};

void LoadingPredictorPreconnectTest::SetUp() {
  LoadingPredictorTest::SetUp();
  auto mock_preconnect_manager =
      std::make_unique<StrictMock<MockPreconnectManager>>(
          predictor_->GetWeakPtr(), profile_->GetRequestContext());
  mock_preconnect_manager_ = mock_preconnect_manager.get();
  predictor_->set_mock_preconnect_manager(std::move(mock_preconnect_manager));
}

LoadingPredictorConfig LoadingPredictorPreconnectTest::CreateConfig() {
  LoadingPredictorConfig config = LoadingPredictorTest::CreateConfig();
  config.mode |= LoadingPredictorConfig::PRECONNECT;
  return config;
}

TEST_F(LoadingPredictorTest, TestPrefetchingDurationHistogram) {
  base::HistogramTester histogram_tester;
  const GURL url = GURL(kUrl);
  const GURL url2 = GURL(kUrl2);
  const GURL url3 = GURL(kUrl3);

  predictor_->PrepareForPageLoad(url, HintOrigin::NAVIGATION);
  predictor_->CancelPageLoadHint(url);
  histogram_tester.ExpectTotalCount(
      internal::kResourcePrefetchPredictorPrefetchingDurationHistogram, 1);

  // Mismatched start / end.
  predictor_->PrepareForPageLoad(url, HintOrigin::NAVIGATION);
  predictor_->CancelPageLoadHint(url2);
  // No increment.
  histogram_tester.ExpectTotalCount(
      internal::kResourcePrefetchPredictorPrefetchingDurationHistogram, 1);

  // Can track a navigation (url2) while one is still in progress (url).
  predictor_->PrepareForPageLoad(url2, HintOrigin::NAVIGATION);
  predictor_->CancelPageLoadHint(url2);
  histogram_tester.ExpectTotalCount(
      internal::kResourcePrefetchPredictorPrefetchingDurationHistogram, 2);

  // Do not track non-prefetchable URLs.
  predictor_->PrepareForPageLoad(url3, HintOrigin::NAVIGATION);
  predictor_->CancelPageLoadHint(url3);
  // No increment.
  histogram_tester.ExpectTotalCount(
      internal::kResourcePrefetchPredictorPrefetchingDurationHistogram, 2);
}

TEST_F(LoadingPredictorTest, TestMainFrameResponseCancelsHint) {
  const GURL url = GURL(kUrl);
  predictor_->PrepareForPageLoad(url, HintOrigin::EXTERNAL);
  EXPECT_EQ(1UL, predictor_->active_hints_.size());

  auto summary =
      CreateURLRequestSummary(SessionID::FromSerializedValue(12), url.spec());
  predictor_->OnMainFrameResponse(summary);
  EXPECT_TRUE(predictor_->active_hints_.empty());
}

TEST_F(LoadingPredictorTest, TestMainFrameRequestCancelsStaleNavigations) {
  const std::string url = kUrl;
  const std::string url2 = kUrl2;
  const SessionID tab_id = SessionID::FromSerializedValue(12);
  const auto& active_navigations = predictor_->active_navigations_;
  const auto& active_hints = predictor_->active_hints_;

  auto summary = CreateURLRequestSummary(tab_id, url);
  auto navigation_id = CreateNavigationID(tab_id, url);

  predictor_->OnMainFrameRequest(summary);
  EXPECT_NE(active_navigations.find(navigation_id), active_navigations.end());
  EXPECT_NE(active_hints.find(GURL(url)), active_hints.end());

  summary = CreateURLRequestSummary(tab_id, url2);
  predictor_->OnMainFrameRequest(summary);
  EXPECT_EQ(active_navigations.find(navigation_id), active_navigations.end());
  EXPECT_EQ(active_hints.find(GURL(url)), active_hints.end());

  auto navigation_id2 = CreateNavigationID(tab_id, url2);
  EXPECT_NE(active_navigations.find(navigation_id2), active_navigations.end());
}

TEST_F(LoadingPredictorTest, TestMainFrameResponseClearsNavigations) {
  const std::string url = kUrl;
  const std::string redirected = kUrl2;
  const SessionID tab_id = SessionID::FromSerializedValue(12);
  const auto& active_navigations = predictor_->active_navigations_;
  const auto& active_hints = predictor_->active_hints_;

  auto summary = CreateURLRequestSummary(tab_id, url);
  auto navigation_id = CreateNavigationID(tab_id, url);

  predictor_->OnMainFrameRequest(summary);
  EXPECT_NE(active_navigations.find(navigation_id), active_navigations.end());
  EXPECT_FALSE(active_hints.empty());

  predictor_->OnMainFrameResponse(summary);
  EXPECT_TRUE(active_navigations.empty());
  EXPECT_TRUE(active_hints.empty());

  // With redirects.
  predictor_->OnMainFrameRequest(summary);
  EXPECT_NE(active_navigations.find(navigation_id), active_navigations.end());
  EXPECT_FALSE(active_hints.empty());

  summary.redirect_url = GURL(redirected);
  predictor_->OnMainFrameRedirect(summary);
  EXPECT_FALSE(active_navigations.empty());
  EXPECT_FALSE(active_hints.empty());

  summary.navigation_id.main_frame_url = GURL(redirected);
  predictor_->OnMainFrameResponse(summary);
  EXPECT_TRUE(active_navigations.empty());
  EXPECT_TRUE(active_hints.empty());
}

TEST_F(LoadingPredictorTest, TestMainFrameRequestDoesntCancelExternalHint) {
  const GURL url = GURL(kUrl);
  const SessionID tab_id = SessionID::FromSerializedValue(12);
  const auto& active_navigations = predictor_->active_navigations_;
  auto& active_hints = predictor_->active_hints_;

  predictor_->PrepareForPageLoad(url, HintOrigin::EXTERNAL);
  auto it = active_hints.find(url);
  EXPECT_NE(it, active_hints.end());
  EXPECT_TRUE(active_navigations.empty());

  // To check that the hint is not replaced, set the start time in the past,
  // and check later that it didn't change.
  base::TimeTicks start_time = it->second - base::TimeDelta::FromSeconds(10);
  it->second = start_time;

  auto summary = CreateURLRequestSummary(tab_id, url.spec());
  predictor_->OnMainFrameRequest(summary);
  EXPECT_NE(active_navigations.find(summary.navigation_id),
            active_navigations.end());
  it = active_hints.find(url);
  EXPECT_NE(it, active_hints.end());
  EXPECT_EQ(start_time, it->second);
}

TEST_F(LoadingPredictorTest, TestDontTrackNonPrefetchableUrls) {
  const GURL url3 = GURL(kUrl3);
  predictor_->PrepareForPageLoad(url3, HintOrigin::NAVIGATION);
  EXPECT_TRUE(predictor_->active_hints_.empty());
}

TEST_F(LoadingPredictorTest, TestDontPredictOmniboxHints) {
  const GURL omnibox_suggestion = GURL("http://search.com/kittens");
  // We expect that no prediction will be requested.
  predictor_->PrepareForPageLoad(omnibox_suggestion, HintOrigin::OMNIBOX);
  EXPECT_TRUE(predictor_->active_hints_.empty());
}

TEST_F(LoadingPredictorPreconnectTest, TestHandleOmniboxHint) {
  const GURL preconnect_suggestion = GURL("http://search.com/kittens");
  EXPECT_CALL(*mock_preconnect_manager_,
              StartPreconnectUrl(preconnect_suggestion, true));
  predictor_->PrepareForPageLoad(preconnect_suggestion, HintOrigin::OMNIBOX,
                                 true);
  // The second suggestion for the same host should be filtered out.
  const GURL preconnect_suggestion2 = GURL("http://search.com/puppies");
  predictor_->PrepareForPageLoad(preconnect_suggestion2, HintOrigin::OMNIBOX,
                                 true);

  const GURL preresolve_suggestion = GURL("http://en.wikipedia.org/wiki/main");
  EXPECT_CALL(*mock_preconnect_manager_,
              StartPreresolveHost(preresolve_suggestion));
  predictor_->PrepareForPageLoad(preresolve_suggestion, HintOrigin::OMNIBOX,
                                 false);
  // The second suggestions should be filtered out as well.
  const GURL preresolve_suggestion2 =
      GURL("http://en.wikipedia.org/wiki/random");
  predictor_->PrepareForPageLoad(preresolve_suggestion2, HintOrigin::OMNIBOX,
                                 false);
}

// Checks that the predictor preconnects to an initial origin even when it
// doesn't have any historical data for this host.
TEST_F(LoadingPredictorPreconnectTest, TestAddInitialUrlToEmptyPrediction) {
  GURL main_frame_url("http://search.com/kittens");
  EXPECT_CALL(*mock_predictor_, PredictPreconnectOrigins(main_frame_url, _))
      .WillOnce(Return(false));
  EXPECT_CALL(
      *mock_preconnect_manager_,
      StartProxy(main_frame_url, std::vector<PreconnectRequest>(
                                     {{GURL("http://search.com"), 2}})));
  predictor_->PrepareForPageLoad(main_frame_url, HintOrigin::NAVIGATION);
}

// Checks that the predictor doesn't add an initial origin to a preconnect list
// if the list already containts the origin.
TEST_F(LoadingPredictorPreconnectTest, TestAddInitialUrlMatchesPrediction) {
  GURL main_frame_url("http://search.com/kittens");
  PreconnectPrediction prediction =
      CreatePreconnectPrediction("search.com", true,
                                 {{GURL("http://search.com"), 1},
                                  {GURL("http://cdn.search.com"), 1},
                                  {GURL("http://ads.search.com"), 0}});
  EXPECT_CALL(*mock_predictor_, PredictPreconnectOrigins(main_frame_url, _))
      .WillOnce(DoAll(SetArgPointee<1>(prediction), Return(true)));
  EXPECT_CALL(
      *mock_preconnect_manager_,
      StartProxy(main_frame_url, std::vector<PreconnectRequest>(
                                     {{GURL("http://search.com"), 2},
                                      {GURL("http://cdn.search.com"), 1},
                                      {GURL("http://ads.search.com"), 0}})));
  predictor_->PrepareForPageLoad(main_frame_url, HintOrigin::EXTERNAL);
}

// Checks that the predictor adds an initial origin to a preconnect list if the
// list doesn't contain this origin already. It may be possible if an initial
// url redirects to another host.
TEST_F(LoadingPredictorPreconnectTest, TestAddInitialUrlDoesntMatchPrediction) {
  GURL main_frame_url("http://search.com/kittens");
  PreconnectPrediction prediction =
      CreatePreconnectPrediction("search.com", true,
                                 {{GURL("http://en.search.com"), 1},
                                  {GURL("http://cdn.search.com"), 1},
                                  {GURL("http://ads.search.com"), 0}});
  EXPECT_CALL(*mock_predictor_, PredictPreconnectOrigins(main_frame_url, _))
      .WillOnce(DoAll(SetArgPointee<1>(prediction), Return(true)));
  EXPECT_CALL(
      *mock_preconnect_manager_,
      StartProxy(main_frame_url, std::vector<PreconnectRequest>(
                                     {{GURL("http://search.com"), 2},
                                      {GURL("http://en.search.com"), 1},
                                      {GURL("http://cdn.search.com"), 1},
                                      {GURL("http://ads.search.com"), 0}})));
  predictor_->PrepareForPageLoad(main_frame_url, HintOrigin::EXTERNAL);
}

// Checks that the predictor doesn't preconnect to a bad url.
TEST_F(LoadingPredictorPreconnectTest, TestAddInvalidInitialUrl) {
  GURL main_frame_url("file:///tmp/index.html");
  EXPECT_CALL(*mock_predictor_, PredictPreconnectOrigins(main_frame_url, _))
      .WillOnce(Return(false));
  predictor_->PrepareForPageLoad(main_frame_url, HintOrigin::EXTERNAL);
}

}  // namespace predictors
