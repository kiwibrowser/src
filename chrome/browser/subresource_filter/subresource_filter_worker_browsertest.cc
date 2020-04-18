// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/subresource_filter/subresource_filter_browser_test_harness.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace subresource_filter {

class SubresourceFilterWorkerFetchBrowserTest
    : public SubresourceFilterBrowserTest {
 public:
  SubresourceFilterWorkerFetchBrowserTest() {}

  ~SubresourceFilterWorkerFetchBrowserTest() override {}

 protected:
  void ClearTitle() {
    ASSERT_TRUE(content::ExecuteScript(web_contents()->GetMainFrame(),
                                       "document.title = \"\";"));
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterWorkerFetchBrowserTest);
};

IN_PROC_BROWSER_TEST_F(SubresourceFilterWorkerFetchBrowserTest, WorkerFetch) {
  const base::string16 fetch_succeeded_title =
      base::ASCIIToUTF16("FetchSucceeded");
  const base::string16 fetch_failed_title = base::ASCIIToUTF16("FetchFailed");
  GURL url(GetTestUrl("subresource_filter/worker_fetch.html"));
  ConfigureAsPhishingURL(url);

  ASSERT_NO_FATAL_FAILURE(SetRulesetToDisallowURLsWithPathSuffix(
      "suffix-that-does-not-match-anything"));
  {
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(),
        fetch_succeeded_title);
    title_watcher.AlsoWaitForTitle(fetch_failed_title);
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(fetch_succeeded_title, title_watcher.WaitAndGetTitle());
  }
  ClearTitle();
  ASSERT_NO_FATAL_FAILURE(
      SetRulesetToDisallowURLsWithPathSuffix("worker_fetch_data.txt"));
  {
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(),
        fetch_succeeded_title);
    title_watcher.AlsoWaitForTitle(fetch_failed_title);
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(fetch_failed_title, title_watcher.WaitAndGetTitle());
  }
  ClearTitle();
  // The main frame document should never be filtered.
  SetRulesetToDisallowURLsWithPathSuffix("worker_fetch.html");
  {
    content::TitleWatcher title_watcher(
        browser()->tab_strip_model()->GetActiveWebContents(),
        fetch_succeeded_title);
    title_watcher.AlsoWaitForTitle(fetch_failed_title);
    ui_test_utils::NavigateToURL(browser(), url);
    EXPECT_EQ(fetch_succeeded_title, title_watcher.WaitAndGetTitle());
  }
}

}  // namespace subresource_filter
