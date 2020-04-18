// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_snippets/contextual/contextual_content_suggestions_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "components/image_fetcher/core/image_fetcher_impl.h"
#include "components/ntp_snippets/category_info.h"
#include "components/ntp_snippets/content_suggestion.h"
#include "components/ntp_snippets/contextual/contextual_suggestion.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_debugging_reporter.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_fetcher.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_reporter.h"
#include "components/ntp_snippets/contextual/contextual_suggestions_test_utils.h"
#include "components/ntp_snippets/remote/cached_image_fetcher.h"
#include "components/ntp_snippets/remote/json_to_categories.h"
#include "components/ntp_snippets/remote/remote_suggestions_database.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_unittest_util.h"

using ntp_snippets::CachedImageFetcher;
using ntp_snippets::Category;
using ntp_snippets::ContentSuggestion;
using ntp_snippets::KnownCategories;
using ntp_snippets::ImageFetchedCallback;
using ntp_snippets::ImageDataFetchedCallback;
using ntp_snippets::RemoteSuggestionsDatabase;
using ntp_snippets::RequestThrottler;

using testing::_;
using testing::AllOf;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::Mock;
using testing::Pointee;
using testing::Property;

namespace contextual_suggestions {

namespace {

// Always fetches the result that was set by SetFakeResponse.
class FakeContextualSuggestionsFetcher : public ContextualSuggestionsFetcher {
 public:
  void FetchContextualSuggestionsClusters(
      const GURL& url,
      FetchClustersCallback callback,
      ReportFetchMetricsCallback metrics_callback) override {
    ContextualSuggestionsResult result;
    result.peek_text = "peek text";
    result.clusters = std::move(fake_suggestions_);
    result.peek_conditions = peek_conditions_;
    std::move(callback).Run(std::move(result));
    fake_suggestions_.clear();
  }

  void SetFakeResponse(std::vector<Cluster> fake_suggestions,
                       PeekConditions peek_conditions = PeekConditions()) {
    fake_suggestions_ = std::move(fake_suggestions);
    peek_conditions_ = peek_conditions;
  }

 private:
  std::vector<Cluster> fake_suggestions_;
  PeekConditions peek_conditions_;
};

// Always fetches a fake image if the given URL is valid.
class FakeCachedImageFetcher : public CachedImageFetcher {
 public:
  FakeCachedImageFetcher(PrefService* pref_service)
      : CachedImageFetcher(std::unique_ptr<image_fetcher::ImageFetcher>(),
                           pref_service,
                           nullptr){};

  void FetchSuggestionImage(const ContentSuggestion::ID&,
                            const GURL& image_url,
                            ImageDataFetchedCallback image_data_callback,
                            ImageFetchedCallback callback) override {
    gfx::Image image;
    if (image_url.is_valid()) {
      image = gfx::test::CreateImage();
    }
    std::move(callback).Run(image);
  }
};

}  // namespace

class ContextualContentSuggestionsServiceTest : public testing::Test {
 public:
  ContextualContentSuggestionsServiceTest() {
    RequestThrottler::RegisterProfilePrefs(pref_service_.registry());
    std::unique_ptr<FakeContextualSuggestionsFetcher> fetcher =
        std::make_unique<FakeContextualSuggestionsFetcher>();
    fetcher_ = fetcher.get();
    auto debugging_reporter = std::make_unique<
        contextual_suggestions::ContextualSuggestionsDebuggingReporter>();
    auto reporter_provider = std::make_unique<
        contextual_suggestions::ContextualSuggestionsReporterProvider>(
        std::move(debugging_reporter));
    source_ = std::make_unique<ContextualContentSuggestionsService>(
        std::move(fetcher),
        std::make_unique<FakeCachedImageFetcher>(&pref_service_),
        std::unique_ptr<RemoteSuggestionsDatabase>(),
        std::move(reporter_provider));
  }

  FakeContextualSuggestionsFetcher* fetcher() { return fetcher_; }
  ContextualContentSuggestionsService* source() { return source_.get(); }

 private:
  FakeContextualSuggestionsFetcher* fetcher_;
  base::MessageLoop message_loop_;
  TestingPrefServiceSimple pref_service_;
  std::unique_ptr<ContextualContentSuggestionsService> source_;

  DISALLOW_COPY_AND_ASSIGN(ContextualContentSuggestionsServiceTest);
};

TEST_F(ContextualContentSuggestionsServiceTest,
       ShouldFetchContextualSuggestionsClusters) {
  MockClustersCallback mock_callback;
  std::vector<Cluster> clusters;
  GURL context_url("http://www.from.url");

  clusters.emplace_back(ClusterBuilder("Title")
                            .AddSuggestion(SuggestionBuilder(context_url)
                                               .Title("Title1")
                                               .PublisherName("from.url")
                                               .Snippet("Summary")
                                               .ImageId("abc")
                                               .Build())
                            .Build());

  fetcher()->SetFakeResponse(std::move(clusters));
  source()->FetchContextualSuggestionClusters(
      context_url, mock_callback.ToOnceCallback(), base::DoNothing());
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(mock_callback.has_run);
}

TEST_F(ContextualContentSuggestionsServiceTest, ShouldRejectInvalidUrls) {
  std::vector<Cluster> clusters;
  for (GURL invalid_url :
       {GURL("htp:/"), GURL("www.foobar"), GURL("http://127.0.0.1/"),
        GURL("file://some.file"), GURL("chrome://settings"), GURL("")}) {
    MockClustersCallback mock_callback;
    source()->FetchContextualSuggestionClusters(
        invalid_url,
        base::BindOnce(&MockClustersCallback::Done,
                       base::Unretained(&mock_callback)),
        base::DoNothing());
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(mock_callback.has_run);
    EXPECT_EQ(mock_callback.response_peek_text, "");
    EXPECT_EQ(mock_callback.response_clusters.size(), 0u);
  }
}

TEST_F(ContextualContentSuggestionsServiceTest,
       ShouldNotReportLowConfidenceResults) {
  MockClustersCallback mock_callback;
  std::vector<Cluster> clusters;
  GURL context_url("http://www.from.url");

  clusters.emplace_back(ClusterBuilder("Title")
                            .AddSuggestion(SuggestionBuilder(context_url)
                                               .Title("Title1")
                                               .PublisherName("from.url")
                                               .Snippet("Summary")
                                               .ImageId("abc")
                                               .Build())
                            .Build());

  PeekConditions peek_conditions;
  peek_conditions.confidence = 0.5;

  fetcher()->SetFakeResponse(std::move(clusters), peek_conditions);

  source()->FetchContextualSuggestionClusters(
      context_url, mock_callback.ToOnceCallback(), base::DoNothing());
  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(mock_callback.has_run);
  EXPECT_EQ(mock_callback.response_clusters.size(), 0u);
  EXPECT_EQ(mock_callback.response_peek_text, std::string());
}

}  // namespace contextual_suggestions
