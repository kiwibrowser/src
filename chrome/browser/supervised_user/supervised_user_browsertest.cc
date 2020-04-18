// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/supervised_user/supervised_user_constants.h"
#include "chrome/browser/supervised_user/supervised_user_interstitial.h"
#include "chrome/browser/supervised_user/supervised_user_navigation_observer.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "chrome/browser/supervised_user/supervised_user_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service.h"
#include "chrome/browser/supervised_user/supervised_user_settings_service_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/interstitial_page.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/public/cpp/network_switches.h"
#include "testing/gmock/include/gmock/gmock.h"

using content::InterstitialPage;
using content::NavigationController;
using content::NavigationEntry;
using content::WebContents;

namespace {

bool AreCommittedInterstitialsEnabled() {
  return base::FeatureList::IsEnabled(
      features::kSupervisedUserCommittedInterstitials);
}

class InterstitialPageObserver : public content::WebContentsObserver {
 public:
  InterstitialPageObserver(WebContents* web_contents,
                           const base::Closure& callback)
      : content::WebContentsObserver(web_contents), callback_(callback) {}
  ~InterstitialPageObserver() override {}

  void DidAttachInterstitialPage() override {
    DCHECK(!AreCommittedInterstitialsEnabled());
    callback_.Run();
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    // With committed interstitials, DidAttachInterstitialPage is not called, so
    // call the callback from here if there was an error.
    if (AreCommittedInterstitialsEnabled() &&
        navigation_handle->IsErrorPage()) {
      callback_.Run();
    }
  }

 private:
  base::Closure callback_;
};

// TODO(carlosil): These tests can be turned into regular (non-parameterized)
// tests once committed interstitials are the only code path, special cases for
// committed/non-committed interstitials should also be cleaned up.

// Tests filtering for supervised users.
class SupervisedUserTest : public InProcessBrowserTest,
                           public testing::WithParamInterface<bool> {
 public:
  // Indicates whether the interstitial should proceed or not.
  enum InterstitialAction {
    INTERSTITIAL_PROCEED,
    INTERSTITIAL_DONTPROCEED,
  };

  SupervisedUserTest() : supervised_user_service_(nullptr) {}
  ~SupervisedUserTest() override {}

  bool ShownPageIsInterstitial(Browser* browser) {
    WebContents* tab = browser->tab_strip_model()->GetActiveWebContents();
    EXPECT_FALSE(tab->IsCrashed());
    if (AreCommittedInterstitialsEnabled()) {
      base::string16 title;
      ui_test_utils::GetCurrentTabTitle(browser, &title);
      return tab->GetController().GetActiveEntry()->GetPageType() ==
                 content::PAGE_TYPE_ERROR &&
             title == base::ASCIIToUTF16("Site blocked");
    }
    return tab->ShowingInterstitialPage();
  }

  void SendAccessRequest(WebContents* tab) {
    if (AreCommittedInterstitialsEnabled()) {
      tab->GetMainFrame()->ExecuteJavaScriptForTests(base::ASCIIToUTF16(
          "supervisedUserErrorPageController.requestPermission()"));
      return;
    }

    InterstitialPage* interstitial_page = tab->GetInterstitialPage();
    ASSERT_TRUE(interstitial_page);

    // Get the SupervisedUserInterstitial delegate.
    content::InterstitialPageDelegate* delegate =
        interstitial_page->GetDelegateForTesting();

    // Simulate the click on the "request" button.
    delegate->CommandReceived("\"request\"");
  }

  void GoBack(WebContents* tab) {
    if (AreCommittedInterstitialsEnabled()) {
      tab->GetMainFrame()->ExecuteJavaScriptForTests(
          base::ASCIIToUTF16("supervisedUserErrorPageController.goBack()"));
      return;
    }
    InterstitialPage* interstitial_page = tab->GetInterstitialPage();
    ASSERT_TRUE(interstitial_page);

    // Get the SupervisedUserInterstitial delegate.
    content::InterstitialPageDelegate* delegate =
        interstitial_page->GetDelegateForTesting();

    // Simulate the click on the "back" button
    delegate->CommandReceived("\"back\"");
  }

  void GoBackAndWaitForNavigation(WebContents* tab) {
    DCHECK(AreCommittedInterstitialsEnabled());
    content::TestNavigationObserver observer(tab);
    GoBack(tab);
    observer.Wait();
  }

 protected:
  void SetUpOnMainThread() override {
    if (GetParam()) {
      feature_list.InitAndEnableFeature(
          features::kSupervisedUserCommittedInterstitials);
    } else {
      feature_list.InitAndDisableFeature(
          features::kSupervisedUserCommittedInterstitials);
    }

    // Set up the SupervisedUserNavigationObserver manually since the profile
    // was not supervised when the browser was created.
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    SupervisedUserNavigationObserver::CreateForWebContents(web_contents);

    supervised_user_service_ =
        SupervisedUserServiceFactory::GetForProfile(browser()->profile());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Enable the test server and remap all URLs to it.
    ASSERT_TRUE(embedded_test_server()->Start());
    std::string host_port = embedded_test_server()->host_port_pair().ToString();
    command_line->AppendSwitchASCII(network::switches::kHostResolverRules,
                                    "MAP *.example.com " + host_port + "," +
                                        "MAP *.new-example.com " + host_port +
                                        "," + "MAP *.a.com " + host_port);

    command_line->AppendSwitchASCII(switches::kSupervisedUserId, "asdf");
  }

  // Acts like a synchronous call to history's QueryHistory. Modified from
  // history_querying_unittest.cc.
  void QueryHistory(history::HistoryService* history_service,
                    const std::string& text_query,
                    const history::QueryOptions& options,
                    history::QueryResults* results) {
    base::RunLoop run_loop;
    base::CancelableTaskTracker history_task_tracker;
    auto callback = [](history::QueryResults* new_results,
                       base::RunLoop* run_loop,
                       history::QueryResults* results) {
      results->Swap(new_results);
      run_loop->Quit();
    };
    history_service->QueryHistory(base::UTF8ToUTF16(text_query), options,
                                  base::Bind(callback, results, &run_loop),
                                  &history_task_tracker);
    run_loop.Run();  // Will go until ...Complete calls Quit.
  }

  SupervisedUserService* supervised_user_service_;

 private:
  base::test::ScopedFeatureList feature_list;
};

// Tests the filter mode in which all sites are blocked by default.
class SupervisedUserBlockModeTest : public SupervisedUserTest {
 public:
  void SetUpOnMainThread() override {
    SupervisedUserTest::SetUpOnMainThread();

    Profile* profile = browser()->profile();
    SupervisedUserSettingsService* supervised_user_settings_service =
        SupervisedUserSettingsServiceFactory::GetForProfile(profile);
    supervised_user_settings_service->SetLocalSetting(
        supervised_users::kContentPackDefaultFilteringBehavior,
        std::make_unique<base::Value>(SupervisedUserURLFilter::BLOCK));
  }
};

class MockTabStripModelObserver : public TabStripModelObserver {
 public:
  explicit MockTabStripModelObserver(TabStripModel* tab_strip)
      : tab_strip_(tab_strip) {
    tab_strip_->AddObserver(this);
  }

  ~MockTabStripModelObserver() override { tab_strip_->RemoveObserver(this); }

  MOCK_METHOD3(TabClosingAt, void(TabStripModel*, content::WebContents*, int));

 private:
  TabStripModel* tab_strip_;
};

INSTANTIATE_TEST_CASE_P(, SupervisedUserTest, ::testing::Values(false, true));

INSTANTIATE_TEST_CASE_P(,
                        SupervisedUserBlockModeTest,
                        ::testing::Values(false, true));

// Navigates to a blocked URL.
IN_PROC_BROWSER_TEST_P(SupervisedUserBlockModeTest,
                       SendAccessRequestOnBlockedURL) {
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  WebContents* tab = browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  SendAccessRequest(tab);

  // TODO(sergiu): Properly check that the access request was sent here.

  if (AreCommittedInterstitialsEnabled())
    GoBackAndWaitForNavigation(tab);
  else
    GoBack(tab);

  // Make sure that the tab is still there.
  EXPECT_EQ(tab, browser()->tab_strip_model()->GetActiveWebContents());

  EXPECT_FALSE(ShownPageIsInterstitial(browser()));
}

// Navigates to a blocked URL in a new tab. We expect the tab to be closed
// automatically on pressing the "back" button on the interstitial.
IN_PROC_BROWSER_TEST_P(SupervisedUserBlockModeTest, OpenBlockedURLInNewTab) {
  TabStripModel* tab_strip = browser()->tab_strip_model();
  WebContents* prev_tab = tab_strip->GetActiveWebContents();

  // Open blocked URL in a new tab.
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), test_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Check that we got the interstitial.
  WebContents* tab = tab_strip->GetActiveWebContents();
  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  // On pressing the "back" button, the new tab should be closed, and we should
  // get back to the previous active tab.
  MockTabStripModelObserver observer(tab_strip);
  base::RunLoop run_loop;
  EXPECT_CALL(observer,
              TabClosingAt(tab_strip, tab, tab_strip->active_index()))
      .WillOnce(testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
  GoBack(tab);
  run_loop.Run();
  EXPECT_EQ(prev_tab, tab_strip->GetActiveWebContents());
}

// Navigates to a page in a new tab, then blocks it (which makes the
// interstitial page behave differently from the preceding test, where the
// navigation is blocked before it commits). The expected behavior is the same
// though: the tab should be closed when going back.
IN_PROC_BROWSER_TEST_P(SupervisedUserTest, BlockNewTabAfterLoading) {
  TabStripModel* tab_strip = browser()->tab_strip_model();
  WebContents* prev_tab = tab_strip->GetActiveWebContents();

  // Open URL in a new tab.
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), test_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Check that there is no interstitial.
  WebContents* tab = tab_strip->GetActiveWebContents();
  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  {
    // Block the current URL.
    // TODO(carlosil): Remove this run_loop once Committed interstitials are the
    // only code path.
    base::RunLoop run_loop;
    InterstitialPageObserver interstitial_observer(tab, run_loop.QuitClosure());

    SupervisedUserSettingsService* supervised_user_settings_service =
        SupervisedUserSettingsServiceFactory::GetForProfile(
            browser()->profile());
    supervised_user_settings_service->SetLocalSetting(
        supervised_users::kContentPackDefaultFilteringBehavior,
        std::make_unique<base::Value>(SupervisedUserURLFilter::BLOCK));

    const SupervisedUserURLFilter* filter =
        supervised_user_service_->GetURLFilter();
    ASSERT_EQ(SupervisedUserURLFilter::BLOCK,
              filter->GetFilteringBehaviorForURL(test_url));

    if (AreCommittedInterstitialsEnabled()) {
      content::TestNavigationObserver observer(tab);
      observer.Wait();
    } else {
      content::RunThisRunLoop(&run_loop);
    }

    // Check that we got the interstitial.
    ASSERT_TRUE(ShownPageIsInterstitial(browser()));
  }

  {
    // On pressing the "back" button, the new tab should be closed, and we
    // should get back to the previous active tab.
    MockTabStripModelObserver observer(tab_strip);
    base::RunLoop run_loop;
    EXPECT_CALL(observer,
                TabClosingAt(tab_strip, tab, tab_strip->active_index()))
        .WillOnce(testing::InvokeWithoutArgs(&run_loop, &base::RunLoop::Quit));
    GoBack(tab);
    run_loop.Run();
    EXPECT_EQ(prev_tab, tab_strip->GetActiveWebContents());
  }
}

// Tests that we don't end up canceling an interstitial (thereby closing the
// whole tab) by attempting to show a second one above it.
IN_PROC_BROWSER_TEST_P(SupervisedUserTest, DontShowInterstitialTwice) {
  TabStripModel* tab_strip = browser()->tab_strip_model();

  // Open URL in a new tab.
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), test_url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  // Check that there is no interstitial.
  WebContents* tab = tab_strip->GetActiveWebContents();
  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  // Block the current URL.
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(browser()->profile());
  base::RunLoop run_loop;
  InterstitialPageObserver interstitial_observer(tab, run_loop.QuitClosure());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackDefaultFilteringBehavior,
      std::make_unique<base::Value>(SupervisedUserURLFilter::BLOCK));

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();
  ASSERT_EQ(SupervisedUserURLFilter::BLOCK,
            filter->GetFilteringBehaviorForURL(test_url));

  if (AreCommittedInterstitialsEnabled()) {
    content::TestNavigationObserver observer(tab);
    observer.Wait();
  } else {
    content::RunThisRunLoop(&run_loop);
  }

  // Check that we got the interstitial.
  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  // Trigger a no-op change to the site lists, which will notify observers of
  // the URL filter.
  Profile* profile = browser()->profile();
  SupervisedUserService* supervised_user_service =
      SupervisedUserServiceFactory::GetForProfile(profile);
  supervised_user_service->OnSiteListUpdated();

  content::RunAllPendingInMessageLoop();
  EXPECT_EQ(tab, tab_strip->GetActiveWebContents());
}

// Tests that it's possible to navigate from a blocked page to another blocked
// page.
IN_PROC_BROWSER_TEST_P(SupervisedUserBlockModeTest,
                       NavigateFromBlockedPageToBlockedPage) {
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  WebContents* tab = browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  GURL test_url2("http://www.a.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url2);

  ASSERT_TRUE(ShownPageIsInterstitial(browser()));
  EXPECT_EQ(test_url2, tab->GetVisibleURL());
}

// Tests whether a visit attempt adds a special history entry.
IN_PROC_BROWSER_TEST_P(SupervisedUserBlockModeTest, HistoryVisitRecorded) {
  GURL allowed_url("http://www.example.com/simple.html");

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();

  // Set the host as allowed.
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetKey(allowed_url.host(), base::Value(true));
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(
          browser()->profile());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter->GetFilteringBehaviorForURL(allowed_url));
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter->GetFilteringBehaviorForURL(allowed_url.GetWithEmptyPath()));

  ui_test_utils::NavigateToURL(browser(), allowed_url);

  // Navigate to it and check that we don't get an interstitial.
  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  // Navigate to a blocked page and go back on the interstitial.
  GURL blocked_url("http://www.new-example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), blocked_url);

  ASSERT_TRUE(ShownPageIsInterstitial(browser()));
  WebContents* tab = browser()->tab_strip_model()->GetActiveWebContents();

  if (AreCommittedInterstitialsEnabled())
    GoBackAndWaitForNavigation(tab);
  else
    GoBack(tab);

  EXPECT_EQ(allowed_url.spec(), tab->GetURL().spec());
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter->GetFilteringBehaviorForURL(allowed_url.GetWithEmptyPath()));
  EXPECT_EQ(SupervisedUserURLFilter::BLOCK,
            filter->GetFilteringBehaviorForURL(blocked_url.GetWithEmptyPath()));

  // Query the history entry.
  history::HistoryService* history_service =
      HistoryServiceFactory::GetForProfile(browser()->profile(),
                                           ServiceAccessType::EXPLICIT_ACCESS);
  history::QueryOptions options;
  history::QueryResults results;
  QueryHistory(history_service, "", options, &results);

  // With committed interstitials enabled, going back to the site is an actual
  // back navigation (instead of just closing the interstitial), so the most
  // recent history entry will be the allowed site, with non-committed
  // interstitials, the most recent one will be the blocked one.
  int allowed = AreCommittedInterstitialsEnabled() ? 0 : 1;
  int blocked = AreCommittedInterstitialsEnabled() ? 1 : 0;

  // Check that the entries have the correct blocked_visit value.
  ASSERT_EQ(2u, results.size());
  EXPECT_EQ(blocked_url.spec(), results[blocked].url().spec());
  EXPECT_TRUE(results[blocked].blocked_visit());
  EXPECT_EQ(allowed_url.spec(), results[allowed].url().spec());
  EXPECT_FALSE(results[allowed].blocked_visit());
}

IN_PROC_BROWSER_TEST_P(SupervisedUserTest, GoBackOnDontProceed) {
  // We start out at the initial navigation.
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(0, web_contents->GetController().GetCurrentEntryIndex());

  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  // Set the host as blocked and wait for the interstitial to appear.
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetKey(test_url.host(), base::Value(false));
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(
          browser()->profile());
  auto message_loop_runner = base::MakeRefCounted<content::MessageLoopRunner>();
  InterstitialPageObserver interstitial_observer(
      web_contents, message_loop_runner->QuitClosure());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();
  ASSERT_EQ(SupervisedUserURLFilter::BLOCK,
            filter->GetFilteringBehaviorForURL(test_url));

  if (AreCommittedInterstitialsEnabled()) {
    content::TestNavigationObserver observer(web_contents);
    observer.Wait();
  } else {
    message_loop_runner->Run();
  }

  content::WindowedNotificationObserver observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());
  GoBack(web_contents);
  observer.Wait();

  // We should have gone back to the initial navigation.
  EXPECT_EQ(0, web_contents->GetController().GetCurrentEntryIndex());
}

IN_PROC_BROWSER_TEST_P(SupervisedUserTest, ClosingBlockedTabDoesNotCrash) {
  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  ASSERT_EQ(0, web_contents->GetController().GetCurrentEntryIndex());

  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  // Set the host as blocked and wait for the interstitial to appear.
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetKey(test_url.host(), base::Value(false));
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(browser()->profile());
  auto message_loop_runner = base::MakeRefCounted<content::MessageLoopRunner>();
  InterstitialPageObserver interstitial_observer(
      web_contents, message_loop_runner->QuitClosure());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();
  ASSERT_EQ(SupervisedUserURLFilter::BLOCK,
            filter->GetFilteringBehaviorForURL(test_url));

  message_loop_runner->Run();

  if (!AreCommittedInterstitialsEnabled()) {
    InterstitialPage* interstitial_page = web_contents->GetInterstitialPage();
    ASSERT_TRUE(interstitial_page);
  }

  // Verify that there is no crash when closing the blocked tab
  // (https://crbug.com/719708).
  browser()->tab_strip_model()->CloseWebContentsAt(
      0, TabStripModel::CLOSE_USER_GESTURE);
  content::RunAllPendingInMessageLoop();
}

IN_PROC_BROWSER_TEST_P(SupervisedUserTest, BlockThenUnblock) {
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_FALSE(ShownPageIsInterstitial(browser()));

  // Set the host as blocked and wait for the interstitial to appear.
  auto dict = std::make_unique<base::DictionaryValue>();
  dict->SetKey(test_url.host(), base::Value(false));
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(
          browser()->profile());
  // TODO(carlosil): Remove this run_loop once Committed interstitials are the
  // only code path.
  base::RunLoop run_loop;
  InterstitialPageObserver observer(web_contents, run_loop.QuitClosure());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();
  ASSERT_EQ(SupervisedUserURLFilter::BLOCK,
            filter->GetFilteringBehaviorForURL(test_url));

  if (AreCommittedInterstitialsEnabled()) {
    content::TestNavigationObserver observer(web_contents);
    observer.Wait();
  } else {
    content::RunThisRunLoop(&run_loop);
  }
  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  dict = std::make_unique<base::DictionaryValue>();
  dict->SetKey(test_url.host(), base::Value(true));
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));
  ASSERT_EQ(SupervisedUserURLFilter::ALLOW,
            filter->GetFilteringBehaviorForURL(test_url));

  if (AreCommittedInterstitialsEnabled()) {
    content::TestNavigationObserver observer(web_contents);
    observer.Wait();
  }

  ASSERT_EQ(test_url, web_contents->GetURL());

  EXPECT_FALSE(ShownPageIsInterstitial(browser()));
}

IN_PROC_BROWSER_TEST_P(SupervisedUserBlockModeTest, Unblock) {
  GURL test_url("http://www.example.com/simple.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_TRUE(ShownPageIsInterstitial(browser()));

  content::WindowedNotificationObserver observer(
      content::NOTIFICATION_LOAD_STOP,
      content::NotificationService::AllSources());

  // Set the host as allowed.
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetKey(test_url.host(), base::Value(true));
  SupervisedUserSettingsService* supervised_user_settings_service =
      SupervisedUserSettingsServiceFactory::GetForProfile(
          browser()->profile());
  supervised_user_settings_service->SetLocalSetting(
      supervised_users::kContentPackManualBehaviorHosts, std::move(dict));

  const SupervisedUserURLFilter* filter =
      supervised_user_service_->GetURLFilter();
  EXPECT_EQ(SupervisedUserURLFilter::ALLOW,
            filter->GetFilteringBehaviorForURL(test_url.GetWithEmptyPath()));

  observer.Wait();
  EXPECT_EQ(test_url, web_contents->GetURL());
}

}  // namespace
