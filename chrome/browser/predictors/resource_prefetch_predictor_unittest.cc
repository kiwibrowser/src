// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/resource_prefetch_predictor.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "base/test/histogram_tester.h"
#include "base/test/test_simple_task_runner.h"
#include "base/time/time.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/predictors/loading_predictor.h"
#include "chrome/browser/predictors/loading_test_util.h"
#include "chrome/browser/predictors/resource_prefetch_predictor_tables.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_service.h"
#include "components/history/core/browser/history_types.h"
#include "components/sessions/core/session_id.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::StrictMock;
using testing::UnorderedElementsAre;

namespace predictors {
namespace {

using RedirectDataMap = std::map<std::string, RedirectData>;
using OriginDataMap = std::map<std::string, OriginData>;

constexpr SessionID kTabId = SessionID::FromSerializedValue(1);

template <typename T>
class FakeGlowplugKeyValueTable : public GlowplugKeyValueTable<T> {
 public:
  FakeGlowplugKeyValueTable() : GlowplugKeyValueTable<T>("") {}
  void GetAllData(std::map<std::string, T>* data_map,
                  sql::Connection* db) const override {
    *data_map = data_;
  }
  void UpdateData(const std::string& key,
                  const T& data,
                  sql::Connection* db) override {
    data_[key] = data;
  }
  void DeleteData(const std::vector<std::string>& keys,
                  sql::Connection* db) override {
    for (const auto& key : keys)
      data_.erase(key);
  }
  void DeleteAllData(sql::Connection* db) override { data_.clear(); }

  std::map<std::string, T> data_;
};

class MockResourcePrefetchPredictorTables
    : public ResourcePrefetchPredictorTables {
 public:
  MockResourcePrefetchPredictorTables(
      scoped_refptr<base::SequencedTaskRunner> db_task_runner)
      : ResourcePrefetchPredictorTables(std::move(db_task_runner)) {}

  void ScheduleDBTask(const base::Location& from_here, DBTask task) override {
    ExecuteDBTaskOnDBSequence(std::move(task));
  }

  void ExecuteDBTaskOnDBSequence(DBTask task) override {
    std::move(task).Run(nullptr);
  }

  GlowplugKeyValueTable<RedirectData>* host_redirect_table() override {
    return &host_redirect_table_;
  }

  GlowplugKeyValueTable<OriginData>* origin_table() override {
    return &origin_table_;
  }

  FakeGlowplugKeyValueTable<RedirectData> host_redirect_table_;
  FakeGlowplugKeyValueTable<OriginData> origin_table_;

 protected:
  ~MockResourcePrefetchPredictorTables() override = default;
};

class MockResourcePrefetchPredictorObserver : public TestObserver {
 public:
  explicit MockResourcePrefetchPredictorObserver(
      ResourcePrefetchPredictor* predictor)
      : TestObserver(predictor) {}

  MOCK_METHOD1(OnNavigationLearned, void(const PageRequestSummary& summary));
};

}  // namespace

class ResourcePrefetchPredictorTest : public testing::Test {
 public:
  ResourcePrefetchPredictorTest();
  ~ResourcePrefetchPredictorTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  void InitializePredictor() {
    loading_predictor_->StartInitialization();
    db_task_runner_->RunUntilIdle();
    profile_->BlockUntilHistoryProcessesPendingRequests();
  }

  void ResetPredictor(bool small_db = true) {
    if (loading_predictor_)
      loading_predictor_->Shutdown();

    LoadingPredictorConfig config;
    PopulateTestConfig(&config, small_db);
    loading_predictor_ =
        std::make_unique<LoadingPredictor>(config, profile_.get());
    predictor_ = loading_predictor_->resource_prefetch_predictor();
    predictor_->set_mock_tables(mock_tables_);
  }

  void InitializeSampleData();

  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;
  scoped_refptr<base::TestSimpleTaskRunner> db_task_runner_;
  net::TestURLRequestContext url_request_context_;

  std::unique_ptr<LoadingPredictor> loading_predictor_;
  ResourcePrefetchPredictor* predictor_;
  scoped_refptr<StrictMock<MockResourcePrefetchPredictorTables>> mock_tables_;

  RedirectDataMap test_host_redirect_data_;
  OriginDataMap test_origin_data_;

  MockURLRequestJobFactory url_request_job_factory_;

  std::unique_ptr<base::HistogramTester> histogram_tester_;
};

ResourcePrefetchPredictorTest::ResourcePrefetchPredictorTest()
    : profile_(std::make_unique<TestingProfile>()),
      db_task_runner_(base::MakeRefCounted<base::TestSimpleTaskRunner>()),
      mock_tables_(
          base::MakeRefCounted<StrictMock<MockResourcePrefetchPredictorTables>>(
              db_task_runner_)) {}

ResourcePrefetchPredictorTest::~ResourcePrefetchPredictorTest() = default;

void ResourcePrefetchPredictorTest::SetUp() {
  InitializeSampleData();

  CHECK(profile_->CreateHistoryService(true, false));
  profile_->BlockUntilHistoryProcessesPendingRequests();
  CHECK(HistoryServiceFactory::GetForProfile(
      profile_.get(), ServiceAccessType::EXPLICIT_ACCESS));
  // Initialize the predictor with empty data.
  ResetPredictor();
  // The first creation of the LoadingPredictor constructs the PredictorDatabase
  // for the |profile_|. The PredictorDatabase is initialized asynchronously and
  // we have to wait for the initialization completion even though the database
  // object is later replaced by a mock object.
  content::RunAllTasksUntilIdle();
  CHECK_EQ(predictor_->initialization_state_,
           ResourcePrefetchPredictor::NOT_INITIALIZED);
  InitializePredictor();
  CHECK_EQ(predictor_->initialization_state_,
           ResourcePrefetchPredictor::INITIALIZED);

  url_request_job_factory_.Reset();
  url_request_context_.set_job_factory(&url_request_job_factory_);

  histogram_tester_ = std::make_unique<base::HistogramTester>();
}

void ResourcePrefetchPredictorTest::TearDown() {
  EXPECT_EQ(*predictor_->host_redirect_data_->data_cache_,
            mock_tables_->host_redirect_table_.data_);
  EXPECT_EQ(*predictor_->origin_data_->data_cache_,
            mock_tables_->origin_table_.data_);
  loading_predictor_->Shutdown();
}

void ResourcePrefetchPredictorTest::InitializeSampleData() {
  {  // Host redirect data.
    RedirectData bbc = CreateRedirectData("bbc.com", 9);
    InitializeRedirectStat(bbc.add_redirect_endpoints(), "www.bbc.com", 8, 4,
                           1);
    InitializeRedirectStat(bbc.add_redirect_endpoints(), "m.bbc.com", 5, 8, 0);
    InitializeRedirectStat(bbc.add_redirect_endpoints(), "bbc.co.uk", 1, 3, 0);

    RedirectData microsoft = CreateRedirectData("microsoft.com", 10);
    InitializeRedirectStat(microsoft.add_redirect_endpoints(),
                           "www.microsoft.com", 10, 0, 0);

    test_host_redirect_data_.clear();
    test_host_redirect_data_.insert(std::make_pair(bbc.primary_key(), bbc));
    test_host_redirect_data_.insert(
        std::make_pair(microsoft.primary_key(), microsoft));
  }

  {  // Origin data.
    OriginData google = CreateOriginData("google.com", 12);
    InitializeOriginStat(google.add_origins(), "https://static.google.com", 12,
                         0, 0, 3., false, true);
    InitializeOriginStat(google.add_origins(), "https://cats.google.com", 12, 0,
                         0, 5., true, true);
    test_origin_data_.insert({"google.com", google});

    OriginData twitter = CreateOriginData("twitter.com", 42);
    InitializeOriginStat(twitter.add_origins(), "https://static.twitter.com",
                         12, 0, 0, 3., false, true);
    InitializeOriginStat(twitter.add_origins(), "https://random.140chars.com",
                         12, 0, 0, 3., false, true);
    test_origin_data_.insert({"twitter.com", twitter});
  }
}

// Tests that the predictor initializes correctly without any data.
TEST_F(ResourcePrefetchPredictorTest, LazilyInitializeEmpty) {
  EXPECT_TRUE(mock_tables_->host_redirect_table_.data_.empty());
  EXPECT_TRUE(mock_tables_->origin_table_.data_.empty());
}

// Tests that the history and the db tables data are loaded correctly.
TEST_F(ResourcePrefetchPredictorTest, LazilyInitializeWithData) {
  mock_tables_->host_redirect_table_.data_ = test_host_redirect_data_;
  mock_tables_->origin_table_.data_ = test_origin_data_;

  ResetPredictor();
  InitializePredictor();

  // Test that the internal variables correctly initialized.
  EXPECT_EQ(predictor_->initialization_state_,
            ResourcePrefetchPredictor::INITIALIZED);

  // Integrity of the cache and the backend storage is checked on TearDown.
}

// Single navigation that will be recorded. Will check for duplicate
// resources and also for number of resources saved.
TEST_F(ResourcePrefetchPredictorTest, NavigationUrlNotInDB) {
  URLRequestSummary main_frame =
      CreateURLRequestSummary(kTabId, "http://www.google.com");

  std::vector<URLRequestSummary> resources;
  resources.push_back(CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://google.com/style1.css",
      content::RESOURCE_TYPE_STYLESHEET));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script1.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script2.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script1.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/image1.png",
                                              content::RESOURCE_TYPE_IMAGE));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/image2.png",
                                              content::RESOURCE_TYPE_IMAGE));
  resources.push_back(CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://google.com/style2.css",
      content::RESOURCE_TYPE_STYLESHEET));

  auto no_store =
      CreateURLRequestSummary(kTabId, "http://www.google.com",
                              "http://static.google.com/style2-no-store.css",
                              content::RESOURCE_TYPE_STYLESHEET);
  no_store.is_no_store = true;

  auto redirected = CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://reader.google.com/style.css",
      content::RESOURCE_TYPE_STYLESHEET);
  redirected.redirect_url = GURL("http://dev.null.google.com/style.css");

  auto page_summary = CreatePageRequestSummary(
      "http://www.google.com", "http://www.google.com", resources);
  page_summary.UpdateOrAddToOrigins(no_store);
  page_summary.UpdateOrAddToOrigins(redirected);

  redirected.is_no_store = true;
  redirected.request_url = redirected.redirect_url;
  redirected.redirect_url = GURL();
  page_summary.UpdateOrAddToOrigins(redirected);

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  OriginData origin_data = CreateOriginData("www.google.com");
  InitializeOriginStat(origin_data.add_origins(), "http://www.google.com/", 1,
                       0, 0, 1., false, true);
  InitializeOriginStat(origin_data.add_origins(), "http://static.google.com/",
                       1, 0, 0, 3., true, true);
  InitializeOriginStat(origin_data.add_origins(), "http://dev.null.google.com/",
                       1, 0, 0, 5., true, true);
  InitializeOriginStat(origin_data.add_origins(), "http://google.com/", 1, 0, 0,
                       2., false, true);
  InitializeOriginStat(origin_data.add_origins(), "http://reader.google.com/",
                       1, 0, 0, 4., false, true);
  EXPECT_EQ(mock_tables_->origin_table_.data_,
            OriginDataMap({{origin_data.host(), origin_data}}));

  RedirectData host_redirect_data = CreateRedirectData("www.google.com");
  InitializeRedirectStat(host_redirect_data.add_redirect_endpoints(),
                         "www.google.com", 1, 0, 0);
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_,
            RedirectDataMap(
                {{host_redirect_data.primary_key(), host_redirect_data}}));
}

// Tests that navigation is recorded correctly for URL already present in
// the database cache.
TEST_F(ResourcePrefetchPredictorTest, NavigationUrlInDB) {
  ResetPredictor();
  InitializePredictor();

  URLRequestSummary main_frame = CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://www.google.com",
      content::RESOURCE_TYPE_MAIN_FRAME);

  std::vector<URLRequestSummary> resources;
  resources.push_back(CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://google.com/style1.css",
      content::RESOURCE_TYPE_STYLESHEET));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script1.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script2.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/script1.js",
                                              content::RESOURCE_TYPE_SCRIPT));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/image1.png",
                                              content::RESOURCE_TYPE_IMAGE));
  resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                              "http://google.com/image2.png",
                                              content::RESOURCE_TYPE_IMAGE));
  resources.push_back(CreateURLRequestSummary(
      kTabId, "http://www.google.com", "http://google.com/style2.css",
      content::RESOURCE_TYPE_STYLESHEET));
  auto no_store =
      CreateURLRequestSummary(kTabId, "http://www.google.com",
                              "http://static.google.com/style2-no-store.css",
                              content::RESOURCE_TYPE_STYLESHEET);
  no_store.is_no_store = true;

  auto page_summary = CreatePageRequestSummary(
      "http://www.google.com", "http://www.google.com", resources);
  page_summary.UpdateOrAddToOrigins(no_store);

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  RedirectData host_redirect_data = CreateRedirectData("www.google.com");
  InitializeRedirectStat(host_redirect_data.add_redirect_endpoints(),
                         "www.google.com", 1, 0, 0);
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_,
            RedirectDataMap(
                {{host_redirect_data.primary_key(), host_redirect_data}}));

  OriginData origin_data = CreateOriginData("www.google.com");
  InitializeOriginStat(origin_data.add_origins(), "http://www.google.com/", 1.,
                       0, 0, 1., false, true);
  InitializeOriginStat(origin_data.add_origins(), "http://static.google.com/",
                       1, 0, 0, 3., true, true);
  InitializeOriginStat(origin_data.add_origins(), "http://google.com/", 1, 0, 0,
                       2., false, true);
  EXPECT_EQ(mock_tables_->origin_table_.data_,
            OriginDataMap({{origin_data.host(), origin_data}}));
}

// Tests that a URL is deleted before another is added if the cache is full.
TEST_F(ResourcePrefetchPredictorTest, NavigationUrlNotInDBAndDBFull) {
  mock_tables_->origin_table_.data_ = test_origin_data_;

  ResetPredictor();
  InitializePredictor();

  URLRequestSummary main_frame = CreateURLRequestSummary(
      kTabId, "http://www.nike.com", "http://www.nike.com",
      content::RESOURCE_TYPE_MAIN_FRAME);

  URLRequestSummary resource1 = CreateURLRequestSummary(
      kTabId, "http://www.nike.com", "http://nike.com/style1.css",
      content::RESOURCE_TYPE_STYLESHEET);
  URLRequestSummary resource2 = CreateURLRequestSummary(
      kTabId, "http://www.nike.com", "http://nike.com/image2.png",
      content::RESOURCE_TYPE_IMAGE);

  auto page_summary = CreatePageRequestSummary(
      "http://www.nike.com", "http://www.nike.com", {resource1, resource2});

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  RedirectData host_redirect_data = CreateRedirectData("www.nike.com");
  InitializeRedirectStat(host_redirect_data.add_redirect_endpoints(),
                         "www.nike.com", 1, 0, 0);
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_,
            RedirectDataMap(
                {{host_redirect_data.primary_key(), host_redirect_data}}));

  OriginData origin_data = CreateOriginData("www.nike.com");
  InitializeOriginStat(origin_data.add_origins(), "http://www.nike.com/", 1, 0,
                       0, 1., false, true);
  InitializeOriginStat(origin_data.add_origins(), "http://nike.com/", 1, 0, 0,
                       2., false, true);
  OriginDataMap expected_origin_data = test_origin_data_;
  expected_origin_data.erase("google.com");
  expected_origin_data["www.nike.com"] = origin_data;
  EXPECT_EQ(mock_tables_->origin_table_.data_, expected_origin_data);
}

TEST_F(ResourcePrefetchPredictorTest,
       NavigationManyResourcesWithDifferentOrigins) {
  URLRequestSummary main_frame =
      CreateURLRequestSummary(kTabId, "http://www.google.com");

  auto gen = [](int i) {
    return base::StringPrintf("http://cdn%d.google.com/script.js", i);
  };
  std::vector<URLRequestSummary> resources;
  const int num_resources = predictor_->config_.max_origins_per_entry + 10;
  for (int i = 1; i <= num_resources; ++i) {
    resources.push_back(CreateURLRequestSummary(kTabId, "http://www.google.com",
                                                gen(i),
                                                content::RESOURCE_TYPE_SCRIPT));
  }

  auto page_summary = CreatePageRequestSummary(
      "http://www.google.com", "http://www.google.com", resources);

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  OriginData origin_data = CreateOriginData("www.google.com");
  InitializeOriginStat(origin_data.add_origins(), "http://www.google.com/", 1,
                       0, 0, 1, false, true);
  for (int i = 1;
       i <= static_cast<int>(predictor_->config_.max_origins_per_entry) - 1;
       ++i) {
    InitializeOriginStat(origin_data.add_origins(),
                         GURL(gen(i)).GetOrigin().spec(), 1, 0, 0, i + 1, false,
                         true);
  }
  EXPECT_EQ(mock_tables_->origin_table_.data_,
            OriginDataMap({{origin_data.host(), origin_data}}));
}

TEST_F(ResourcePrefetchPredictorTest, RedirectUrlNotInDB) {
  auto page_summary = CreatePageRequestSummary(
      "https://facebook.com/google", "http://fb.com/google",
      std::vector<URLRequestSummary>());

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  RedirectData host_redirect_data = CreateRedirectData("fb.com");
  InitializeRedirectStat(host_redirect_data.add_redirect_endpoints(),
                         "facebook.com", 1, 0, 0);
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_,
            RedirectDataMap(
                {{host_redirect_data.primary_key(), host_redirect_data}}));
}

// Tests that redirect is recorded correctly for URL already present in
// the database cache.
TEST_F(ResourcePrefetchPredictorTest, RedirectUrlInDB) {
  mock_tables_->host_redirect_table_.data_ = test_host_redirect_data_;

  ResetPredictor();
  InitializePredictor();

  auto page_summary = CreatePageRequestSummary(
      "https://facebook.com/google", "http://fb.com/google",
      std::vector<URLRequestSummary>());

  StrictMock<MockResourcePrefetchPredictorObserver> mock_observer(predictor_);
  EXPECT_CALL(mock_observer, OnNavigationLearned(page_summary));

  predictor_->RecordPageRequestSummary(
      std::make_unique<PageRequestSummary>(page_summary));
  profile_->BlockUntilHistoryProcessesPendingRequests();

  RedirectData host_redirect_data = CreateRedirectData("fb.com");
  InitializeRedirectStat(host_redirect_data.add_redirect_endpoints(),
                         "facebook.com", 1, 0, 0);
  RedirectDataMap expected_host_redirect_data = test_host_redirect_data_;
  expected_host_redirect_data.erase("bbc.com");
  expected_host_redirect_data[host_redirect_data.primary_key()] =
      host_redirect_data;
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_,
            expected_host_redirect_data);
}

TEST_F(ResourcePrefetchPredictorTest, DeleteUrls) {
  ResetPredictor(false);
  InitializePredictor();

  // Add some dummy entries to cache.

  RedirectDataMap host_redirects;
  host_redirects.insert(
      {"www.google.com", CreateRedirectData("www.google.com")});
  host_redirects.insert({"www.nike.com", CreateRedirectData("www.nike.com")});
  host_redirects.insert(
      {"www.wikipedia.org", CreateRedirectData("www.wikipedia.org")});
  for (const auto& redirect : host_redirects) {
    predictor_->host_redirect_data_->UpdateData(redirect.first,
                                                redirect.second);
  }

  // TODO(alexilin): Add origin data.

  history::URLRows rows;
  rows.push_back(history::URLRow(GURL("http://www.google.com/page2.html")));
  rows.push_back(history::URLRow(GURL("http://www.apple.com")));
  rows.push_back(history::URLRow(GURL("http://www.nike.com")));

  host_redirects.erase("www.google.com");
  host_redirects.erase("www.nike.com");

  predictor_->DeleteUrls(rows);
  EXPECT_EQ(mock_tables_->host_redirect_table_.data_, host_redirects);

  predictor_->DeleteAllUrls();
  EXPECT_TRUE(mock_tables_->host_redirect_table_.data_.empty());
}

TEST_F(ResourcePrefetchPredictorTest, GetRedirectEndpoint) {
  auto& redirect_data = *predictor_->host_redirect_data_;
  std::string redirect_endpoint;
  // Returns the initial url if data_map doesn't contain an entry for the url.
  EXPECT_TRUE(predictor_->GetRedirectEndpoint("bbc.com", redirect_data,
                                              &redirect_endpoint));
  EXPECT_EQ(redirect_endpoint, "bbc.com");

  // The data to be requested for the confident endpoint.
  RedirectData nyt = CreateRedirectData("nyt.com", 1);
  InitializeRedirectStat(nyt.add_redirect_endpoints(), "mobile.nytimes.com", 10,
                         0, 0);
  redirect_data.UpdateData(nyt.primary_key(), nyt);
  EXPECT_TRUE(predictor_->GetRedirectEndpoint("nyt.com", redirect_data,
                                              &redirect_endpoint));
  EXPECT_EQ(redirect_endpoint, "mobile.nytimes.com");

  // The data to check negative result due not enough confidence.
  RedirectData facebook = CreateRedirectData("fb.com", 3);
  InitializeRedirectStat(facebook.add_redirect_endpoints(), "facebook.com", 5,
                         5, 0);
  redirect_data.UpdateData(facebook.primary_key(), facebook);
  EXPECT_FALSE(predictor_->GetRedirectEndpoint("fb.com", redirect_data,
                                               &redirect_endpoint));

  // The data to check negative result due ambiguity.
  RedirectData google = CreateRedirectData("google.com", 4);
  InitializeRedirectStat(google.add_redirect_endpoints(), "google.com", 10, 0,
                         0);
  InitializeRedirectStat(google.add_redirect_endpoints(), "google.fr", 10, 1,
                         0);
  InitializeRedirectStat(google.add_redirect_endpoints(), "google.ws", 20, 20,
                         0);
  redirect_data.UpdateData(google.primary_key(), google);
  EXPECT_FALSE(predictor_->GetRedirectEndpoint("google.com", redirect_data,
                                               &redirect_endpoint));
}

TEST_F(ResourcePrefetchPredictorTest, TestPredictPreconnectOrigins) {
  const GURL main_frame_url("http://google.com/?query=cats");
  auto prediction = std::make_unique<PreconnectPrediction>();
  // No prefetch data.
  EXPECT_FALSE(predictor_->IsUrlPreconnectable(main_frame_url));
  EXPECT_FALSE(
      predictor_->PredictPreconnectOrigins(main_frame_url, prediction.get()));

  const char* cdn_origin = "https://cdn%d.google.com";
  auto gen_origin = [cdn_origin](int n) {
    return base::StringPrintf(cdn_origin, n);
  };

  // Add origins associated with the main frame host.
  OriginData google = CreateOriginData("google.com");
  InitializeOriginStat(google.add_origins(), gen_origin(1), 10, 0, 0, 1.0, true,
                       true);  // High confidence - preconnect.
  InitializeOriginStat(google.add_origins(), gen_origin(2), 10, 5, 0, 2.0, true,
                       true);  // Medium confidence - preresolve.
  InitializeOriginStat(google.add_origins(), gen_origin(3), 1, 10, 10, 3.0,
                       true, true);  // Low confidence - ignore.
  predictor_->origin_data_->UpdateData(google.host(), google);

  prediction = std::make_unique<PreconnectPrediction>();
  EXPECT_TRUE(predictor_->IsUrlPreconnectable(main_frame_url));
  EXPECT_TRUE(
      predictor_->PredictPreconnectOrigins(main_frame_url, prediction.get()));
  EXPECT_EQ(*prediction,
            CreatePreconnectPrediction(
                "google.com", false,
                {{GURL(gen_origin(1)), 1}, {GURL(gen_origin(2)), 0}}));

  // Add a redirect.
  RedirectData redirect = CreateRedirectData("google.com", 3);
  InitializeRedirectStat(redirect.add_redirect_endpoints(), "www.google.com",
                         10, 0, 0);
  predictor_->host_redirect_data_->UpdateData(redirect.primary_key(), redirect);

  // Prediction failed: no data associated with the redirect endpoint.
  prediction = std::make_unique<PreconnectPrediction>();
  EXPECT_FALSE(predictor_->IsUrlPreconnectable(main_frame_url));
  EXPECT_FALSE(
      predictor_->PredictPreconnectOrigins(main_frame_url, prediction.get()));

  // Add a resource associated with the redirect endpoint.
  OriginData www_google = CreateOriginData("www.google.com", 4);
  InitializeOriginStat(www_google.add_origins(), gen_origin(4), 10, 0, 0, 1.0,
                       true,
                       true);  // High confidence - preconnect.
  predictor_->origin_data_->UpdateData(www_google.host(), www_google);

  prediction = std::make_unique<PreconnectPrediction>();
  EXPECT_TRUE(predictor_->IsUrlPreconnectable(main_frame_url));
  EXPECT_TRUE(
      predictor_->PredictPreconnectOrigins(main_frame_url, prediction.get()));
  EXPECT_EQ(*prediction,
            CreatePreconnectPrediction("www.google.com", true,
                                       {{GURL(gen_origin(4)), 1}}));
}

}  // namespace predictors
