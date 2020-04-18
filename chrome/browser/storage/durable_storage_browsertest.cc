// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

class DurableStorageBrowserTest : public InProcessBrowserTest {
 public:
  DurableStorageBrowserTest() = default;
  ~DurableStorageBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine*) override;
  void SetUpOnMainThread() override;

 protected:
  content::RenderFrameHost* GetRenderFrameHost(Browser* browser) {
    return browser->tab_strip_model()->GetActiveWebContents()->GetMainFrame();
  }

  content::RenderFrameHost* GetRenderFrameHost() {
    return GetRenderFrameHost(browser());
  }

  void Bookmark(Browser* browser) {
    bookmarks::BookmarkModel* bookmark_model =
        BookmarkModelFactory::GetForBrowserContext(browser->profile());
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);
    bookmarks::AddIfNotBookmarked(bookmark_model, url_, base::ASCIIToUTF16(""));
  }

  void Bookmark() {
    Bookmark(browser());
  }

  GURL url_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DurableStorageBrowserTest);
};

void DurableStorageBrowserTest::SetUpCommandLine(
    base::CommandLine* command_line) {
  command_line->AppendSwitch(
              switches::kEnableExperimentalWebPlatformFeatures);
}

void DurableStorageBrowserTest::SetUpOnMainThread() {
  if (embedded_test_server()->Started())
    return;
  ASSERT_TRUE(embedded_test_server()->Start());
  url_ = embedded_test_server()->GetURL("/durable/durability-permissions.html");
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, QueryNonBookmarkedPage) {
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_FALSE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, RequestNonBookmarkedPage) {
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "requestPermission()", &is_persistent));
  EXPECT_FALSE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, QueryBookmarkedPage) {
  // Documents that the current behavior is to return "default" if script
  // hasn't requested the durable permission, even if it would be autogranted.
  Bookmark();
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_FALSE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, RequestBookmarkedPage) {
  Bookmark();
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "requestPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, BookmarkThenUnbookmark) {
  Bookmark();
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;

  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "requestPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);

  bookmarks::BookmarkModel* bookmark_model =
      BookmarkModelFactory::GetForBrowserContext(browser()->profile());
  bookmarks::RemoveAllBookmarks(bookmark_model, url_);

  // Unbookmarking doesn't change the permission.
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
  // Requesting after unbookmarking doesn't change the default box.
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "requestPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
  // Querying after requesting after unbookmarking still reports "granted".
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, FirstTabSeesResult) {
  ui_test_utils::NavigateToURL(browser(), url_);
  bool is_persistent = false;

  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_FALSE(is_persistent);

  chrome::NewTab(browser());
  ui_test_utils::NavigateToURL(browser(), url_);
  Bookmark();

  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "requestPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);

  browser()->tab_strip_model()->ActivateTabAt(0, false);
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(), "checkPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
}

IN_PROC_BROWSER_TEST_F(DurableStorageBrowserTest, Incognito) {
  Browser* browser = CreateIncognitoBrowser();
  ui_test_utils::NavigateToURL(browser, url_);

  Bookmark(browser);
  bool is_persistent = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(GetRenderFrameHost(browser),
                                                   "requestPermission()",
                                                   &is_persistent));
  EXPECT_TRUE(is_persistent);

  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      GetRenderFrameHost(browser), "checkPermission()", &is_persistent));
  EXPECT_TRUE(is_persistent);
}
