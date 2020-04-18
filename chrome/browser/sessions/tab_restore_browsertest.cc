// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/sessions/session_restore_test_helper.h"
#include "chrome/browser/sessions/tab_restore_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_live_tab_context.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/find_bar/find_notification_details.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/keep_alive_registry/keep_alive_types.h"
#include "components/keep_alive_registry/scoped_keep_alive.h"
#include "components/sessions/core/tab_restore_service.h"
#include "components/sessions/core/tab_restore_service_observer.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_SESSION_SERVICE)
#include "chrome/browser/sessions/tab_loader.h"
#endif  // BUILDFLAG(ENABLE_SESSION_SERVICE)

// Class used to run a message loop waiting for the TabRestoreService to finish
// loading. Does nothing if the TabRestoreService was already loaded.
class WaitForLoadObserver : public sessions::TabRestoreServiceObserver {
 public:
  explicit WaitForLoadObserver(Browser* browser)
      : tab_restore_service_(
          TabRestoreServiceFactory::GetForProfile(browser->profile())),
        do_wait_(!tab_restore_service_->IsLoaded()) {
    if (do_wait_)
      tab_restore_service_->AddObserver(this);
  }

  ~WaitForLoadObserver() override {
    if (do_wait_)
      tab_restore_service_->RemoveObserver(this);
  }

  void Wait() {
    if (do_wait_)
      run_loop_.Run();
  }

 private:
  // Overridden from TabRestoreServiceObserver:
  void TabRestoreServiceDestroyed(
      sessions::TabRestoreService* service) override {}
  void TabRestoreServiceLoaded(sessions::TabRestoreService* service) override {
    DCHECK(do_wait_);
    run_loop_.Quit();
  }

  sessions::TabRestoreService* tab_restore_service_;
  const bool do_wait_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(WaitForLoadObserver);
};

class TabRestoreTest : public InProcessBrowserTest {
 public:
  TabRestoreTest() : active_browser_list_(NULL) {
    url1_ = ui_test_utils::GetTestUrl(
        base::FilePath().AppendASCII("session_history"),
        base::FilePath().AppendASCII("bot1.html"));
    url2_ = ui_test_utils::GetTestUrl(
        base::FilePath().AppendASCII("session_history"),
        base::FilePath().AppendASCII("bot2.html"));
  }

 protected:
  void SetUpOnMainThread() override {
    active_browser_list_ = BrowserList::GetInstance();
  }

  Browser* GetBrowser(int index) {
    CHECK(static_cast<int>(active_browser_list_->size()) > index);
    return active_browser_list_->get(index);
  }

  // Adds tabs to the given browser, all navigated to url1_. Returns
  // the final number of tabs.
  int AddSomeTabs(Browser* browser, int how_many) {
    int starting_tab_count = browser->tab_strip_model()->count();

    for (int i = 0; i < how_many; ++i) {
      ui_test_utils::NavigateToURLWithDisposition(
          browser, url1_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
          ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
    }
    int tab_count = browser->tab_strip_model()->count();
    EXPECT_EQ(starting_tab_count + how_many, tab_count);
    return tab_count;
  }

  void CloseTab(int index) {
    content::WebContentsDestroyedWatcher destroyed_watcher(
        browser()->tab_strip_model()->GetWebContentsAt(index));
    browser()->tab_strip_model()->CloseWebContentsAt(
        index, TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
    destroyed_watcher.Wait();
  }

  // Uses the undo-close-tab accelerator to undo a close-tab or close-window
  // operation. The newly restored tab is expected to appear in the
  // window at index |expected_window_index|, at the |expected_tabstrip_index|,
  // and to be active. If |expected_window_index| is equal to the number of
  // current windows, the restored tab is expected to be created in a new
  // window (since the index is 0-based).
  void RestoreTab(int expected_window_index, int expected_tabstrip_index) {
    int window_count = static_cast<int>(active_browser_list_->size());
    ASSERT_GT(window_count, 0);

    bool expect_new_window = (expected_window_index == window_count);

    Browser* browser;
    if (expect_new_window) {
      browser = active_browser_list_->get(0);
    } else {
      browser = GetBrowser(expected_window_index);
    }
    int tab_count = browser->tab_strip_model()->count();
    ASSERT_GT(tab_count, 0);

    // Restore the tab.
    content::WindowedNotificationObserver tab_added_observer(
        chrome::NOTIFICATION_TAB_PARENTED,
        content::NotificationService::AllSources());
    content::WindowedNotificationObserver tab_loaded_observer(
        content::NOTIFICATION_LOAD_STOP,
        content::NotificationService::AllSources());
    {
      WaitForLoadObserver waiter(browser);
      chrome::RestoreTab(browser);
      waiter.Wait();
    }
    tab_added_observer.Wait();
    tab_loaded_observer.Wait();

    if (expect_new_window) {
      int new_window_count = static_cast<int>(active_browser_list_->size());
      EXPECT_EQ(++window_count, new_window_count);
      browser = GetBrowser(expected_window_index);
    } else {
      EXPECT_EQ(++tab_count, browser->tab_strip_model()->count());
    }

    // Get a handle to the restored tab.
    ASSERT_GT(browser->tab_strip_model()->count(), expected_tabstrip_index);

    // Ensure that the tab and window are active.
    EXPECT_EQ(expected_tabstrip_index,
              browser->tab_strip_model()->active_index());
  }

  void GoBack(Browser* browser) {
    content::WindowedNotificationObserver observer(
        content::NOTIFICATION_LOAD_STOP,
        content::NotificationService::AllSources());
    chrome::GoBack(browser, WindowOpenDisposition::CURRENT_TAB);
    observer.Wait();
  }

  void EnsureTabFinishedRestoring(content::WebContents* tab) {
    content::NavigationController* controller = &tab->GetController();
    if (!controller->NeedsReload() && !controller->GetPendingEntry() &&
        !controller->GetWebContents()->IsLoading())
      return;

    content::WindowedNotificationObserver observer(
        content::NOTIFICATION_LOAD_STOP,
        content::Source<content::NavigationController>(controller));
    observer.Wait();
  }

  GURL url1_;
  GURL url2_;

  const BrowserList* active_browser_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TabRestoreTest);
};

// Close the end tab in the current window, then restore it. The tab should be
// in its original position, and active.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, Basic) {
  int starting_tab_count = browser()->tab_strip_model()->count();
  int tab_count = AddSomeTabs(browser(), 1);

  int closed_tab_index = tab_count - 1;
  CloseTab(closed_tab_index);
  EXPECT_EQ(starting_tab_count, browser()->tab_strip_model()->count());

  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, closed_tab_index));

  // And make sure everything looks right.
  EXPECT_EQ(starting_tab_count + 1, browser()->tab_strip_model()->count());
  EXPECT_EQ(closed_tab_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(url1_,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Close a tab not at the end of the current window, then restore it. The tab
// should be in its original position, and active.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, MiddleTab) {
  int starting_tab_count = browser()->tab_strip_model()->count();
  AddSomeTabs(browser(), 3);

  // Close one in the middle
  int closed_tab_index = starting_tab_count + 1;
  CloseTab(closed_tab_index);
  EXPECT_EQ(starting_tab_count + 2, browser()->tab_strip_model()->count());

  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, closed_tab_index));

  // And make sure everything looks right.
  EXPECT_EQ(starting_tab_count + 3, browser()->tab_strip_model()->count());
  EXPECT_EQ(closed_tab_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(url1_,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Close a tab, switch windows, then restore the tab. The tab should be in its
// original window and position, and active.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreToDifferentWindow) {
  int starting_tab_count = browser()->tab_strip_model()->count();
  AddSomeTabs(browser(), 3);

  // Close one in the middle
  int closed_tab_index = starting_tab_count + 1;
  CloseTab(closed_tab_index);
  EXPECT_EQ(starting_tab_count + 2, browser()->tab_strip_model()->count());

  // Create a new browser.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Restore tab into original browser.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, closed_tab_index));

  // And make sure everything looks right.
  EXPECT_EQ(starting_tab_count + 3, browser()->tab_strip_model()->count());
  EXPECT_EQ(closed_tab_index, browser()->tab_strip_model()->active_index());
  EXPECT_EQ(url1_,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Close a tab, open a new window, close the first window, then restore the
// tab. It should be in a new window.
// If this becomes flaky, use http://crbug.com/14774
IN_PROC_BROWSER_TEST_F(TabRestoreTest, DISABLED_BasicRestoreFromClosedWindow) {
  // Navigate to url1 then url2.
  ui_test_utils::NavigateToURL(browser(), url1_);
  ui_test_utils::NavigateToURL(browser(), url2_);

  // Create a new browser.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Close the final tab in the first browser.
  content::WindowedNotificationObserver window_observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  CloseTab(0);
  window_observer.Wait();

  ASSERT_NO_FATAL_FAILURE(RestoreTab(1, 0));

  // Tab should be in a new window.
  Browser* browser = GetBrowser(1);
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();
  // And make sure the URLs match.
  EXPECT_EQ(url2_, web_contents->GetURL());
  GoBack(browser);
  EXPECT_EQ(url1_, web_contents->GetURL());
}

#if defined(OS_WIN)
// Flakily times out: http://crbug.com/171503
#define MAYBE_DontLoadRestoredTab DISABLED_DontLoadRestoredTab
#else
#define MAYBE_DontLoadRestoredTab DontLoadRestoredTab
#endif

// Restore a tab then make sure it doesn't restore again.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, MAYBE_DontLoadRestoredTab) {
  // Add two tabs
  int starting_tab_count = browser()->tab_strip_model()->count();
  AddSomeTabs(browser(), 2);
  ASSERT_EQ(browser()->tab_strip_model()->count(), starting_tab_count + 2);

  // Close one of them.
  CloseTab(0);
  ASSERT_EQ(browser()->tab_strip_model()->count(), starting_tab_count + 1);

  // Restore it.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, 0));
  ASSERT_EQ(browser()->tab_strip_model()->count(), starting_tab_count + 2);

  // Make sure that there's nothing else to restore.
  ASSERT_EQ(chrome::GetRestoreTabType(browser()),
            TabStripModelDelegate::RESTORE_NONE);
}

// Open a window with multiple tabs, close a tab, then close the window.
// Restore both and make sure the tab goes back into the window.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreWindowAndTab) {
  int starting_tab_count = browser()->tab_strip_model()->count();
  AddSomeTabs(browser(), 3);

  // Close one in the middle
  int closed_tab_index = starting_tab_count + 1;
  CloseTab(closed_tab_index);
  EXPECT_EQ(starting_tab_count + 2, browser()->tab_strip_model()->count());

  // Create a new browser.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Close the first browser.
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(1u, active_browser_list_->size());

  // Restore the first window. The expected_tabstrip_index (second argument)
  // indicates the expected active tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(1, starting_tab_count + 1));
  Browser* browser = GetBrowser(1);
  EXPECT_EQ(starting_tab_count + 2, browser->tab_strip_model()->count());

  // Restore the closed tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(1, closed_tab_index));
  EXPECT_EQ(starting_tab_count + 3, browser->tab_strip_model()->count());
  EXPECT_EQ(url1_,
            browser->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Open a window with two tabs, close both (closing the window), then restore
// both. Make sure both restored tabs are in the same window.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreIntoSameWindow) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url1_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  // Navigate the rightmost one to url2_ for easier identification.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Create a new browser.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Close all but one tab in the first browser, left to right.
  while (browser()->tab_strip_model()->count() > 1)
    CloseTab(0);

  // Close the last tab, closing the browser.
  content::WindowedNotificationObserver observer(
      chrome::NOTIFICATION_BROWSER_CLOSED,
      content::NotificationService::AllSources());
  CloseTab(0);
  observer.Wait();
  EXPECT_EQ(1u, active_browser_list_->size());

  // Restore the last-closed tab into a new window.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(1, 0));
  Browser* browser = GetBrowser(1);
  EXPECT_EQ(1, browser->tab_strip_model()->count());
  EXPECT_EQ(url2_,
            browser->tab_strip_model()->GetActiveWebContents()->GetURL());

  // Restore the next-to-last-closed tab into the same window.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(1, 0));
  EXPECT_EQ(2, browser->tab_strip_model()->count());
  EXPECT_EQ(url1_,
            browser->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// Open a window with two tabs, close the window, then restore the window.
// Ensure that the restored window has the expected bounds.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreWindowBounds) {
  // Create a browser window with two tabs.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url1_, WindowOpenDisposition::CURRENT_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Create a second browser window.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Deliberately change the bounds of the first window to something different.
  gfx::Rect bounds = browser()->window()->GetBounds();
  bounds.set_width(640);
  bounds.set_height(480);
  bounds.Offset(20, 20);
  browser()->window()->SetBounds(bounds);
  gfx::Rect bounds2 = browser()->window()->GetBounds();
  EXPECT_EQ(bounds, bounds2);

  // Close the first window.
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(1u, active_browser_list_->size());

  // Check that the TabRestoreService has the contents of the closed window and
  // the correct bounds.
  Browser* browser = GetBrowser(0);
  sessions::TabRestoreService* service =
      TabRestoreServiceFactory::GetForProfile(browser->profile());
  const sessions::TabRestoreService::Entries& entries = service->entries();
  EXPECT_EQ(1u, entries.size());
  sessions::TabRestoreService::Entry* entry = entries.front().get();
  ASSERT_EQ(sessions::TabRestoreService::WINDOW, entry->type);
  sessions::TabRestoreService::Window* entry_win =
      static_cast<sessions::TabRestoreService::Window*>(entry);
  EXPECT_EQ(bounds, entry_win->bounds);
  auto& tabs = entry_win->tabs;
  EXPECT_EQ(2u, tabs.size());

  // Restore the window. Ensure that a second window is created, that is has 2
  // tabs, and that it has the expected bounds.
  service->RestoreMostRecentEntry(browser->live_tab_context());
  EXPECT_EQ(2u, active_browser_list_->size());
  browser = GetBrowser(1);
  EXPECT_EQ(2, browser->tab_strip_model()->count());
  // We expect the overridden bounds to the browser window to have been
  // specified at window creation. The actual bounds of the window itself may
  // change as the browser refuses to create windows that are offscreen, so will
  // adjust bounds slightly in some cases.
  EXPECT_EQ(bounds, browser->override_bounds());
}

// Open a window with two tabs, close both (closing the window), then restore
// one by ID. Guards against regression of crbug.com/622752.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreTabFromClosedWindowByID) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url1_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Create a new browser.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(2u, active_browser_list_->size());

  // Close the window.
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(1u, active_browser_list_->size());

  // Check that the TabRestoreService has the contents of the closed window.
  Browser* browser = GetBrowser(0);
  sessions::TabRestoreService* service =
      TabRestoreServiceFactory::GetForProfile(browser->profile());
  const sessions::TabRestoreService::Entries& entries = service->entries();
  EXPECT_EQ(1u, entries.size());
  sessions::TabRestoreService::Entry* entry = entries.front().get();
  ASSERT_EQ(sessions::TabRestoreService::WINDOW, entry->type);
  sessions::TabRestoreService::Window* entry_win =
      static_cast<sessions::TabRestoreService::Window*>(entry);
  auto& tabs = entry_win->tabs;
  EXPECT_EQ(3u, tabs.size());
  EXPECT_EQ(url::kAboutBlankURL, tabs[0]->navigations.front().virtual_url());
  EXPECT_EQ(url1_, tabs[1]->navigations.front().virtual_url());
  EXPECT_EQ(url2_, tabs[2]->navigations.front().virtual_url());
  EXPECT_EQ(2, entry_win->selected_tab_index);

  // Find the Tab to restore.
  SessionID tab_id_to_restore = SessionID::InvalidValue();
  bool found_tab_to_restore = false;
  for (const auto& tab_ptr : tabs) {
    auto& tab = *tab_ptr;
    if (tab.navigations[tab.current_navigation_index].virtual_url() == url1_) {
      tab_id_to_restore = tab.id;
      found_tab_to_restore = true;
      break;
    }
  }
  ASSERT_TRUE(found_tab_to_restore);

  // Restore the tab into the current window.
  EXPECT_EQ(1, browser->tab_strip_model()->count());
  content::WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_PARENTED,
      content::NotificationService::AllSources());
  content::WindowedNotificationObserver tab_loaded_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());
  service->RestoreEntryById(browser->live_tab_context(), tab_id_to_restore,
                            WindowOpenDisposition::NEW_FOREGROUND_TAB);
  tab_added_observer.Wait();
  tab_loaded_observer.Wait();

  // Check that the tab was correctly restored.
  EXPECT_EQ(2, browser->tab_strip_model()->count());
  EXPECT_EQ(url1_,
            browser->tab_strip_model()->GetActiveWebContents()->GetURL());

  // Check that the window entry was adjusted.
  EXPECT_EQ(2u, tabs.size());
  EXPECT_EQ(url::kAboutBlankURL, tabs[0]->navigations.front().virtual_url());
  EXPECT_EQ(url2_, tabs[1]->navigations.front().virtual_url());
  EXPECT_EQ(1, entry_win->selected_tab_index);
}

// Tests that a duplicate history entry is not created when we restore a page
// to an existing SiteInstance.  (Bug 1230446)
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreWithExistingSiteInstance) {
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL http_url1(embedded_test_server()->GetURL("/title1.html"));
  GURL http_url2(embedded_test_server()->GetURL("/title2.html"));
  int tab_count = browser()->tab_strip_model()->count();

  // Add a tab
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), http_url1, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_EQ(++tab_count, browser()->tab_strip_model()->count());

  // Navigate to another same-site URL.
  content::WebContents* tab =
      browser()->tab_strip_model()->GetWebContentsAt(tab_count - 1);
  content::WindowedNotificationObserver observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());
  static_cast<content::WebContentsDelegate*>(browser())->OpenURLFromTab(
      tab, content::OpenURLParams(http_url2, content::Referrer(),
                                  WindowOpenDisposition::CURRENT_TAB,
                                  ui::PAGE_TRANSITION_TYPED, false));
  observer.Wait();

  // Close the tab.
  CloseTab(1);

  // Create a new tab to the original site.  Assuming process-per-site is
  // enabled, this will ensure that the SiteInstance used by the restored tab
  // will already exist when the restore happens.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), http_url2, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Restore the closed tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, tab_count - 1));

  // And make sure the URLs match.
  EXPECT_EQ(http_url2,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
  GoBack(browser());
  EXPECT_EQ(http_url1,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
}

// See crbug.com/248574
#if defined(OS_WIN)
#define MAYBE_RestoreCrossSiteWithExistingSiteInstance \
  DISABLED_RestoreCrossSiteWithExistingSiteInstance
#else
#define MAYBE_RestoreCrossSiteWithExistingSiteInstance \
  RestoreCrossSiteWithExistingSiteInstance
#endif

// Tests that the SiteInstances used for entries in a restored tab's history
// are given appropriate max page IDs, even if the renderer for the entry
// already exists.  (Bug 1204135)
IN_PROC_BROWSER_TEST_F(TabRestoreTest,
                       MAYBE_RestoreCrossSiteWithExistingSiteInstance) {
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL http_url1(embedded_test_server()->GetURL("/title1.html"));
  GURL http_url2(embedded_test_server()->GetURL("/title2.html"));

  int tab_count = browser()->tab_strip_model()->count();

  // Add a tab
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), http_url1, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_EQ(++tab_count, browser()->tab_strip_model()->count());

  // Navigate to more URLs, then a cross-site URL.
  ui_test_utils::NavigateToURL(browser(), http_url2);
  ui_test_utils::NavigateToURL(browser(), http_url1);
  ui_test_utils::NavigateToURL(browser(), url1_);

  // Close the tab.
  CloseTab(1);

  // Create a new tab to the original site.  Assuming process-per-site is
  // enabled, this will ensure that the SiteInstance will already exist when
  // the user clicks Back in the restored tab.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), http_url2, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Restore the closed tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, tab_count - 1));

  // And make sure the URLs match.
  EXPECT_EQ(url1_,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
  GoBack(browser());
  EXPECT_EQ(http_url1,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());

  // Navigating to a new URL should clear the forward list, because the max
  // page ID of the renderer should have been updated when we restored the tab.
  ui_test_utils::NavigateToURL(browser(), http_url2);
  EXPECT_FALSE(chrome::CanGoForward(browser()));
  EXPECT_EQ(http_url2,
            browser()->tab_strip_model()->GetActiveWebContents()->GetURL());
}

IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreWindow) {
  // Create a new window.
  size_t window_count = active_browser_list_->size();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUINewTabURL),
      WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  EXPECT_EQ(++window_count, active_browser_list_->size());

  // Create two more tabs, one with url1, the other url2.
  int initial_tab_count = browser()->tab_strip_model()->count();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url1_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2_, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Close the window.
  CloseBrowserSynchronously(browser());
  EXPECT_EQ(window_count - 1, active_browser_list_->size());

  // Restore the window.
  content::WindowedNotificationObserver open_window_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  content::WindowedNotificationObserver load_stop_observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());
  chrome::RestoreTab(active_browser_list_->get(0));
  open_window_observer.Wait();
  EXPECT_EQ(window_count, active_browser_list_->size());

  Browser* browser = GetBrowser(1);
  EXPECT_EQ(initial_tab_count + 2, browser->tab_strip_model()->count());
  load_stop_observer.Wait();

  EXPECT_EQ(initial_tab_count + 1, browser->tab_strip_model()->active_index());
  content::WebContents* restored_tab =
      browser->tab_strip_model()->GetWebContentsAt(initial_tab_count + 1);
  EnsureTabFinishedRestoring(restored_tab);
  EXPECT_EQ(url2_, restored_tab->GetURL());

  restored_tab =
      browser->tab_strip_model()->GetWebContentsAt(initial_tab_count);
  EnsureTabFinishedRestoring(restored_tab);
  EXPECT_EQ(url1_, restored_tab->GetURL());
}

// https://crbug.com/835305: Timeout flakiness on Win7 Tests (dbg)(1) bot and
// PASS/FAIL flakiness on Linux Chromium OS ASan LSan Tests (1) bot.
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER) || \
    (defined(OS_WIN) && !defined(NDEBUG))
#define MAYBE_RestoreTabWithSpecialURL DISABLED_RestoreTabWithSpecialURL
#else
#define MAYBE_RestoreTabWithSpecialURL RestoreTabWithSpecialURL
#endif

// Restore tab with special URL chrome://credits/ and make sure the page loads
// properly after restore. See http://crbug.com/31905.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, MAYBE_RestoreTabWithSpecialURL) {
  // Navigate new tab to a special URL.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUICreditsURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Close the tab.
  CloseTab(1);

  // Restore the closed tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, 1));
  content::WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(1);
  EnsureTabFinishedRestoring(tab);

  // See if content is as expected.
  EXPECT_GT(
      ui_test_utils::FindInPage(tab, base::ASCIIToUTF16("webkit"), true, false,
                                NULL, NULL),
      0);
}

// https://crbug.com/667932: Flakiness on linux_chromium_asan_rel_ng bot.
// https://crbug.com/835305: Timeout flakiness on Win7 Tests (dbg)(1) bot.
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER) || \
    (defined(OS_WIN) && !defined(NDEBUG))
#define MAYBE_RestoreTabWithSpecialURLOnBack DISABLED_RestoreTabWithSpecialURLOnBack
#else
#define MAYBE_RestoreTabWithSpecialURLOnBack RestoreTabWithSpecialURLOnBack
#endif

// Restore tab with special URL in its navigation history, go back to that
// entry and see that it loads properly. See http://crbug.com/31905
IN_PROC_BROWSER_TEST_F(TabRestoreTest, MAYBE_RestoreTabWithSpecialURLOnBack) {
  ASSERT_TRUE(embedded_test_server()->Start());

  const GURL http_url(embedded_test_server()->GetURL("/title1.html"));

  // Navigate new tab to a special URL.
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), GURL(chrome::kChromeUICreditsURL),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Then navigate to a normal URL.
  ui_test_utils::NavigateToURL(browser(), http_url);

  // Close the tab.
  CloseTab(1);

  // Restore the closed tab.
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, 1));
  content::WebContents* tab = browser()->tab_strip_model()->GetWebContentsAt(1);
  EnsureTabFinishedRestoring(tab);
  ASSERT_EQ(http_url, tab->GetURL());

  // Go back, and see if content is as expected.
  GoBack(browser());
  EXPECT_GT(
      ui_test_utils::FindInPage(tab, base::ASCIIToUTF16("webkit"), true, false,
                                NULL, NULL),
      0);
}

IN_PROC_BROWSER_TEST_F(TabRestoreTest, PRE_RestoreOnStartup) {
  // This results in a new tab at the end with url1.
  AddSomeTabs(browser(), 1);

  while (browser()->tab_strip_model()->count())
    CloseTab(0);
}

// Verifies restoring a tab works on startup.
IN_PROC_BROWSER_TEST_F(TabRestoreTest, RestoreOnStartup) {
  ASSERT_NO_FATAL_FAILURE(RestoreTab(0, 1));
  EXPECT_EQ(url1_,
            browser()->tab_strip_model()->GetWebContentsAt(1)->GetURL());
}

// Check that TabRestoreService and SessionService do not try to restore the
// same thing.
IN_PROC_BROWSER_TEST_F(TabRestoreTest,
                       RestoreFirstBrowserWhenSessionServiceEnabled) {
  // Do not exit from test when last browser is closed.
  ScopedKeepAlive keep_alive(KeepAliveOrigin::SESSION_RESTORE,
                             KeepAliveRestartOption::DISABLED);

  // Enable session service.
  SessionStartupPref pref(SessionStartupPref::LAST);
  Profile* profile = browser()->profile();
  SessionStartupPref::SetStartupPref(profile, pref);

  // Add tabs and close browser.
  AddSomeTabs(browser(), 3);
  // 1st tab is about:blank added by InProcessBrowserTest.
  EXPECT_EQ(4, browser()->tab_strip_model()->count());
  CloseBrowserSynchronously(browser());

  SessionRestoreTestHelper helper;
  // Restore browser (this is what Cmd-Shift-T does on Mac).
  chrome::OpenWindowWithRestoredTabs(profile);
  if (SessionRestore::IsRestoring(profile))
    helper.Wait();
  Browser* browser = GetBrowser(0);
  EXPECT_EQ(4, browser->tab_strip_model()->count());
}

// TabLoader (used here) is available only when browser is built
// with ENABLE_SESSION_SERVICE.
#if BUILDFLAG(ENABLE_SESSION_SERVICE)
IN_PROC_BROWSER_TEST_F(TabRestoreTest,
                       TabsFromRestoredWindowsAreLoadedGradually) {
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2_, WindowOpenDisposition::NEW_WINDOW,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_BROWSER);
  Browser* browser2 = GetBrowser(1);

  // Add tabs and close browser.
  const int tabs_count = 4;
  AddSomeTabs(browser2, tabs_count - browser2->tab_strip_model()->count());
  EXPECT_EQ(tabs_count, browser2->tab_strip_model()->count());
  CloseBrowserSynchronously(browser2);

  // Limit the number of restored tabs that are loaded.
  TabLoader::SetMaxLoadedTabCountForTest(2);

  // Restore recently closed window.
  content::WindowedNotificationObserver open_window_observer(
      chrome::NOTIFICATION_BROWSER_OPENED,
      content::NotificationService::AllSources());
  chrome::OpenWindowWithRestoredTabs(browser()->profile());
  open_window_observer.Wait();
  browser2 = GetBrowser(1);

  EXPECT_EQ(tabs_count, browser2->tab_strip_model()->count());
  EXPECT_EQ(tabs_count - 1, browser2->tab_strip_model()->active_index());
  // These two tabs should be loaded by TabLoader.
  EnsureTabFinishedRestoring(
      browser2->tab_strip_model()->GetWebContentsAt(tabs_count - 1));
  EnsureTabFinishedRestoring(browser2->tab_strip_model()->GetWebContentsAt(0));

  // The following isn't necessary but just to be sure there is no any async
  // task that could have an impact on the expectations below.
  content::RunAllPendingInMessageLoop();

  // These tabs shouldn't want to be loaded.
  for (int tab_idx = 1; tab_idx < tabs_count - 1; ++tab_idx) {
    auto* contents = browser2->tab_strip_model()->GetWebContentsAt(tab_idx);
    EXPECT_FALSE(contents->IsLoading());
    EXPECT_TRUE(contents->GetController().NeedsReload());
  }
}
#endif  // BUILDFLAG(ENABLE_SESSION_SERVICE)
