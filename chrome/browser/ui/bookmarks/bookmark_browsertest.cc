// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/browser/url_and_title.h"
#include "components/bookmarks/test/bookmark_test_helpers.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using bookmarks::BookmarkModel;
using bookmarks::UrlAndTitle;

namespace {
const char kPersistBookmarkURL[] = "http://www.cnn.com/";
const char kPersistBookmarkTitle[] = "CNN";
} // namespace

class TestBookmarkTabHelperDelegate : public BookmarkTabHelperDelegate {
 public:
  TestBookmarkTabHelperDelegate()
      : starred_(false) {
  }
  ~TestBookmarkTabHelperDelegate() override {}

  void URLStarredChanged(content::WebContents*, bool starred) override {
    starred_ = starred;
  }
  bool is_starred() const { return starred_; }

 private:
  bool starred_;

  DISALLOW_COPY_AND_ASSIGN(TestBookmarkTabHelperDelegate);
};

class BookmarkBrowsertest : public InProcessBrowserTest {
 public:
  BookmarkBrowsertest() {}

  bool IsVisible() {
    return browser()->bookmark_bar_state() == BookmarkBar::SHOW;
  }

  static void CheckAnimation(Browser* browser, const base::Closure& quit_task) {
    if (!browser->window()->IsBookmarkBarAnimating())
      quit_task.Run();
  }

  base::TimeDelta WaitForBookmarkBarAnimationToFinish() {
    base::Time start(base::Time::Now());
    scoped_refptr<content::MessageLoopRunner> runner =
        new content::MessageLoopRunner;

    base::Timer timer(false, true);
    timer.Start(
        FROM_HERE,
        base::TimeDelta::FromMilliseconds(15),
        base::Bind(&CheckAnimation, browser(), runner->QuitClosure()));
    runner->Run();
    return base::Time::Now() - start;
  }

  BookmarkModel* WaitForBookmarkModel(Profile* profile) {
    BookmarkModel* bookmark_model =
        BookmarkModelFactory::GetForBrowserContext(profile);
    bookmarks::test::WaitForBookmarkModelToLoad(bookmark_model);
    return bookmark_model;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(BookmarkBrowsertest);
};

// Test of bookmark bar toggling, visibility, and animation.
IN_PROC_BROWSER_TEST_F(BookmarkBrowsertest, BookmarkBarVisibleWait) {
  ASSERT_FALSE(IsVisible());
  chrome::ExecuteCommand(browser(), IDC_SHOW_BOOKMARK_BAR);
  base::TimeDelta delay = WaitForBookmarkBarAnimationToFinish();
  LOG(INFO) << "Took " << delay.InMilliseconds() << " ms to show bookmark bar";
  ASSERT_TRUE(IsVisible());
  chrome::ExecuteCommand(browser(), IDC_SHOW_BOOKMARK_BAR);
  delay = WaitForBookmarkBarAnimationToFinish();
  LOG(INFO) << "Took " << delay.InMilliseconds() << " ms to hide bookmark bar";
  ASSERT_FALSE(IsVisible());
}

// Verify that bookmarks persist browser restart.
IN_PROC_BROWSER_TEST_F(BookmarkBrowsertest, PRE_Persist) {
  BookmarkModel* bookmark_model = WaitForBookmarkModel(browser()->profile());

  bookmarks::AddIfNotBookmarked(bookmark_model,
                                GURL(kPersistBookmarkURL),
                                base::ASCIIToUTF16(kPersistBookmarkTitle));
}

#if defined(THREAD_SANITIZER) || defined(OS_WIN)
// BookmarkBrowsertest.Persist fails under ThreadSanitizer on Linux, see
// http://crbug.com/340223.
// Also flaky under Windows. See crbug.com/813759.
#define MAYBE_Persist DISABLED_Persist
#else
#define MAYBE_Persist Persist
#endif
IN_PROC_BROWSER_TEST_F(BookmarkBrowsertest, MAYBE_Persist) {
  BookmarkModel* bookmark_model = WaitForBookmarkModel(browser()->profile());

  std::vector<UrlAndTitle> urls;
  bookmark_model->GetBookmarks(&urls);

  ASSERT_EQ(1u, urls.size());
  ASSERT_EQ(GURL(kPersistBookmarkURL), urls[0].url);
  ASSERT_EQ(base::ASCIIToUTF16(kPersistBookmarkTitle), urls[0].title);
}

#if !defined(OS_CHROMEOS)  // No multi-profile on ChromeOS.

// Sanity check that bookmarks from different profiles are separate.
// DISABLED_ because it regularly times out: http://crbug.com/159002.
IN_PROC_BROWSER_TEST_F(BookmarkBrowsertest, DISABLED_MultiProfile) {
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());

  BookmarkModel* bookmark_model1 = WaitForBookmarkModel(browser()->profile());

  ui_test_utils::BrowserAddedObserver observer;
  g_browser_process->profile_manager()->CreateMultiProfileAsync(
      base::string16(), std::string(), ProfileManager::CreateCallback(),
      std::string());
  Browser* browser2 = observer.WaitForSingleNewBrowser();
  BookmarkModel* bookmark_model2 = WaitForBookmarkModel(browser2->profile());

  bookmarks::AddIfNotBookmarked(bookmark_model1,
                                GURL(kPersistBookmarkURL),
                                base::ASCIIToUTF16(kPersistBookmarkTitle));
  std::vector<UrlAndTitle> urls1, urls2;
  bookmark_model1->GetBookmarks(&urls1);
  bookmark_model2->GetBookmarks(&urls2);
  ASSERT_EQ(1u, urls1.size());
  ASSERT_TRUE(urls2.empty());
}

#endif

// Flaky on Linux: http://crbug.com/504869.
#if defined(OS_LINUX)
#define MAYBE_HideStarOnNonbookmarkedInterstitial \
    DISABLED_HideStarOnNonbookmarkedInterstitial
#else
#define MAYBE_HideStarOnNonbookmarkedInterstitial \
    HideStarOnNonbookmarkedInterstitial
#endif
IN_PROC_BROWSER_TEST_F(BookmarkBrowsertest,
                       MAYBE_HideStarOnNonbookmarkedInterstitial) {
  // Start an HTTPS server with a certificate error.
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_MISMATCHED_NAME);
  https_server.ServeFilesFromSourceDirectory("chrome/test/data");
  ASSERT_TRUE(https_server.Start());
  ASSERT_TRUE(embedded_test_server()->Start());

  BookmarkModel* bookmark_model = WaitForBookmarkModel(browser()->profile());
  GURL bookmark_url = embedded_test_server()->GetURL("example.test", "/");
  bookmarks::AddIfNotBookmarked(bookmark_model,
                                bookmark_url,
                                base::ASCIIToUTF16("Bookmark"));

  TestBookmarkTabHelperDelegate bookmark_delegate;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  BookmarkTabHelper* tab_helper =
      BookmarkTabHelper::FromWebContents(web_contents);
  tab_helper->set_delegate(&bookmark_delegate);

  // Go to a bookmarked url. Bookmark star should show.
  ui_test_utils::NavigateToURL(browser(), bookmark_url);
  EXPECT_FALSE(web_contents->ShowingInterstitialPage());
  EXPECT_TRUE(bookmark_delegate.is_starred());

  // Now go to a non-bookmarked url which triggers an SSL warning. Bookmark
  // star should disappear.
  GURL error_url = https_server.GetURL("/");
  ui_test_utils::NavigateToURL(browser(), error_url);
  web_contents = browser()->tab_strip_model()->GetActiveWebContents();
  content::WaitForInterstitialAttach(web_contents);
  EXPECT_TRUE(web_contents->ShowingInterstitialPage());
  EXPECT_FALSE(bookmark_delegate.is_starred());

  // The delegate is required to outlive the tab helper.
  tab_helper->set_delegate(nullptr);
}
