// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_view.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/devtools/devtools_window_testing.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tab_ui_helper.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view_observer.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/views/scoped_macviews_browser_mode.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"

class BrowserViewTest : public InProcessBrowserTest {
 public:
  BrowserViewTest() : InProcessBrowserTest(), devtools_(nullptr) {}

 protected:
  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  views::WebView* devtools_web_view() {
    return browser_view()->GetDevToolsWebViewForTest();
  }

  views::WebView* contents_web_view() {
    return browser_view()->contents_web_view();
  }

  void OpenDevToolsWindow(bool docked) {
    devtools_ =
        DevToolsWindowTesting::OpenDevToolsWindowSync(browser(), docked);
  }

  void CloseDevToolsWindow() {
    DevToolsWindowTesting::CloseDevToolsWindowSync(devtools_);
  }

  void SetDevToolsBounds(const gfx::Rect& bounds) {
    DevToolsWindowTesting::Get(devtools_)->SetInspectedPageBounds(bounds);
  }

  DevToolsWindow* devtools_;

 private:
  test::ScopedMacViewsBrowserMode views_mode_{true};

  DISALLOW_COPY_AND_ASSIGN(BrowserViewTest);
};

namespace {

// Used to simulate scenario in a crash. When WebContentsDestroyed() is invoked
// updates the navigation state of another tab.
class TestWebContentsObserver : public content::WebContentsObserver {
 public:
  TestWebContentsObserver(content::WebContents* source,
                          content::WebContents* other)
      : content::WebContentsObserver(source),
        other_(other) {}
  ~TestWebContentsObserver() override {}

  void WebContentsDestroyed() override {
    other_->NotifyNavigationStateChanged(static_cast<content::InvalidateTypes>(
        content::INVALIDATE_TYPE_URL | content::INVALIDATE_TYPE_LOAD));
  }

 private:
  content::WebContents* other_;

  DISALLOW_COPY_AND_ASSIGN(TestWebContentsObserver);
};

}  // namespace

// Verifies don't crash when CloseNow() is invoked with two tabs in a browser.
// Additionally when one of the tabs is destroyed NotifyNavigationStateChanged()
// is invoked on the other.
IN_PROC_BROWSER_TEST_F(BrowserViewTest, CloseWithTabs) {
  Browser* browser2 =
      new Browser(Browser::CreateParams(browser()->profile(), true));
  chrome::AddTabAt(browser2, GURL(), -1, true);
  chrome::AddTabAt(browser2, GURL(), -1, true);
  TestWebContentsObserver observer(
      browser2->tab_strip_model()->GetWebContentsAt(0),
      browser2->tab_strip_model()->GetWebContentsAt(1));
  BrowserView::GetBrowserViewForBrowser(browser2)->GetWidget()->CloseNow();
}

// Same as CloseWithTabs, but activates the first tab, which is the first tab
// BrowserView will destroy.
IN_PROC_BROWSER_TEST_F(BrowserViewTest, CloseWithTabsStartWithActive) {
  Browser* browser2 =
      new Browser(Browser::CreateParams(browser()->profile(), true));
  chrome::AddTabAt(browser2, GURL(), -1, true);
  chrome::AddTabAt(browser2, GURL(), -1, true);
  browser2->tab_strip_model()->ActivateTabAt(0, true);
  TestWebContentsObserver observer(
      browser2->tab_strip_model()->GetWebContentsAt(0),
      browser2->tab_strip_model()->GetWebContentsAt(1));
  BrowserView::GetBrowserViewForBrowser(browser2)->GetWidget()->CloseNow();
}

// Verifies that page and devtools WebViews are being correctly layed out
// when DevTools is opened/closed/updated/undocked.

// Flaky on Chrome OS.  http://crbug.com/693000
#if defined(OS_CHROMEOS)
#define MAYBE_DevToolsUpdatesBrowserWindow DISABLED_DevToolsUpdatesBrowserWindow
#else
#define MAYBE_DevToolsUpdatesBrowserWindow DevToolsUpdatesBrowserWindow
#endif
IN_PROC_BROWSER_TEST_F(BrowserViewTest, MAYBE_DevToolsUpdatesBrowserWindow) {
  gfx::Rect full_bounds =
      browser_view()->GetContentsContainerForTest()->GetLocalBounds();
  gfx::Rect small_bounds(10, 20, 30, 40);

  browser_view()->UpdateDevTools();
  EXPECT_FALSE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(full_bounds, contents_web_view()->bounds());

  // Docked.
  OpenDevToolsWindow(true);
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());

  SetDevToolsBounds(small_bounds);
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(small_bounds, contents_web_view()->bounds());

  browser_view()->UpdateDevTools();
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(small_bounds, contents_web_view()->bounds());

  CloseDevToolsWindow();
  EXPECT_FALSE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(full_bounds, contents_web_view()->bounds());

  browser_view()->UpdateDevTools();
  EXPECT_FALSE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(full_bounds, contents_web_view()->bounds());

  // Undocked.
  OpenDevToolsWindow(false);
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());

  SetDevToolsBounds(small_bounds);
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(small_bounds, contents_web_view()->bounds());

  browser_view()->UpdateDevTools();
  EXPECT_TRUE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(small_bounds, contents_web_view()->bounds());

  CloseDevToolsWindow();
  EXPECT_FALSE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(full_bounds, contents_web_view()->bounds());

  browser_view()->UpdateDevTools();
  EXPECT_FALSE(devtools_web_view()->web_contents());
  EXPECT_EQ(full_bounds, devtools_web_view()->bounds());
  EXPECT_EQ(full_bounds, contents_web_view()->bounds());
}

class BookmarkBarViewObserverImpl : public BookmarkBarViewObserver {
 public:
  BookmarkBarViewObserverImpl() : change_count_(0) {
  }

  int change_count() const { return change_count_; }
  void clear_change_count() { change_count_ = 0; }

  // BookmarkBarViewObserver:
  void OnBookmarkBarVisibilityChanged() override { change_count_++; }

 private:
  int change_count_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkBarViewObserverImpl);
};

// Verifies we don't unnecessarily change the visibility of the BookmarkBarView.
IN_PROC_BROWSER_TEST_F(BrowserViewTest, AvoidUnnecessaryVisibilityChanges) {
  // Create two tabs, the first empty and the second the ntp. Make it so the
  // BookmarkBarView isn't shown (meaning it'll only be shown when on the ntp).
  browser()->profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowBookmarkBar, false);
  GURL new_tab_url(chrome::kChromeUINewTabURL);
  chrome::AddTabAt(browser(), GURL(), -1, true);
  ui_test_utils::NavigateToURL(browser(), new_tab_url);

  ASSERT_TRUE(browser_view()->bookmark_bar());
  BookmarkBarViewObserverImpl observer;
  BookmarkBarView* bookmark_bar = browser_view()->bookmark_bar();
  bookmark_bar->AddObserver(&observer);
  EXPECT_TRUE(bookmark_bar->visible());

  // Go to empty tab. Bookmark bar should hide.
  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_FALSE(bookmark_bar->visible());
  EXPECT_EQ(1, observer.change_count());
  observer.clear_change_count();

  // Go to ntp tab. Bookmark bar should show.
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  EXPECT_TRUE(bookmark_bar->visible());
  EXPECT_EQ(1, observer.change_count());
  observer.clear_change_count();

  // Repeat with the bookmark bar always visible.
  browser()->profile()->GetPrefs()->SetBoolean(
      bookmarks::prefs::kShowBookmarkBar, true);
  browser()->tab_strip_model()->ActivateTabAt(1, true);
  EXPECT_TRUE(bookmark_bar->visible());
  observer.clear_change_count();

  browser()->tab_strip_model()->ActivateTabAt(0, true);
  EXPECT_TRUE(bookmark_bar->visible());
  EXPECT_EQ(0, observer.change_count());
  observer.clear_change_count();

  browser()->tab_strip_model()->ActivateTabAt(1, true);
  EXPECT_TRUE(bookmark_bar->visible());
  EXPECT_EQ(0, observer.change_count());
  observer.clear_change_count();

  browser_view()->bookmark_bar()->RemoveObserver(&observer);
}

// Launch the app, navigate to a page with a title, check that the tab title
// is set before load finishes and the throbber state updates when the title
// changes. Regression test for crbug.com/752266
IN_PROC_BROWSER_TEST_F(BrowserViewTest, TitleAndLoadState) {
  const base::string16 test_title(base::ASCIIToUTF16("Title Of Awesomeness"));
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  content::TitleWatcher title_watcher(contents, test_title);
  content::TestNavigationObserver navigation_watcher(
      contents, 1, content::MessageLoopRunner::QuitMode::DEFERRED);

  TabStrip* tab_strip = browser_view()->tabstrip();

  // Navigate without blocking.
  ui_test_utils::NavigateToURLWithDispositionBlockUntilNavigationsComplete(
      browser(),
      ui_test_utils::GetTestUrl(
          base::FilePath(base::FilePath::kCurrentDirectory),
          base::FilePath(FILE_PATH_LITERAL("title2.html"))),
      0, WindowOpenDisposition::CURRENT_TAB, ui_test_utils::BROWSER_TEST_NONE);
  EXPECT_TRUE(browser()->tab_strip_model()->TabsAreLoading());
  EXPECT_EQ(TabNetworkState::kWaiting,
            tab_strip->tab_at(0)->data().network_state);
  EXPECT_EQ(test_title, title_watcher.WaitAndGetTitle());
  EXPECT_TRUE(browser()->tab_strip_model()->TabsAreLoading());
  EXPECT_EQ(TabNetworkState::kLoading,
            tab_strip->tab_at(0)->data().network_state);

  // Now block for the navigation to complete.
  navigation_watcher.Wait();
  EXPECT_FALSE(browser()->tab_strip_model()->TabsAreLoading());
  EXPECT_EQ(TabNetworkState::kNone, tab_strip->tab_at(0)->data().network_state);
}

// Verifies a tab should show its favicon.
IN_PROC_BROWSER_TEST_F(BrowserViewTest, ShowFaviconInTab) {
  // Opens "chrome://version/" page, which uses default favicon.
  GURL version_url(chrome::kChromeUIVersionURL);
  ui_test_utils::NavigateToURL(browser(), version_url);
  auto* contents = browser()->tab_strip_model()->GetActiveWebContents();
  auto* helper = TabUIHelper::FromWebContents(contents);
  ASSERT_TRUE(helper);

  auto favicon = helper->GetFavicon();
  ASSERT_FALSE(favicon.IsEmpty());
}
