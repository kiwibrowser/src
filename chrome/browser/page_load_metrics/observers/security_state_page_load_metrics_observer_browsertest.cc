// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/security_state_page_load_metrics_observer.h"

#include <memory>

#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

using UkmEntry = ukm::builders::Security_SiteEngagement;

// A WebContentsObserver to allow waiting on a change in visible security state.
class SecurityStyleTestObserver : public content::WebContentsObserver {
 public:
  explicit SecurityStyleTestObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents) {}
  ~SecurityStyleTestObserver() override {}

  void DidChangeVisibleSecurityState() override { run_loop_.Quit(); }

  void WaitForDidChangeVisibleSecurityState() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  DISALLOW_COPY_AND_ASSIGN(SecurityStyleTestObserver);
};

class SecurityStatePageLoadMetricsBrowserTest : public InProcessBrowserTest {
 public:
  SecurityStatePageLoadMetricsBrowserTest() {}
  ~SecurityStatePageLoadMetricsBrowserTest() override {}

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();
    test_ukm_recorder_ = std::make_unique<ukm::TestAutoSetUkmRecorder>();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

 protected:
  void StartHttpsServer(net::EmbeddedTestServer::ServerCertificate cert) {
    https_test_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTPS);
    https_test_server_->SetSSLConfig(cert);
    https_test_server_->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_test_server_->Start());
  }

  void StartHttpServer() {
    http_test_server_ = std::make_unique<net::EmbeddedTestServer>(
        net::EmbeddedTestServer::TYPE_HTTP);
    http_test_server_->ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(http_test_server_->Start());
  }

  void CloseAllTabs() {
    TabStripModel* tab_strip_model = browser()->tab_strip_model();
    content::WebContentsDestroyedWatcher destroyed_watcher(
        tab_strip_model->GetActiveWebContents());
    tab_strip_model->CloseAllTabs();
    destroyed_watcher.Wait();
  }

  // Checks UKM results for a specific URL and metric.
  void ExpectMetricForUrl(const GURL& url,
                          const char* metric_name,
                          int64_t expected_value) {
    // Find last entry matching |url|.
    const ukm::mojom::UkmEntry* last = nullptr;
    for (auto* entry :
         test_ukm_recorder_->GetEntriesByName(UkmEntry::kEntryName)) {
      auto* source = test_ukm_recorder_->GetSourceForSourceId(entry->source_id);
      if (source && source->url() == url)
        last = entry;
    }
    ASSERT_TRUE(last);
    if (last) {
      test_ukm_recorder_->ExpectEntrySourceHasUrl(last, url);
      test_ukm_recorder_->ExpectEntryMetric(last, metric_name, expected_value);
    }
  }

  size_t CountUkmEntries() {
    return test_ukm_recorder_->GetEntriesByName(UkmEntry::kEntryName).size();
  }

  base::HistogramTester* histogram_tester() const {
    return histogram_tester_.get();
  }
  net::EmbeddedTestServer* https_test_server() {
    return https_test_server_.get();
  }
  net::EmbeddedTestServer* http_test_server() {
    return http_test_server_.get();
  }

 private:
  std::unique_ptr<base::HistogramTester> histogram_tester_;
  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_ukm_recorder_;
  std::unique_ptr<net::EmbeddedTestServer> https_test_server_;
  std::unique_ptr<net::EmbeddedTestServer> http_test_server_;

  DISALLOW_COPY_AND_ASSIGN(SecurityStatePageLoadMetricsBrowserTest);
};

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest, Simple_Https) {
  StartHttpsServer(net::EmbeddedTestServer::CERT_OK);
  GURL url = https_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  CloseAllTabs();

  // Site Engagement metrics.
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      1);
  EXPECT_EQ(1u, CountUkmEntries());
  ExpectMetricForUrl(url, UkmEntry::kInitialSecurityLevelName,
                     security_state::SECURE);
  ExpectMetricForUrl(url, UkmEntry::kFinalSecurityLevelName,
                     security_state::SECURE);

  // Navigation metrics.
  histogram_tester()->ExpectUniqueSample(
      SecurityStatePageLoadMetricsObserver::
          GetPageEndReasonHistogramNameForTesting(security_state::SECURE),
      page_load_metrics::END_CLOSE, 1);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest, Simple_Http) {
  StartHttpServer();
  GURL url = http_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  CloseAllTabs();

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(1u, CountUkmEntries());
  ExpectMetricForUrl(url, UkmEntry::kInitialSecurityLevelName,
                     security_state::NONE);
  ExpectMetricForUrl(url, UkmEntry::kFinalSecurityLevelName,
                     security_state::NONE);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest, ReloadPage) {
  StartHttpsServer(net::EmbeddedTestServer::CERT_OK);
  GURL url = https_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  EXPECT_TRUE(content::WaitForLoadStop(contents));

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      1);
  EXPECT_EQ(1u, CountUkmEntries());
  ExpectMetricForUrl(url, UkmEntry::kInitialSecurityLevelName,
                     security_state::SECURE);
  ExpectMetricForUrl(url, UkmEntry::kFinalSecurityLevelName,
                     security_state::SECURE);

  histogram_tester()->ExpectUniqueSample(
      SecurityStatePageLoadMetricsObserver::
          GetPageEndReasonHistogramNameForTesting(security_state::SECURE),
      page_load_metrics::END_RELOAD, 1);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest, OtherScheme) {
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIVersionURL));
  CloseAllTabs();
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(0u, CountUkmEntries());

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetPageEndReasonHistogramNameForTesting(security_state::NONE),
      0);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest, MixedContent) {
  StartHttpsServer(net::EmbeddedTestServer::CERT_OK);
  GURL url = https_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);

  content::WebContents* tab =
      browser()->tab_strip_model()->GetActiveWebContents();
  SecurityStyleTestObserver observer(tab);
  ASSERT_TRUE(content::ExecuteScript(
      browser()->tab_strip_model()->GetActiveWebContents(),
      "var img = document.createElement('img'); "
      "img.src = 'http://example.com/image.png'; "
      "document.body.appendChild(img);"));
  observer.WaitForDidChangeVisibleSecurityState();
  CloseAllTabs();

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      1);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(1u, CountUkmEntries());
  ExpectMetricForUrl(url, UkmEntry::kInitialSecurityLevelName,
                     security_state::SECURE);
  ExpectMetricForUrl(url, UkmEntry::kFinalSecurityLevelName,
                     security_state::NONE);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest,
                       UncommittedLoadWithError) {
  // Make HTTPS server cause an interstitial.
  StartHttpsServer(net::EmbeddedTestServer::CERT_EXPIRED);
  GURL url = https_test_server()->GetURL("/simple.html");
  ui_test_utils::NavigateToURL(browser(), url);
  CloseAllTabs();

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(0u, CountUkmEntries());
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest,
                       HostDoesNotExist) {
  GURL url("http://nonexistent.test/page.html");
  ui_test_utils::NavigateToURL(browser(), url);

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(0u, CountUkmEntries());

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetPageEndReasonHistogramNameForTesting(security_state::SECURE),
      0);
}

IN_PROC_BROWSER_TEST_F(SecurityStatePageLoadMetricsBrowserTest,
                       Navigate_Both_NonHtmlMainResource) {
  StartHttpServer();
  StartHttpsServer(net::EmbeddedTestServer::CERT_OK);

  GURL http_url = http_test_server()->GetURL("/circle.svg");
  GURL https_url = https_test_server()->GetURL("/circle.svg");
  ui_test_utils::NavigateToURL(browser(), http_url);
  ui_test_utils::NavigateToURL(browser(), https_url);
  CloseAllTabs();

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementFinalHistogramNameForTesting(security_state::SECURE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::NONE),
      0);
  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetEngagementDeltaHistogramNameForTesting(security_state::SECURE),
      0);
  EXPECT_EQ(0u, CountUkmEntries());

  histogram_tester()->ExpectTotalCount(
      SecurityStatePageLoadMetricsObserver::
          GetPageEndReasonHistogramNameForTesting(security_state::SECURE),
      0);
}
