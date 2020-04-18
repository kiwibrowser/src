// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/scoped_temp_dir.h"
#include "base/i18n/base_i18n_switches.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/browser/search/instant_service_observer.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/search/instant_test_utils.h"
#include "chrome/browser/ui/search/local_ntp_test_utils.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/search/instant_types.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_navigation_throttle_inserter.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_feature.mojom.h"
#include "url/gurl.h"

namespace {

// In a non-signed-in, fresh profile with no history, there should be one
// default TopSites tile (see history::PrepopulatedPage).
const int kDefaultMostVisitedItemCount = 1;

class TestMostVisitedObserver : public InstantServiceObserver {
 public:
  explicit TestMostVisitedObserver(InstantService* service)
      : service_(service), expected_count_(0) {
    service_->AddObserver(this);
  }

  ~TestMostVisitedObserver() override { service_->RemoveObserver(this); }

  void WaitForNumberOfItems(size_t count) {
    DCHECK(!quit_closure_);

    expected_count_ = count;

    if (items_.size() == count) {
      return;
    }

    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  void ThemeInfoChanged(const ThemeBackgroundInfo&) override {}

  void MostVisitedItemsChanged(
      const std::vector<InstantMostVisitedItem>& items) override {
    items_ = items;

    if (quit_closure_ && items_.size() == expected_count_) {
      std::move(quit_closure_).Run();
      quit_closure_.Reset();
    }
  }

  InstantService* const service_;

  std::vector<InstantMostVisitedItem> items_;

  size_t expected_count_;
  base::OnceClosure quit_closure_;
};

class LocalNTPTest : public InProcessBrowserTest {
 public:
  LocalNTPTest() {
    feature_list_.InitAndEnableFeature(features::kUseGoogleLocalNtp);
  }

  LocalNTPTest(const std::vector<base::Feature>& enabled_features) {
    feature_list_.InitWithFeatures(enabled_features, {});
  }

 private:
  void SetUpOnMainThread() override {
    // Some tests depend on the prepopulated most visited tiles coming from
    // TopSites, so make sure they are available before running the tests.
    // (TopSites is loaded asynchronously at startup, so without this, there's
    // a chance that it hasn't finished and we receive 0 tiles.)
    InstantService* instant_service =
        InstantServiceFactory::GetForProfile(browser()->profile());
    TestMostVisitedObserver mv_observer(instant_service);
    // Make sure the observer knows about the current items. Typically, this
    // gets triggered by navigating to an NTP.
    instant_service->UpdateMostVisitedItemsInfo();
    mv_observer.WaitForNumberOfItems(kDefaultMostVisitedItemCount);
  }

  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(LocalNTPTest, EmbeddedSearchAPIOnlyAvailableOnNTP) {
  // Set up a test server, so we have some arbitrary non-NTP URL to navigate to.
  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  test_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(test_server.Start());
  const GURL other_url = test_server.GetURL("/simple.html");

  // Open an NTP.
  content::WebContents* active_tab = local_ntp_test_utils::OpenNewTab(
      browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // Check that the embeddedSearch API is available.
  bool result = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_TRUE(result);

  // Navigate somewhere else in the same tab.
  content::TestNavigationObserver elsewhere_observer(active_tab);
  ui_test_utils::NavigateToURL(browser(), other_url);
  elsewhere_observer.Wait();
  ASSERT_TRUE(elsewhere_observer.last_navigation_succeeded());
  ASSERT_FALSE(search::IsInstantNTP(active_tab));

  // Now the embeddedSearch API should have gone away.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_FALSE(result);

  // Navigate back to the NTP.
  content::TestNavigationObserver back_observer(active_tab);
  chrome::GoBack(browser(), WindowOpenDisposition::CURRENT_TAB);
  back_observer.Wait();
  // The API should be back.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_TRUE(result);

  // Navigate forward to the non-NTP page.
  content::TestNavigationObserver fwd_observer(active_tab);
  chrome::GoForward(browser(), WindowOpenDisposition::CURRENT_TAB);
  fwd_observer.Wait();
  // The API should be gone.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_FALSE(result);

  // Navigate to a new NTP instance.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // Now the API should be available again.
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_TRUE(result);
}

// The spare RenderProcessHost is warmed up *before* the target destination is
// known and therefore doesn't include any special command-line flags that are
// used when launching a RenderProcessHost known to be needed for NTP.  This
// test ensures that the spare RenderProcessHost doesn't accidentally end up
// being used for NTP navigations.
IN_PROC_BROWSER_TEST_F(LocalNTPTest, SpareProcessDoesntInterfereWithSearchAPI) {
  content::WebContents* active_tab =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Navigate to a non-NTP URL, so that the next step needs to swap the process.
  GURL non_ntp_url = ui_test_utils::GetTestUrl(
      base::FilePath(), base::FilePath().AppendASCII("title1.html"));
  ui_test_utils::NavigateToURL(browser(), non_ntp_url);
  content::RenderProcessHost* old_process =
      active_tab->GetMainFrame()->GetProcess();

  // Navigate to an NTP while a spare process is present.
  content::RenderProcessHost::WarmupSpareRenderProcessHost(
      browser()->profile());
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  // Verify that a process swap has taken place.  This is an indirect indication
  // that the spare process could have been used (during the process swap).
  // This assertion is a sanity check of the test setup, rather than
  // verification of the core thing that the test cares about.
  content::RenderProcessHost* new_process =
      active_tab->GetMainFrame()->GetProcess();
  ASSERT_NE(new_process, old_process);

  // Check that the embeddedSearch API is available - the spare
  // RenderProcessHost either shouldn't be used, or if used it should have been
  // launched with the appropriate, NTP-specific cmdline flags.
  bool result = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.chrome.embeddedSearch", &result));
  EXPECT_TRUE(result);
}

// Regression test for crbug.com/776660 and crbug.com/776655.
IN_PROC_BROWSER_TEST_F(LocalNTPTest, EmbeddedSearchAPIExposesStaticFunctions) {
  // Open an NTP.
  content::WebContents* active_tab = local_ntp_test_utils::OpenNewTab(
      browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  struct TestCase {
    const char* function_name;
    const char* args;
  } test_cases[] = {
      {"window.chrome.embeddedSearch.searchBox.paste", "\"text\""},
      {"window.chrome.embeddedSearch.searchBox.startCapturingKeyStrokes", ""},
      {"window.chrome.embeddedSearch.searchBox.stopCapturingKeyStrokes", ""},
      {"window.chrome.embeddedSearch.newTabPage.checkIsUserSignedIntoChromeAs",
       "\"user@email.com\""},
      {"window.chrome.embeddedSearch.newTabPage.checkIsUserSyncingHistory",
       "\"user@email.com\""},
      {"window.chrome.embeddedSearch.newTabPage.deleteMostVisitedItem", "1"},
      {"window.chrome.embeddedSearch.newTabPage.deleteMostVisitedItem",
       "\"1\""},
      {"window.chrome.embeddedSearch.newTabPage.getMostVisitedItemData", "1"},
      {"window.chrome.embeddedSearch.newTabPage.logEvent", "1"},
      {"window.chrome.embeddedSearch.newTabPage.undoAllMostVisitedDeletions",
       ""},
      {"window.chrome.embeddedSearch.newTabPage.undoMostVisitedDeletion", "1"},
      {"window.chrome.embeddedSearch.newTabPage.undoMostVisitedDeletion",
       "\"1\""},
  };

  for (const TestCase& test_case : test_cases) {
    // Make sure that the API function exists.
    bool result = false;
    ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
        active_tab, base::StringPrintf("!!%s", test_case.function_name),
        &result));
    ASSERT_TRUE(result);

    // Check that it can be called normally.
    EXPECT_TRUE(content::ExecuteScript(
        active_tab,
        base::StringPrintf("%s(%s)", test_case.function_name, test_case.args)));

    // Check that it can be called even after it's assigned to a var, i.e.
    // without a "this" binding.
    EXPECT_TRUE(content::ExecuteScript(
        active_tab,
        base::StringPrintf("var f = %s; f(%s)", test_case.function_name,
                           test_case.args)));
  }
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, EmbeddedSearchAPIEndToEnd) {
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));

  TestMostVisitedObserver observer(
      InstantServiceFactory::GetForProfile(browser()->profile()));

  // Navigating to an NTP should trigger an update of the MV items.
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());
  observer.WaitForNumberOfItems(kDefaultMostVisitedItemCount);

  // Make sure the same number of items is available in JS.
  int most_visited_count = -1;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "window.chrome.embeddedSearch.newTabPage.mostVisited.length",
      &most_visited_count));
  ASSERT_EQ(kDefaultMostVisitedItemCount, most_visited_count);

  // Get the ID of one item.
  int most_visited_rid = -1;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "window.chrome.embeddedSearch.newTabPage.mostVisited[0].rid",
      &most_visited_rid));

  // Delete that item. The deletion should arrive on the native side.
  ASSERT_TRUE(content::ExecuteScript(
      active_tab,
      base::StringPrintf(
          "window.chrome.embeddedSearch.newTabPage.deleteMostVisitedItem(%d)",
          most_visited_rid)));
  observer.WaitForNumberOfItems(kDefaultMostVisitedItemCount - 1);
}

// Regression test for crbug.com/592273.
IN_PROC_BROWSER_TEST_F(LocalNTPTest, EmbeddedSearchAPIAfterDownload) {
  // Set up a test server, so we have some URL to download.
  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  test_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(test_server.Start());
  const GURL download_url = test_server.GetURL("/download-test1.lib");

  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));

  TestMostVisitedObserver observer(
      InstantServiceFactory::GetForProfile(browser()->profile()));

  // Navigating to an NTP should trigger an update of the MV items.
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());
  observer.WaitForNumberOfItems(kDefaultMostVisitedItemCount);

  // Download some file.
  content::DownloadTestObserverTerminal download_observer(
      content::BrowserContext::GetDownloadManager(browser()->profile()), 1,
      content::DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_ACCEPT);
  ui_test_utils::NavigateToURL(browser(), download_url);
  download_observer.WaitForFinished();

  // This should have changed the visible URL, but not the last committed one.
  ASSERT_EQ(download_url, active_tab->GetVisibleURL());
  ASSERT_EQ(GURL(chrome::kChromeUINewTabURL),
            active_tab->GetLastCommittedURL());

  // Make sure the same number of items is available in JS.
  int most_visited_count = -1;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "window.chrome.embeddedSearch.newTabPage.mostVisited.length",
      &most_visited_count));
  ASSERT_EQ(kDefaultMostVisitedItemCount, most_visited_count);

  // Get the ID of one item.
  int most_visited_rid = -1;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      active_tab, "window.chrome.embeddedSearch.newTabPage.mostVisited[0].rid",
      &most_visited_rid));

  // Since the current page is still an NTP, it should be possible to delete MV
  // items (as well as anything else that the embeddedSearch API allows).
  ASSERT_TRUE(content::ExecuteScript(
      active_tab,
      base::StringPrintf(
          "window.chrome.embeddedSearch.newTabPage.deleteMostVisitedItem(%d)",
          most_visited_rid)));
  observer.WaitForNumberOfItems(kDefaultMostVisitedItemCount - 1);
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, NTPRespectsBrowserLanguageSetting) {
  // If the platform cannot load the French locale (GetApplicationLocale() is
  // platform specific, and has been observed to fail on a small number of
  // platforms), abort the test.
  if (!local_ntp_test_utils::SwitchBrowserLanguageToFrench()) {
    LOG(ERROR) << "Failed switching to French language, aborting test.";
    return;
  }

  // Open a new tab.
  content::WebContents* active_tab = local_ntp_test_utils::OpenNewTab(
      browser(), GURL(chrome::kChromeUINewTabURL));

  // Verify that the NTP is in French.
  EXPECT_EQ(base::ASCIIToUTF16("Nouvel onglet"), active_tab->GetTitle());
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, GoogleNTPLoadsWithoutError) {
  // Open a new blank tab.
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));
  ASSERT_FALSE(search::IsInstantNTP(active_tab));

  // Attach a console observer, listening for any message ("*" pattern).
  content::ConsoleObserverDelegate console_observer(active_tab, "*");
  active_tab->SetDelegate(&console_observer);

  base::HistogramTester histograms;

  // Navigate to the NTP.
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());

  bool is_google = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.configData && !!window.configData.isGooglePage",
      &is_google));
  EXPECT_TRUE(is_google);

  // We shouldn't have gotten any console error messages.
  EXPECT_TRUE(console_observer.message().empty()) << console_observer.message();

  // Make sure load time metrics were recorded.
  histograms.ExpectTotalCount("NewTabPage.LoadTime", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.LocalNTP", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.LocalNTP.Google", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.MostVisited", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime.LocalNTP", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime.MostVisited", 1);

  // Make sure impression metrics were recorded. There should be 1 tile, the
  // default prepopulated TopSites (see history::PrepopulatedPage).
  histograms.ExpectTotalCount("NewTabPage.NumberOfTiles", 1);
  histograms.ExpectBucketCount("NewTabPage.NumberOfTiles", 1, 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression", 1);
  histograms.ExpectBucketCount("NewTabPage.SuggestionsImpression", 0, 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression.client", 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression.Thumbnail", 1);
  histograms.ExpectTotalCount("NewTabPage.TileTitle", 1);
  histograms.ExpectTotalCount("NewTabPage.TileTitle.client", 1);
  histograms.ExpectTotalCount("NewTabPage.TileType", 1);
  histograms.ExpectTotalCount("NewTabPage.TileType.client", 1);
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, NonGoogleNTPLoadsWithoutError) {
  local_ntp_test_utils::SetUserSelectedDefaultSearchProvider(
      browser()->profile(), "https://www.example.com",
      /*ntp_url=*/"");

  // Open a new blank tab.
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));
  ASSERT_FALSE(search::IsInstantNTP(active_tab));

  // Attach a console observer, listening for any message ("*" pattern).
  content::ConsoleObserverDelegate console_observer(active_tab, "*");
  active_tab->SetDelegate(&console_observer);

  base::HistogramTester histograms;

  // Navigate to the NTP.
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());

  bool is_google = false;
  ASSERT_TRUE(instant_test_utils::GetBoolFromJS(
      active_tab, "!!window.configData && !!window.configData.isGooglePage",
      &is_google));
  EXPECT_FALSE(is_google);

  // We shouldn't have gotten any console error messages.
  EXPECT_TRUE(console_observer.message().empty()) << console_observer.message();

  // Make sure load time metrics were recorded.
  histograms.ExpectTotalCount("NewTabPage.LoadTime", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.LocalNTP", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.LocalNTP.Other", 1);
  histograms.ExpectTotalCount("NewTabPage.LoadTime.MostVisited", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime.LocalNTP", 1);
  histograms.ExpectTotalCount("NewTabPage.TilesReceivedTime.MostVisited", 1);

  // Make sure impression metrics were recorded. There should be 1 tile, the
  // default prepopulated TopSites (see history::PrepopulatedPage).
  histograms.ExpectTotalCount("NewTabPage.NumberOfTiles", 1);
  histograms.ExpectBucketCount("NewTabPage.NumberOfTiles", 1, 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression", 1);
  histograms.ExpectBucketCount("NewTabPage.SuggestionsImpression", 0, 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression.client", 1);
  histograms.ExpectTotalCount("NewTabPage.SuggestionsImpression.Thumbnail", 1);
  histograms.ExpectTotalCount("NewTabPage.TileTitle", 1);
  histograms.ExpectTotalCount("NewTabPage.TileTitle.client", 1);
  histograms.ExpectTotalCount("NewTabPage.TileType", 1);
  histograms.ExpectTotalCount("NewTabPage.TileType.client", 1);
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, FrenchGoogleNTPLoadsWithoutError) {
  if (!local_ntp_test_utils::SwitchBrowserLanguageToFrench()) {
    LOG(ERROR) << "Failed switching to French language, aborting test.";
    return;
  }

  // Open a new blank tab.
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));
  ASSERT_FALSE(search::IsInstantNTP(active_tab));

  // Attach a console observer, listening for any message ("*" pattern).
  content::ConsoleObserverDelegate console_observer(active_tab, "*");
  active_tab->SetDelegate(&console_observer);

  // Navigate to the NTP and make sure it's actually in French.
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());
  ASSERT_EQ(base::ASCIIToUTF16("Nouvel onglet"), active_tab->GetTitle());

  // We shouldn't have gotten any console error messages.
  EXPECT_TRUE(console_observer.message().empty()) << console_observer.message();
}

// Tests that blink::UseCounter do not track feature usage for NTP activities.
// TODO(lunalu): remove this test when blink side use counter is removed
// (crbug.com/811948).
IN_PROC_BROWSER_TEST_F(LocalNTPTest, ShouldNotTrackBlinkUseCounterForNTP) {
  base::HistogramTester histogram_tester;
  const char kFeaturesHistogramName[] = "Blink.UseCounter.Features_Legacy";

  // Set up a test server, so we have some arbitrary non-NTP URL to navigate to.
  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  test_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(test_server.Start());
  const GURL other_url = test_server.GetURL("/simple.html");

  // Open an NTP.
  content::WebContents* active_tab = local_ntp_test_utils::OpenNewTab(
      browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // Expect no PageVisits count.
  EXPECT_EQ(nullptr,
            base::StatisticsRecorder::FindHistogram(kFeaturesHistogramName));
  // Navigate somewhere else in the same tab.
  ui_test_utils::NavigateToURL(browser(), other_url);
  ASSERT_FALSE(search::IsInstantNTP(active_tab));
  // Navigate back to NTP.
  content::TestNavigationObserver back_observer(active_tab);
  chrome::GoBack(browser(), WindowOpenDisposition::CURRENT_TAB);
  back_observer.Wait();
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // There should be exactly 1 count of PageVisits.
  histogram_tester.ExpectBucketCount(
      kFeaturesHistogramName,
      static_cast<int32_t>(blink::mojom::WebFeature::kPageVisits), 1);

  // Navigate forward to the non-NTP page.
  content::TestNavigationObserver fwd_observer(active_tab);
  chrome::GoForward(browser(), WindowOpenDisposition::CURRENT_TAB);
  fwd_observer.Wait();
  ASSERT_FALSE(search::IsInstantNTP(active_tab));
  // Navigate to a new NTP instance.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // There should be 2 counts of PageVisits.
  histogram_tester.ExpectBucketCount(
      kFeaturesHistogramName,
      static_cast<int32_t>(blink::mojom::WebFeature::kPageVisits), 2);
}

class LocalNTPRTLTest : public LocalNTPTest {
 public:
  LocalNTPRTLTest() {}

 private:
  void SetUpCommandLine(base::CommandLine* cmdline) override {
    cmdline->AppendSwitchASCII(switches::kForceUIDirection,
                               switches::kForceDirectionRTL);
  }
};

IN_PROC_BROWSER_TEST_F(LocalNTPRTLTest, RightToLeft) {
  // Open an NTP.
  content::WebContents* active_tab = local_ntp_test_utils::OpenNewTab(
      browser(), GURL(chrome::kChromeUINewTabURL));
  ASSERT_TRUE(search::IsInstantNTP(active_tab));
  // Check that the "dir" attribute on the main "html" element says "rtl".
  std::string dir;
  ASSERT_TRUE(instant_test_utils::GetStringFromJS(
      active_tab, "document.documentElement.dir", &dir));
  EXPECT_EQ("rtl", dir);
}

// Returns the RenderFrameHost corresponding to the most visited iframe in the
// given |tab|. |tab| must correspond to an NTP.
content::RenderFrameHost* GetMostVisitedIframe(content::WebContents* tab) {
  for (content::RenderFrameHost* frame : tab->GetAllFrames()) {
    if (frame->GetFrameName() == "mv-single") {
      return frame;
    }
  }
  return nullptr;
}

IN_PROC_BROWSER_TEST_F(LocalNTPTest, LoadsIframe) {
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());

  // Get the iframe and check that the tiles loaded correctly.
  content::RenderFrameHost* iframe = GetMostVisitedIframe(active_tab);

  // Get the total number of (non-empty) tiles from the iframe.
  int total_thumbs = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.mv-thumb').length", &total_thumbs));
  // Also get how many of the tiles succeeded and failed in loading their
  // thumbnail images.
  int succeeded_imgs = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.mv-thumb img').length",
      &succeeded_imgs));
  int failed_imgs = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.mv-thumb.failed-img').length",
      &failed_imgs));

  // First, sanity check that the numbers line up (none of the css classes was
  // renamed, etc).
  EXPECT_EQ(total_thumbs, succeeded_imgs + failed_imgs);

  // Since we're in a non-signed-in, fresh profile with no history, there should
  // be the default TopSites tiles (see history::PrepopulatedPage).
  // Check that there is at least one tile, and that all of them loaded their
  // images successfully.
  EXPECT_GT(total_thumbs, 0);
  EXPECT_EQ(total_thumbs, succeeded_imgs);
  EXPECT_EQ(0, failed_imgs);
}

class LocalNTPMDTest : public LocalNTPTest {
 public:
  LocalNTPMDTest()
      : LocalNTPTest({features::kUseGoogleLocalNtp, features::kNtpUIMd}) {}

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(LocalNTPMDTest, LoadsMDIframe) {
  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));
  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());

  // Get the iframe and check that the tiles loaded correctly.
  content::RenderFrameHost* iframe = GetMostVisitedIframe(active_tab);

  // Get the total number of (non-empty) tiles from the iframe and tiles with
  // thumbnails. There should be no thumbnails in Material Design.
  int total_thumbs = 0;
  int total_favicons = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.mv-thumb').length", &total_thumbs));
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.md-favicon').length",
      &total_favicons));
  // Also get how many of the tiles succeeded and failed in loading their
  // favicon images.
  int succeeded_favicons = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.md-favicon img').length",
      &succeeded_favicons));
  int failed_favicons = 0;
  ASSERT_TRUE(instant_test_utils::GetIntFromJS(
      iframe, "document.querySelectorAll('.md-favicon.failed-favicon').length",
      &failed_favicons));

  // First, sanity check that the numbers line up (none of the css classes was
  // renamed, etc).
  EXPECT_EQ(total_favicons, succeeded_favicons + failed_favicons);

  // Since we're in a non-signed-in, fresh profile with no history, there should
  // be the default TopSites tiles (see history::PrepopulatedPage).
  // Check that there is at least one tile, and that all of them loaded their
  // images successfully. Also check that no thumbnails have loaded.
  EXPECT_EQ(total_thumbs, 0);
  EXPECT_EQ(total_favicons, succeeded_favicons);
  EXPECT_EQ(0, failed_favicons);
}

// A minimal implementation of an interstitial page.
class TestInterstitialPageDelegate : public content::InterstitialPageDelegate {
 public:
  static void Show(content::WebContents* web_contents, const GURL& url) {
    // The InterstitialPage takes ownership of this object, and will delete it
    // when it gets destroyed itself.
    new TestInterstitialPageDelegate(web_contents, url);
  }

  ~TestInterstitialPageDelegate() override {}

 private:
  TestInterstitialPageDelegate(content::WebContents* web_contents,
                               const GURL& url) {
    // |page| takes ownership of |this|.
    content::InterstitialPage* page =
        content::InterstitialPage::Create(web_contents, true, url, this);
    page->Show();
  }

  std::string GetHTMLContents() override { return "<html></html>"; }

  DISALLOW_COPY_AND_ASSIGN(TestInterstitialPageDelegate);
};

// A navigation throttle that will create an interstitial for all pages except
// chrome-search:// ones (i.e. the local NTP).
class TestNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit TestNavigationThrottle(content::NavigationHandle* handle)
      : content::NavigationThrottle(handle), weak_ptr_factory_(this) {}

  static std::unique_ptr<NavigationThrottle> Create(
      content::NavigationHandle* handle) {
    return std::make_unique<TestNavigationThrottle>(handle);
  }

 private:
  ThrottleCheckResult WillStartRequest() override {
    const GURL& url = navigation_handle()->GetURL();
    if (url.SchemeIs(chrome::kChromeSearchScheme)) {
      return NavigationThrottle::PROCEED;
    }

    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindRepeating(&TestNavigationThrottle::ShowInterstitial,
                            weak_ptr_factory_.GetWeakPtr()));
    return NavigationThrottle::DEFER;
  }

  const char* GetNameForLogging() override { return "TestNavigationThrottle"; }

  void ShowInterstitial() {
    TestInterstitialPageDelegate::Show(navigation_handle()->GetWebContents(),
                                       navigation_handle()->GetURL());
  }

  base::WeakPtrFactory<TestNavigationThrottle> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationThrottle);
};

IN_PROC_BROWSER_TEST_F(LocalNTPTest, InterstitialsAreNotNTPs) {
  // Set up a test server, so we have some non-NTP URL to navigate to.
  net::EmbeddedTestServer test_server(net::EmbeddedTestServer::TYPE_HTTPS);
  test_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(test_server.Start());

  content::WebContents* active_tab =
      local_ntp_test_utils::OpenNewTab(browser(), GURL("about:blank"));

  content::TestNavigationThrottleInserter throttle_inserter(
      active_tab, base::BindRepeating(&TestNavigationThrottle::Create));

  local_ntp_test_utils::NavigateToNTPAndWaitUntilLoaded(browser());
  ASSERT_TRUE(search::IsInstantNTP(active_tab));

  // Navigate to some non-NTP URL, which will result in an interstitial.
  const GURL blocked_url = test_server.GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), blocked_url);
  content::WaitForInterstitialAttach(active_tab);
  ASSERT_TRUE(active_tab->ShowingInterstitialPage());
  ASSERT_EQ(blocked_url, active_tab->GetVisibleURL());
  // The interstitial is not an NTP (even though the committed URL may still
  // point to an NTP, see crbug.com/448486).
  EXPECT_FALSE(search::IsInstantNTP(active_tab));

  // Go back to the NTP.
  chrome::GoBack(browser(), WindowOpenDisposition::CURRENT_TAB);
  content::WaitForInterstitialDetach(active_tab);
  // Now the page should be an NTP again.
  EXPECT_TRUE(search::IsInstantNTP(active_tab));
}

}  // namespace
