// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/engagement/top_sites/site_engagement_top_sites_provider.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/engagement/site_engagement_score.h"
#include "chrome/browser/engagement/site_engagement_service.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_client.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/visit_delegate.h"
#include "components/history/core/test/test_history_database.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace {

// A class that runs and waits for a specified number of calls to
// ProvideTopSites(), caching the results so they are available for inspection.
class TopSitesProviderWaiter {
 public:
  TopSitesProviderWaiter(SiteEngagementService* engagement_service,
                         history::HistoryService* history_service,
                         int expected_calls)
      : provider_(
            std::make_unique<SiteEngagementTopSitesProvider>(engagement_service,
                                                             history_service)),
        barrier_(
            base::BarrierClosure(expected_calls, run_loop_.QuitClosure())) {}

  void QueryTopSites(int result_count) {
    provider_->ProvideTopSites(
        result_count,
        base::Bind(&TopSitesProviderWaiter::OnProvideTopSitesComplete,
                   base::Unretained(this)),
        &cancelable_task_tracker_);
  }

  void Wait() { run_loop_.Run(); }

  const std::vector<GURL>& urls() const { return urls_; }
  const std::vector<base::string16>& titles() const { return titles_; }
  const std::vector<history::RedirectList>& redirects() const {
    return redirects_;
  }

 private:
  void OnProvideTopSitesComplete(const history::MostVisitedURLList* urls) {
    for (const history::MostVisitedURL& mv : *urls) {
      urls_.push_back(mv.url);
      titles_.push_back(mv.title);
      redirects_.push_back(mv.redirects);
    }

    barrier_.Run();
  }

  std::unique_ptr<SiteEngagementTopSitesProvider> provider_;
  base::RunLoop run_loop_;
  base::RepeatingClosure barrier_;
  base::CancelableTaskTracker cancelable_task_tracker_;

  std::vector<GURL> urls_;
  std::vector<base::string16> titles_;
  std::vector<history::RedirectList> redirects_;
};

}  // namespace

class SiteEngagementTopSitesProviderTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    CreateHistoryService();
    SiteEngagementScore::SetParamValuesForTesting();
  }

  void TearDown() override {
    ChromeRenderViewHostTestHarness::TearDown();
    history_service_->Shutdown();
    history_service_.reset();
  }

 protected:
  void CreateHistoryService() {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    history_service_ =
        std::make_unique<history::HistoryService>(nullptr, nullptr);
    ASSERT_TRUE(history_service_->Init(
        history::TestHistoryDatabaseParamsForPath(scoped_temp_dir_.GetPath())));
  }

  void AddPageToHistory(const GURL& url,
                        const base::string16& title,
                        const history::RedirectList& redirects,
                        base::Time time) {
    history_service_->AddPage(url, time, nullptr, 0, GURL(), redirects,
                              ui::PAGE_TRANSITION_TYPED,
                              history::SOURCE_BROWSED, false);
    history_service_->SetPageTitle(url, title);
  }

  history::HistoryService* history_service() { return history_service_.get(); }

  SiteEngagementService* GetEngagementService() {
    return SiteEngagementService::Get(profile());
  }

  std::unique_ptr<SiteEngagementTopSitesProvider> CreateProvider() {
    return std::make_unique<SiteEngagementTopSitesProvider>(
        GetEngagementService(), history_service());
  }

  base::ScopedTempDir scoped_temp_dir_;
  std::unique_ptr<history::HistoryService> history_service_;
};

TEST_F(SiteEngagementTopSitesProviderTest, EmptyList) {
  // Ensure that the callback is run if the site engagement service provides no
  // URLs.
  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(10);
  waiter.Wait();

  EXPECT_TRUE(waiter.urls().empty());
  EXPECT_TRUE(waiter.titles().empty());
  EXPECT_TRUE(waiter.redirects().empty());
}

TEST_F(SiteEngagementTopSitesProviderTest, NoDataInHistory) {
  // Ensure that the callback is run if the history service is empty.
  GURL url("https://www.google.com");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url, 3);

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(1);
  waiter.Wait();

  EXPECT_EQ(std::vector<GURL>{url}, waiter.urls());
  EXPECT_EQ(std::vector<base::string16>{base::string16()}, waiter.titles());
  EXPECT_EQ(std::vector<history::RedirectList>{history::RedirectList{url}},
            waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, TitleAndRedirectsInHistory) {
  // Test a basic case with both titles and redirect lists.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 4);
  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  // Set up redirects from url1 and url2 to url3, making the expected redirect
  // list url2, url3, url1 and the expected title that of url3.
  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList{url1, url2, url3}, base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(10);
  waiter.Wait();

  EXPECT_EQ(std::vector<GURL>{url1}, waiter.urls());
  EXPECT_EQ(std::vector<base::string16>{base::UTF8ToUTF16("Another title")},
            waiter.titles());
  EXPECT_EQ((std::vector<history::RedirectList>{
                history::RedirectList{url2, url3, url1}}),
            waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, TitleOnlyInHistory) {
  // Test when the history service returns empty redirect lists.
  GURL url("https://www.google.com");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url, 1);
  AddPageToHistory(url, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(1);
  waiter.Wait();

  EXPECT_EQ(std::vector<GURL>{url}, waiter.urls());
  EXPECT_EQ(std::vector<base::string16>{base::UTF8ToUTF16("Title")},
            waiter.titles());
  EXPECT_EQ(std::vector<history::RedirectList>{history::RedirectList{url}},
            waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, RedirectsOnlyInHistory) {
  // Test when the history service returns empty titles.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 0.5);
  AddPageToHistory(url1, base::string16(), history::RedirectList{},
                   base::Time::Now());
  AddPageToHistory(url2, base::string16(),
                   history::RedirectList{url1, url3, url2}, base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(8);
  waiter.Wait();

  EXPECT_EQ(std::vector<GURL>{url1}, waiter.urls());
  EXPECT_EQ(std::vector<base::string16>{base::string16()}, waiter.titles());
  EXPECT_EQ((std::vector<history::RedirectList>{
                history::RedirectList{url3, url2, url1}}),
            waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, MaxRequestedIsRespected) {
  // Test that the maximum requested number of sites is respected.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");
  GURL url4("https://www.google.org");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 2);
  service->ResetBaseScoreForURL(url2, 1);
  service->ResetBaseScoreForURL(url3, 4);
  service->ResetBaseScoreForURL(url4, 0.5);
  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  AddPageToHistory(url2, base::UTF8ToUTF16("Other title"),
                   history::RedirectList(), base::Time::Now());

  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList(), base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(2);
  waiter.Wait();

  EXPECT_EQ((std::vector<GURL>{url3, url1}), waiter.urls());
  EXPECT_EQ((std::vector<base::string16>{base::UTF8ToUTF16("Another title"),
                                         base::UTF8ToUTF16("Title")}),
            waiter.titles());
  EXPECT_EQ((std::vector<history::RedirectList>{history::RedirectList{url3},
                                                history::RedirectList{url1}}),
            waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, OverlappingRedirects) {
  // Tests a case with a chain of redirects through url1 -> url2 -> url3. All
  // titles should match url3.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 4);
  service->ResetBaseScoreForURL(url2, 1);
  service->ResetBaseScoreForURL(url3, 2);

  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  AddPageToHistory(url2, base::UTF8ToUTF16("Other title"),
                   history::RedirectList(), base::Time::Now());

  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList{url1, url2, url3}, base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 1);
  waiter.QueryTopSites(3);
  waiter.Wait();

  EXPECT_EQ((std::vector<GURL>{url1, url3, url2}), waiter.urls());
  EXPECT_EQ((std::vector<base::string16>{base::UTF8ToUTF16("Another title"),
                                         base::UTF8ToUTF16("Another title"),
                                         base::UTF8ToUTF16("Another title")}),
            waiter.titles());

  EXPECT_EQ(
      (std::vector<history::RedirectList>{
          history::RedirectList{url2, url3, url1}, history::RedirectList{url3},
          history::RedirectList{url3, url2}}),
      waiter.redirects());
}

TEST_F(SiteEngagementTopSitesProviderTest, OverlappingRequests) {
  // Test when multiple requests are queued.
  GURL url1("https://www.google.com.au");
  GURL url2("https://www.google.com");
  GURL url3("https://www.google.blog");

  SiteEngagementService* service = GetEngagementService();
  service->ResetBaseScoreForURL(url1, 4);
  service->ResetBaseScoreForURL(url2, 3);
  service->ResetBaseScoreForURL(url3, 2);
  AddPageToHistory(url1, base::UTF8ToUTF16("Title"), history::RedirectList(),
                   base::Time::Now());

  AddPageToHistory(url2, base::UTF8ToUTF16("Other title"),
                   history::RedirectList(), base::Time::Now());

  AddPageToHistory(url3, base::UTF8ToUTF16("Another title"),
                   history::RedirectList(), base::Time::Now());

  TopSitesProviderWaiter waiter(GetEngagementService(), history_service(), 3);
  waiter.QueryTopSites(1);
  waiter.QueryTopSites(2);
  waiter.QueryTopSites(3);
  waiter.Wait();

  // Expect 3 of url1, 2 of url2, and 1 of url3 in some order.
  EXPECT_EQ(6u, waiter.urls().size());
  EXPECT_EQ(6u, waiter.titles().size());
  EXPECT_EQ(6u, waiter.redirects().size());

  EXPECT_EQ(3, std::count(waiter.urls().begin(), waiter.urls().end(), url1));
  EXPECT_EQ(2, std::count(waiter.urls().begin(), waiter.urls().end(), url2));
  EXPECT_EQ(1, std::count(waiter.urls().begin(), waiter.urls().end(), url3));

  EXPECT_EQ(3, std::count(waiter.titles().begin(), waiter.titles().end(),
                          base::UTF8ToUTF16("Title")));
  EXPECT_EQ(2, std::count(waiter.titles().begin(), waiter.titles().end(),
                          base::UTF8ToUTF16("Other title")));
  EXPECT_EQ(1, std::count(waiter.titles().begin(), waiter.titles().end(),
                          base::UTF8ToUTF16("Another title")));

  EXPECT_EQ(3, std::count(waiter.redirects().begin(), waiter.redirects().end(),
                          history::RedirectList{url1}));
  EXPECT_EQ(2, std::count(waiter.redirects().begin(), waiter.redirects().end(),
                          history::RedirectList{url2}));
  EXPECT_EQ(1, std::count(waiter.redirects().begin(), waiter.redirects().end(),
                          history::RedirectList{url3}));
}
