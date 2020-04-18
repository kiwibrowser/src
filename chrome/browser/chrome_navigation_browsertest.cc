// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_timeouts.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/renderer_context_menu/render_view_context_menu_test_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/url_formatter/url_formatter.h"
#include "components/variations/active_field_trials.h"
#include "components/variations/hashing.h"
#include "components/variations/variations_switches.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/navigation_handle_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "google_apis/gaia/gaia_switches.h"
#include "net/dns/mock_host_resolver.h"

class ChromeNavigationBrowserTest : public InProcessBrowserTest {
 public:
  ChromeNavigationBrowserTest() {}
  ~ChromeNavigationBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Backgrounded renderer processes run at a lower priority, causing the
    // tests to take more time to complete. Disable backgrounding so that the
    // tests don't time out.
    command_line->AppendSwitch(switches::kDisableRendererBackgrounding);

    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void StartServerWithExpiredCert() {
    expired_https_server_.reset(
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS));
    expired_https_server_->SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
    expired_https_server_->AddDefaultHandlers(
        base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
    ASSERT_TRUE(expired_https_server_->Start());
  }

  net::EmbeddedTestServer* expired_https_server() {
    return expired_https_server_.get();
  }

 private:
  std::unique_ptr<net::EmbeddedTestServer> expired_https_server_;

  DISALLOW_COPY_AND_ASSIGN(ChromeNavigationBrowserTest);
};

// Helper class to track and allow waiting for navigation start events.
class DidStartNavigationObserver : public content::WebContentsObserver {
 public:
  explicit DidStartNavigationObserver(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        message_loop_runner_(new content::MessageLoopRunner) {}
  ~DidStartNavigationObserver() override {}

  // Runs a nested run loop and blocks until the full load has
  // completed.
  void Wait() { message_loop_runner_->Run(); }

 private:
  // WebContentsObserver
  void DidStartNavigation(content::NavigationHandle* handle) override {
    if (message_loop_runner_->loop_running())
      message_loop_runner_->Quit();
  }

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<content::MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(DidStartNavigationObserver);
};

// Test to verify that navigations are not deleting the transient
// NavigationEntry when showing an interstitial page and the old renderer
// process is trying to navigate. See https://crbug.com/600046.
IN_PROC_BROWSER_TEST_F(
    ChromeNavigationBrowserTest,
    TransientEntryPreservedOnMultipleNavigationsDuringInterstitial) {
  StartServerWithExpiredCert();

  GURL setup_url =
      embedded_test_server()->GetURL("/window_open_and_navigate.html");
  GURL initial_url = embedded_test_server()->GetURL("/title1.html");
  GURL error_url(expired_https_server()->GetURL("/ssl/blank_page.html"));

  ui_test_utils::NavigateToURL(browser(), setup_url);
  content::WebContents* main_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Call the JavaScript method in the test page, which opens a new window
  // and stores a handle to it.
  content::WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());
  EXPECT_TRUE(content::ExecuteScript(main_web_contents, "openWin();"));
  tab_added_observer.Wait();
  content::WebContents* new_web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Navigate the opened window to a page that will successfully commit and
  // create a NavigationEntry.
  {
    content::TestNavigationObserver observer(new_web_contents);
    EXPECT_TRUE(content::ExecuteScript(
        main_web_contents, "navigate('" + initial_url.spec() + "');"));
    observer.Wait();
    EXPECT_EQ(initial_url, new_web_contents->GetLastCommittedURL());
  }

  // Navigate the opened window to a page which will trigger an
  // interstitial.
  {
    content::TestNavigationObserver observer(new_web_contents);
    EXPECT_TRUE(content::ExecuteScript(
        main_web_contents, "navigate('" + error_url.spec() + "');"));
    observer.Wait();
    EXPECT_EQ(initial_url, new_web_contents->GetLastCommittedURL());
    EXPECT_EQ(error_url, new_web_contents->GetVisibleURL());
  }

  // Navigate again the opened window to the same page. It should not cause
  // WebContents::GetVisibleURL to return the last committed one.
  {
    DidStartNavigationObserver nav_observer(new_web_contents);
    EXPECT_TRUE(content::ExecuteScript(
        main_web_contents, "navigate('" + error_url.spec() + "');"));
    nav_observer.Wait();
    EXPECT_EQ(error_url, new_web_contents->GetVisibleURL());
    EXPECT_TRUE(new_web_contents->GetController().GetTransientEntry());
    EXPECT_FALSE(new_web_contents->IsLoading());
  }
}

// Tests that viewing frame source on a local file:// page with an iframe
// with a remote URL shows the correct tab title.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest, TestViewFrameSource) {
  // The local page file:// URL.
  GURL local_page_with_iframe_url = ui_test_utils::GetTestUrl(
      base::FilePath(base::FilePath::kCurrentDirectory),
      base::FilePath(FILE_PATH_LITERAL("iframe.html")));

  // The non-file:// URL of the page to load in the iframe.
  GURL iframe_target_url = embedded_test_server()->GetURL("/title1.html");
  ui_test_utils::NavigateToURL(browser(), local_page_with_iframe_url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  content::TestNavigationObserver observer(web_contents);
  ASSERT_TRUE(content::ExecuteScript(
      web_contents->GetMainFrame(),
      base::StringPrintf("var iframe = document.getElementById('test');\n"
                         "iframe.setAttribute('src', '%s');\n",
                         iframe_target_url.spec().c_str())));
  observer.Wait();

  content::RenderFrameHost* frame =
      content::ChildFrameAt(web_contents->GetMainFrame(), 0);
  ASSERT_TRUE(frame);
  ASSERT_NE(frame, web_contents->GetMainFrame());

  content::ContextMenuParams params;
  params.page_url = local_page_with_iframe_url;
  params.frame_url = frame->GetLastCommittedURL();
  TestRenderViewContextMenu menu(frame, params);
  menu.Init();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_VIEWFRAMESOURCE, 0);
  ASSERT_EQ(browser()->tab_strip_model()->count(), 2);
  content::WebContents* new_web_contents =
      browser()->tab_strip_model()->GetWebContentsAt(1);
  ASSERT_NE(new_web_contents, web_contents);
  WaitForLoadStop(new_web_contents);

  GURL view_frame_source_url(content::kViewSourceScheme + std::string(":") +
                             iframe_target_url.spec());
  EXPECT_EQ(url_formatter::FormatUrl(view_frame_source_url),
            new_web_contents->GetTitle());
}

// Base class for ctrl+click tests, which contains all the common functionality
// independent from which process the navigation happens in. Each subclass
// defines its own expectations depending on the conditions of the test.
class CtrlClickProcessTest : public ChromeNavigationBrowserTest {
 protected:
  virtual void VerifyProcessExpectations(
      content::WebContents* main_contents,
      content::WebContents* new_contents) = 0;

  // Simulates ctrl-clicking an anchor with the given id in |main_contents|.
  // Verifies that the new contents are in the correct process and separate
  // BrowsingInstance from |main_contents|.  Returns contents of the newly
  // opened tab.
  content::WebContents* SimulateCtrlClick(content::WebContents* main_contents,
                                          const char* id_of_anchor_to_click) {
    // Ctrl-click the anchor/link in the page.
    content::WebContents* new_contents = nullptr;
    {
      content::WebContentsAddedObserver new_tab_observer;
#if defined(OS_MACOSX)
      const char* new_tab_click_script_template =
          "simulateClick(\"%s\", { metaKey: true });";
#else
      const char* new_tab_click_script_template =
          "simulateClick(\"%s\", { ctrlKey: true });";
#endif
      std::string new_tab_click_script = base::StringPrintf(
          new_tab_click_script_template, id_of_anchor_to_click);
      EXPECT_TRUE(ExecuteScript(main_contents, new_tab_click_script));

      // Wait for a new tab to appear (the whole point of this test).
      new_contents = new_tab_observer.GetWebContents();
    }

    // Verify that the new tab has the right contents and is in the tab strip.
    EXPECT_TRUE(WaitForLoadStop(new_contents));
    EXPECT_LT(1, browser()->tab_strip_model()->count());  // More than 1 tab?
    CHECK_NE(TabStripModel::kNoTab,
             browser()->tab_strip_model()->GetIndexOfWebContents(new_contents));
    GURL expected_url(embedded_test_server()->GetURL("/title1.html"));
    EXPECT_EQ(expected_url, new_contents->GetLastCommittedURL());

    VerifyProcessExpectations(main_contents, new_contents);

    {
      // Double-check that main_contents has expected window.name set.
      // This is a sanity check of test setup; this is not a product test.
      std::string name_of_main_contents_window;
      EXPECT_TRUE(ExecuteScriptAndExtractString(
          main_contents, "window.domAutomationController.send(window.name)",
          &name_of_main_contents_window));
      EXPECT_EQ("main_contents", name_of_main_contents_window);

      // Verify that the new contents doesn't have a window.opener set.
      bool window_opener_cast_to_bool = true;
      EXPECT_TRUE(ExecuteScriptAndExtractBool(
          new_contents, "window.domAutomationController.send(!!window.opener)",
          &window_opener_cast_to_bool));
      EXPECT_FALSE(window_opener_cast_to_bool);

      VerifyBrowsingInstanceExpectations(main_contents, new_contents);
    }

    return new_contents;
  }

  void TestCtrlClick(const char* id_of_anchor_to_click) {
    // Navigate to the test page.
    GURL main_url(embedded_test_server()->GetURL(
        "/frame_tree/anchor_to_same_site_location.html"));
    ui_test_utils::NavigateToURL(browser(), main_url);

    // Verify that there is only 1 active tab (with the right contents
    // committed).
    EXPECT_EQ(0, browser()->tab_strip_model()->active_index());
    content::WebContents* main_contents =
        browser()->tab_strip_model()->GetWebContentsAt(0);
    EXPECT_EQ(main_url, main_contents->GetLastCommittedURL());

    // Test what happens after ctrl-click.  SimulateCtrlClick will verify
    // that |new_contents1| is in the correct process and separate
    // BrowsingInstance from |main_contents|.
    content::WebContents* new_contents1 =
        SimulateCtrlClick(main_contents, id_of_anchor_to_click);

    // Test that each subsequent ctrl-click also gets the correct process.
    content::WebContents* new_contents2 =
        SimulateCtrlClick(main_contents, id_of_anchor_to_click);
    EXPECT_FALSE(new_contents1->GetSiteInstance()->IsRelatedSiteInstance(
        new_contents2->GetSiteInstance()));
    VerifyProcessExpectations(new_contents1, new_contents2);
  }

 private:
  void VerifyBrowsingInstanceExpectations(content::WebContents* main_contents,
                                          content::WebContents* new_contents) {
    // Verify that the new contents cannot find the old contents via
    // window.open. (i.e. window.open should open a new window, rather than
    // returning a reference to main_contents / old window).
    std::string location_of_opened_window;
    EXPECT_TRUE(ExecuteScriptAndExtractString(
        new_contents,
        "w = window.open('', 'main_contents');"
        "window.domAutomationController.send(w.location.href);",
        &location_of_opened_window));
    EXPECT_EQ(url::kAboutBlankURL, location_of_opened_window);
  }
};

// Tests that verify that ctrl-click results 1) open up in a new renderer
// process (https://crbug.com/23815) and 2) are in a new BrowsingInstance (e.g.
// cannot find the opener's window by name - https://crbug.com/658386).
class CtrlClickShouldEndUpInNewProcessTest : public CtrlClickProcessTest {
 protected:
  void VerifyProcessExpectations(content::WebContents* main_contents,
                                 content::WebContents* new_contents) override {
    // Verify that the two WebContents are in a different process, SiteInstance
    // and BrowsingInstance from the old contents.
    EXPECT_NE(main_contents->GetMainFrame()->GetProcess(),
              new_contents->GetMainFrame()->GetProcess());
    EXPECT_NE(main_contents->GetMainFrame()->GetSiteInstance(),
              new_contents->GetMainFrame()->GetSiteInstance());
    EXPECT_FALSE(main_contents->GetSiteInstance()->IsRelatedSiteInstance(
        new_contents->GetSiteInstance()));
  }
};

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInNewProcessTest, NoTarget) {
  TestCtrlClick("test-anchor-no-target");
}

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInNewProcessTest, BlankTarget) {
  TestCtrlClick("test-anchor-with-blank-target");
}

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInNewProcessTest, SubframeTarget) {
  TestCtrlClick("test-anchor-with-subframe-target");
}

// Similar to the tests above, but verifies that the new WebContents ends up in
// the same process as the opener when it is exceeding the process limit.
// See https://crbug.com/774723.
class CtrlClickShouldEndUpInSameProcessTest : public CtrlClickProcessTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    CtrlClickProcessTest::SetUpCommandLine(command_line);
    content::IsolateAllSitesForTesting(command_line);
    content::RenderProcessHost::SetMaxRendererProcessCount(1);
  }

  void SetUpOnMainThread() override {
    CtrlClickProcessTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  void VerifyProcessExpectations(content::WebContents* contents1,
                                 content::WebContents* contents2) override {
    // Verify that the two WebContents are in the same process, though different
    // SiteInstance and BrowsingInstance from the old contents.
    EXPECT_EQ(contents1->GetMainFrame()->GetProcess(),
              contents2->GetMainFrame()->GetProcess());
    EXPECT_EQ(contents1->GetMainFrame()->GetSiteInstance()->GetSiteURL(),
              contents2->GetMainFrame()->GetSiteInstance()->GetSiteURL());
    EXPECT_FALSE(contents1->GetSiteInstance()->IsRelatedSiteInstance(
        contents2->GetSiteInstance()));
  }
};

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInSameProcessTest, NoTarget) {
  TestCtrlClick("test-anchor-no-target");
}

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInSameProcessTest, BlankTarget) {
  TestCtrlClick("test-anchor-with-blank-target");
}

IN_PROC_BROWSER_TEST_F(CtrlClickShouldEndUpInSameProcessTest, SubframeTarget) {
  TestCtrlClick("test-anchor-with-subframe-target");
}

class ChromeNavigationPortMappedBrowserTest : public InProcessBrowserTest {
 public:
  ChromeNavigationPortMappedBrowserTest() {}
  ~ChromeNavigationPortMappedBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ASSERT_TRUE(embedded_test_server()->Start());

    // Use the command line parameter for the host resolver, so URLs without
    // explicit port numbers can be mapped under the hood to the port number
    // the |embedded_test_server| uses. It is required to test with potentially
    // malformed URLs.
    std::string port =
        base::IntToString(embedded_test_server()->host_port_pair().port());
    command_line->AppendSwitchASCII(
        "host-resolver-rules",
        "MAP * 127.0.0.1:" + port + ", EXCLUDE 127.0.0.1*");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeNavigationPortMappedBrowserTest);
};

// Test to verify that a malformed URL set as the virtual URL of a
// NavigationEntry will result in the navigation being dropped.
// See https://crbug.com/657720.
IN_PROC_BROWSER_TEST_F(ChromeNavigationPortMappedBrowserTest,
                       ContextMenuNavigationToInvalidUrl) {
  GURL initial_url = embedded_test_server()->GetURL("/title1.html");
  GURL new_tab_url(
      "www.foo.com::/server-redirect?http%3A%2F%2Fbar.com%2Ftitle2.html");

  // Navigate to an initial page, to ensure we have a committed document
  // from which to perform a context menu initiated navigation.
  ui_test_utils::NavigateToURL(browser(), initial_url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // This corresponds to "Open link in new tab".
  content::ContextMenuParams params;
  params.is_editable = false;
  params.media_type = blink::WebContextMenuData::kMediaTypeNone;
  params.page_url = initial_url;
  params.link_url = new_tab_url;

  content::WindowedNotificationObserver tab_added_observer(
      chrome::NOTIFICATION_TAB_ADDED,
      content::NotificationService::AllSources());

  TestRenderViewContextMenu menu(web_contents->GetMainFrame(), params);
  menu.Init();
  menu.ExecuteCommand(IDC_CONTENT_CONTEXT_OPENLINKNEWTAB, 0);

  // Wait for the new tab to be created and for loading to stop. The
  // navigation should not be allowed, therefore there should not be a last
  // committed URL in the new tab.
  tab_added_observer.Wait();
  content::WebContents* new_web_contents =
      browser()->tab_strip_model()->GetWebContentsAt(
          browser()->tab_strip_model()->count() - 1);
  WaitForLoadStop(new_web_contents);

  // If the test is unsuccessful, the return value from GetLastCommittedURL
  // will be the virtual URL for the created NavigationEntry.
  // Note: Before the bug was fixed, the URL was the new_tab_url with a scheme
  // prepended and one less ":" character after the host.
  EXPECT_EQ(GURL(), new_web_contents->GetLastCommittedURL());
}

// A test performing two simultaneous navigations, to ensure code in chrome/,
// such as tab helpers, can handle those cases.
// This test starts a browser-initiated cross-process navigation, which is
// delayed. At the same time, the renderer does a synchronous navigation
// through pushState, which will create a separate navigation and associated
// NavigationHandle. Afterwards, the original cross-process navigation is
// resumed and confirmed to properly commit.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest,
                       SlowCrossProcessNavigationWithPushState) {
  const GURL kURL1 = embedded_test_server()->GetURL("/title1.html");
  const GURL kPushStateURL =
      embedded_test_server()->GetURL("/title1.html#fragment");
  const GURL kURL2 = embedded_test_server()->GetURL("/title2.html");

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Navigate to the initial page.
  ui_test_utils::NavigateToURL(browser(), kURL1);

  // Start navigating to the second page.
  content::TestNavigationManager manager(web_contents, kURL2);
  content::NavigationHandleCommitObserver navigation_observer(web_contents,
                                                              kURL2);
  web_contents->GetController().LoadURL(
      kURL2, content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string());
  EXPECT_TRUE(manager.WaitForRequestStart());

  // The current page does a PushState.
  content::NavigationHandleCommitObserver push_state_observer(web_contents,
                                                              kPushStateURL);
  std::string push_state =
      "history.pushState({}, \"title 1\", \"" + kPushStateURL.spec() + "\");";
  EXPECT_TRUE(ExecuteScript(web_contents, push_state));
  content::NavigationEntry* last_committed =
      web_contents->GetController().GetLastCommittedEntry();
  EXPECT_TRUE(last_committed);
  EXPECT_EQ(kPushStateURL, last_committed->GetURL());

  EXPECT_TRUE(push_state_observer.has_committed());
  EXPECT_TRUE(push_state_observer.was_same_document());
  EXPECT_TRUE(push_state_observer.was_renderer_initiated());

  // Let the navigation finish. It should commit successfully.
  manager.WaitForNavigationFinished();
  last_committed = web_contents->GetController().GetLastCommittedEntry();
  EXPECT_TRUE(last_committed);
  EXPECT_EQ(kURL2, last_committed->GetURL());

  EXPECT_TRUE(navigation_observer.has_committed());
  EXPECT_FALSE(navigation_observer.was_same_document());
  EXPECT_FALSE(navigation_observer.was_renderer_initiated());
}

// Check that if a page has an iframe that loads an error page, that error page
// does not inherit the Content Security Policy from the parent frame.  See
// https://crbug.com/703801.  This test is in chrome/ because error page
// behavior is only fully defined in chrome/.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest,
                       ErrorPageDoesNotInheritCSP) {
  GURL url(
      embedded_test_server()->GetURL("/page_with_csp_and_error_iframe.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Navigate to a page that disallows scripts via CSP and has an iframe that
  // tries to load an invalid URL, which results in an error page.
  GURL error_url("http://invalid.foo/");
  content::NavigationHandleObserver observer(web_contents, error_url);
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_TRUE(observer.has_committed());
  EXPECT_TRUE(observer.is_error());

  // The error page should not inherit the CSP directive that blocks all
  // scripts from the parent frame, so this script should be allowed to
  // execute.  Since ExecuteScript will execute the passed-in script regardless
  // of CSP, use a javascript: URL which does go through the CSP checks.
  content::RenderFrameHost* error_host =
      ChildFrameAt(web_contents->GetMainFrame(), 0);
  std::string location;
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      error_host,
      "location='javascript:domAutomationController.send(location.href)';",
      &location));
  EXPECT_EQ(location, content::kUnreachableWebDataURL);

  // The error page should have a unique origin.
  std::string origin;
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      error_host, "domAutomationController.send(document.origin);", &origin));
  EXPECT_EQ("null", origin);
}

// Test that web pages can't navigate to an error page URL, either directly or
// via a redirect, and that web pages can't embed error pages in iframes.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest,
                       NavigationToErrorURLIsDisallowed) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  GURL url(embedded_test_server()->GetURL("/title1.html"));
  ui_test_utils::NavigateToURL(browser(), url);
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  // Try navigating to the error page URL and make sure it is canceled and the
  // old URL remains the last committed one.
  GURL error_url(content::kUnreachableWebDataURL);
  EXPECT_TRUE(ExecuteScript(web_contents,
                            "location.href = '" + error_url.spec() + "';"));
  content::WaitForLoadStop(web_contents);
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  // Now try navigating to a URL that tries to redirect to the error page URL,
  // and make sure the redirect is blocked.  Note that DidStopLoading will
  // still fire after the redirect is canceled, so TestNavigationObserver can
  // be used to wait for it.
  GURL redirect_to_error_url(
      embedded_test_server()->GetURL("/server-redirect?" + error_url.spec()));
  content::TestNavigationObserver observer(web_contents);
  EXPECT_TRUE(ExecuteScript(
      web_contents, "location.href = '" + redirect_to_error_url.spec() + "';"));
  observer.Wait();
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  // Also ensure that a page can't embed an iframe for an error page URL.
  EXPECT_TRUE(ExecuteScript(web_contents,
                            "var frame = document.createElement('iframe');\n"
                            "frame.src = '" + error_url.spec() + "';\n"
                            "document.body.appendChild(frame);"));
  content::WaitForLoadStop(web_contents);
  content::RenderFrameHost* subframe_host =
      ChildFrameAt(web_contents->GetMainFrame(), 0);
  // The new subframe should remain blank without a committed URL.
  EXPECT_TRUE(subframe_host->GetLastCommittedURL().is_empty());
}

// This test ensures that navigating to a page that returns an error code and
// an empty document still shows Chrome's helpful error page instead of the
// empty document.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest,
                       EmptyDocumentWithErrorCode) {
  GURL url(embedded_test_server()->GetURL("/empty_with_404.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Wait for the navigation to complete.  The empty document should trigger
  // loading of the 404 error page, so check that the last committed entry was
  // indeed for the error page.
  content::TestNavigationObserver observer(web_contents);
  EXPECT_TRUE(
      ExecuteScript(web_contents, "location.href = '" + url.spec() + "';"));
  observer.Wait();
  EXPECT_FALSE(observer.last_navigation_succeeded());
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());
  EXPECT_TRUE(
      IsLastCommittedEntryOfPageType(web_contents, content::PAGE_TYPE_ERROR));

  // Verify that the error page has correct content.  This needs to wait for
  // the error page content to be populated asynchronously by scripts after
  // DidFinishLoad.
  while (true) {
    std::string content;
    EXPECT_TRUE(ExecuteScriptAndExtractString(
        web_contents,
        "domAutomationController.send("
        "    document.body ? document.body.innerText : '');",
        &content));
    if (content.find("HTTP ERROR 404") != std::string::npos)
      break;
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(), TestTimeouts::tiny_timeout());
    run_loop.Run();
  }
}

class SignInIsolationBrowserTest : public ChromeNavigationBrowserTest {
 public:
  SignInIsolationBrowserTest()
      : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {}
  ~SignInIsolationBrowserTest() override {}

  virtual void InitFeatureList() {
    feature_list_.InitAndEnableFeature(features::kSignInProcessIsolation);
  }

  void SetUp() override {
    InitFeatureList();
    https_server_.ServeFilesFromSourceDirectory("chrome/test/data");
    ASSERT_TRUE(https_server_.InitializeAndListen());
    ChromeNavigationBrowserTest::SetUp();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Override the sign-in URL so that it includes correct port from the test
    // server.
    command_line->AppendSwitchASCII(
        ::switches::kGaiaUrl,
        https_server()->GetURL("accounts.google.com", "/").spec());

    // Ignore cert errors so that the sign-in URL can be loaded from a site
    // other than localhost (the EmbeddedTestServer serves a certificate that
    // is valid for localhost).
    command_line->AppendSwitch(switches::kIgnoreCertificateErrors);

    ChromeNavigationBrowserTest::SetUpCommandLine(command_line);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    https_server_.StartAcceptingConnections();
    ChromeNavigationBrowserTest::SetUpOnMainThread();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

  bool HasSyntheticTrial(const std::string& trial_name) {
    std::vector<std::string> synthetic_trials;
    variations::GetSyntheticTrialGroupIdsAsString(&synthetic_trials);
    std::string trial_hash =
        base::StringPrintf("%x", variations::HashName(trial_name));

    for (auto entry : synthetic_trials) {
      if (base::StartsWith(entry, trial_hash, base::CompareCase::SENSITIVE))
        return true;
    }

    return false;
  }

  bool IsInSyntheticTrialGroup(const std::string& trial_name,
                               const std::string& trial_group) {
    std::vector<std::string> synthetic_trials;
    variations::GetSyntheticTrialGroupIdsAsString(&synthetic_trials);
    std::string expected_entry =
        base::StringPrintf("%x-%x", variations::HashName(trial_name),
                           variations::HashName(trial_group));

    for (auto entry : synthetic_trials) {
      if (entry == expected_entry)
        return true;
    }

    return false;
  }

  const std::string kSyntheticTrialName = "SignInProcessIsolationActive";

 protected:
  base::test::ScopedFeatureList feature_list_;

 private:
  net::EmbeddedTestServer https_server_;

  DISALLOW_COPY_AND_ASSIGN(SignInIsolationBrowserTest);
};

// This test ensures that the sign-in origin requires a dedicated process.  It
// only ensures that the corresponding base::Feature works properly;
// IsolatedOriginTest provides the main test coverage of origins whitelisted
// for process isolation.  See https://crbug.com/739418.
IN_PROC_BROWSER_TEST_F(SignInIsolationBrowserTest, NavigateToSignInPage) {
  const GURL first_url =
      embedded_test_server()->GetURL("google.com", "/title1.html");
  const GURL signin_url =
      https_server()->GetURL("accounts.google.com", "/title1.html");
  ui_test_utils::NavigateToURL(browser(), first_url);
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  scoped_refptr<content::SiteInstance> first_instance(
      web_contents->GetMainFrame()->GetSiteInstance());

  // Make sure that a renderer-initiated navigation to the sign-in page swaps
  // processes.
  content::TestNavigationManager manager(web_contents, signin_url);
  EXPECT_TRUE(
      ExecuteScript(web_contents, "location = '" + signin_url.spec() + "';"));
  manager.WaitForNavigationFinished();
  EXPECT_NE(web_contents->GetMainFrame()->GetSiteInstance(), first_instance);
}

// The next four tests verify that the synthetic field trial is set correctly
// for sign-in process isolation.  The synthetic field trial should be created
// when browsing to the sign-in URL for the first time, and it should reflect
// whether or not the sign-in isolation base::Feature is enabled, and whether
// or not it is force-enabled from the command line.
IN_PROC_BROWSER_TEST_F(SignInIsolationBrowserTest, SyntheticTrial) {
  EXPECT_FALSE(HasSyntheticTrial(kSyntheticTrialName));

  ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("foo.com", "/title1.html"));
  EXPECT_FALSE(HasSyntheticTrial(kSyntheticTrialName));

  GURL signin_url(
      https_server()->GetURL("accounts.google.com", "/title1.html"));

  // This test class uses InitAndEnableFeature, which overrides the feature
  // settings as if it came from the command line, so by default, browsing to
  // the sign-in URL should create the synthetic trial with ForceEnabled.
  ui_test_utils::NavigateToURL(browser(), signin_url);
  EXPECT_TRUE(IsInSyntheticTrialGroup(kSyntheticTrialName, "ForceEnabled"));
}

// This test class is used to check the synthetic sign-in trial for the Enabled
// group. It creates a new field trial (with 100% probability of being in the
// group), and initializes the test class's ScopedFeatureList using it, being
// careful to not override it using the command line (which corresponds to
// ForceEnabled).
class EnabledSignInIsolationBrowserTest : public SignInIsolationBrowserTest {
 public:
  EnabledSignInIsolationBrowserTest() {}
  ~EnabledSignInIsolationBrowserTest() override {}

  void InitFeatureList() override {}

  void SetUpOnMainThread() override {
    const std::string kTrialName = "SignInProcessIsolation";
    const std::string kGroupName = "FooGroup";  // unused
    scoped_refptr<base::FieldTrial> trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        features::kSignInProcessIsolation.name,
        base::FeatureList::OverrideState::OVERRIDE_ENABLE_FEATURE, trial.get());

    feature_list_.InitWithFeatureList(std::move(feature_list));
    SignInIsolationBrowserTest::SetUpOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // This test creates and tests its own field trial group, so it needs to
    // disable the field trial testing config, which might define an
    // incompatible trial name/group.
    command_line->AppendSwitch(
        variations::switches::kDisableFieldTrialTestingConfig);
    SignInIsolationBrowserTest::SetUpCommandLine(command_line);
  }

  DISALLOW_COPY_AND_ASSIGN(EnabledSignInIsolationBrowserTest);
};

IN_PROC_BROWSER_TEST_F(EnabledSignInIsolationBrowserTest, SyntheticTrial) {
  EXPECT_FALSE(HasSyntheticTrial(kSyntheticTrialName));
  EXPECT_FALSE(IsInSyntheticTrialGroup(kSyntheticTrialName, "Enabled"));

  GURL signin_url =
      https_server()->GetURL("accounts.google.com", "/title1.html");
  ui_test_utils::NavigateToURL(browser(), signin_url);
  EXPECT_TRUE(IsInSyntheticTrialGroup(kSyntheticTrialName, "Enabled"));

  // A repeat navigation shouldn't change the synthetic trial.
  ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("accounts.google.com", "/title2.html"));
  EXPECT_TRUE(IsInSyntheticTrialGroup(kSyntheticTrialName, "Enabled"));
}

// This test class is similar to EnabledSignInIsolationBrowserTest, but for the
// Disabled group of the synthetic sign-in trial.
class DisabledSignInIsolationBrowserTest : public SignInIsolationBrowserTest {
 public:
  DisabledSignInIsolationBrowserTest() {}
  ~DisabledSignInIsolationBrowserTest() override {}

  void InitFeatureList() override {}

  void SetUpOnMainThread() override {
    const std::string kTrialName = "SignInProcessIsolation";
    const std::string kGroupName = "FooGroup";  // unused
    scoped_refptr<base::FieldTrial> trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        features::kSignInProcessIsolation.name,
        base::FeatureList::OverrideState::OVERRIDE_DISABLE_FEATURE,
        trial.get());

    feature_list_.InitWithFeatureList(std::move(feature_list));
    SignInIsolationBrowserTest::SetUpOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // This test creates and tests its own field trial group, so it needs to
    // disable the field trial testing config, which might define an
    // incompatible trial name/group.
    command_line->AppendSwitch(
        variations::switches::kDisableFieldTrialTestingConfig);
    SignInIsolationBrowserTest::SetUpCommandLine(command_line);
  }

  DISALLOW_COPY_AND_ASSIGN(DisabledSignInIsolationBrowserTest);
};

IN_PROC_BROWSER_TEST_F(DisabledSignInIsolationBrowserTest, SyntheticTrial) {
  EXPECT_FALSE(IsInSyntheticTrialGroup(kSyntheticTrialName, "Disabled"));
  GURL signin_url =
      https_server()->GetURL("accounts.google.com", "/title1.html");
  ui_test_utils::NavigateToURL(browser(), signin_url);
  EXPECT_TRUE(IsInSyntheticTrialGroup(kSyntheticTrialName, "Disabled"));
}

// This test class is similar to EnabledSignInIsolationBrowserTest, but for the
// ForceDisabled group of the synthetic sign-in trial.
class ForceDisabledSignInIsolationBrowserTest
    : public SignInIsolationBrowserTest {
 public:
  ForceDisabledSignInIsolationBrowserTest() {}
  ~ForceDisabledSignInIsolationBrowserTest() override {}

  void InitFeatureList() override {
    feature_list_.InitAndDisableFeature(features::kSignInProcessIsolation);
  }

  DISALLOW_COPY_AND_ASSIGN(ForceDisabledSignInIsolationBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ForceDisabledSignInIsolationBrowserTest,
                       SyntheticTrial) {
  // Test subframe navigation in this case, since that should also trigger
  // synthetic trial creation.
  ui_test_utils::NavigateToURL(browser(),
                               https_server()->GetURL("a.com", "/iframe.html"));
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_FALSE(IsInSyntheticTrialGroup(kSyntheticTrialName, "ForceDisabled"));
  GURL signin_url =
      https_server()->GetURL("accounts.google.com", "/title1.html");
  EXPECT_TRUE(NavigateIframeToURL(web_contents, "test", signin_url));
  EXPECT_TRUE(IsInSyntheticTrialGroup(kSyntheticTrialName, "ForceDisabled"));
}

// Helper class. Track one navigation and tell whether a response from the
// server has been received or not. It is useful for discerning navigations
// blocked after or before the request has been sent.
class WillProcessResponseObserver : public content::WebContentsObserver {
 public:
  explicit WillProcessResponseObserver(content::WebContents* web_contents,
                                       const GURL& url)
      : content::WebContentsObserver(web_contents), url_(url) {}
  ~WillProcessResponseObserver() override {}

  bool WillProcessResponseCalled() { return will_process_response_called_; }

 private:
  GURL url_;
  bool will_process_response_called_ = false;

  // Is used to set |will_process_response_called_| to true when
  // NavigationThrottle::WillProcessResponse() is called.
  class WillProcessResponseObserverThrottle
      : public content::NavigationThrottle {
   public:
    WillProcessResponseObserverThrottle(content::NavigationHandle* handle,
                                        bool* will_process_response_called)
        : NavigationThrottle(handle),
          will_process_response_called_(will_process_response_called) {}

    const char* GetNameForLogging() override {
      return "WillProcessResponseObserverThrottle";
    }

   private:
    bool* will_process_response_called_;
    NavigationThrottle::ThrottleCheckResult WillProcessResponse() override {
      *will_process_response_called_ = true;
      return NavigationThrottle::PROCEED;
    }
  };

  // WebContentsObserver
  void DidStartNavigation(content::NavigationHandle* handle) override {
    if (handle->GetURL() == url_) {
      handle->RegisterThrottleForTesting(
          std::make_unique<WillProcessResponseObserverThrottle>(
              handle, &will_process_response_called_));
    }
  }
};

// In HTTP/HTTPS documents, check that no request with the "ftp:" scheme are
// submitted to load an iframe.
// See https://crbug.com/757809.
// Note: This test couldn't be a content_browsertests, since there would be
// not handler defined for the "ftp" protocol in
// URLRequestJobFactoryImpl::protocol_handler_map_.
// Flaky on Mac only.  http://crbug.com/816646
#if defined(OS_MACOSX)
#define MAYBE_BlockLegacySubresources DISABLED_BlockLegacySubresources
#else
#define MAYBE_BlockLegacySubresources BlockLegacySubresources
#endif
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest,
                       MAYBE_BlockLegacySubresources) {
  net::SpawnedTestServer ftp_server(
      net::SpawnedTestServer::TYPE_FTP,
      base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  ASSERT_TRUE(ftp_server.Start());

  GURL main_url_http(embedded_test_server()->GetURL("/iframe.html"));
  GURL main_url_ftp(ftp_server.GetURL("iframe.html"));
  GURL iframe_url_http(embedded_test_server()->GetURL("/simple.html"));
  GURL iframe_url_ftp(ftp_server.GetURL("simple.html"));
  GURL redirect_url(embedded_test_server()->GetURL("/server-redirect?"));

  struct {
    GURL main_url;
    GURL iframe_url;
    bool allowed;
  } kTestCases[] = {
      {main_url_http, iframe_url_http, true},
      {main_url_http, iframe_url_ftp, false},
      {main_url_ftp, iframe_url_http, true},
      {main_url_ftp, iframe_url_ftp, true},
  };
  for (const auto test_case : kTestCases) {
    // Blocking the request should work, even after a redirect.
    for (bool redirect : {false, true}) {
      GURL iframe_url =
          redirect ? GURL(redirect_url.spec() + test_case.iframe_url.spec())
                   : test_case.iframe_url;
      SCOPED_TRACE(::testing::Message()
                   << std::endl
                   << "- main_url = " << test_case.main_url << std::endl
                   << "- iframe_url = " << iframe_url << std::endl);

      ui_test_utils::NavigateToURL(browser(), test_case.main_url);
      content::WebContents* web_contents =
          browser()->tab_strip_model()->GetActiveWebContents();
      content::NavigationHandleObserver navigation_handle_observer(web_contents,
                                                                   iframe_url);
      WillProcessResponseObserver will_process_response_observer(web_contents,
                                                                 iframe_url);
      EXPECT_TRUE(NavigateIframeToURL(web_contents, "test", iframe_url));

      if (test_case.allowed) {
        EXPECT_TRUE(will_process_response_observer.WillProcessResponseCalled());
        EXPECT_FALSE(navigation_handle_observer.is_error());
        EXPECT_EQ(test_case.iframe_url,
                  navigation_handle_observer.last_committed_url());
      } else {
        EXPECT_FALSE(
            will_process_response_observer.WillProcessResponseCalled());
        EXPECT_TRUE(navigation_handle_observer.is_error());
        EXPECT_EQ(net::ERR_ABORTED,
                  navigation_handle_observer.net_error_code());
      }
    }
  }
}

// Check that it's possible to navigate to a chrome scheme URL from a crashed
// tab. See https://crbug.com/764641.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest, ChromeSchemeNavFromSadTab) {
  // Kill the renderer process.
  content::RenderProcessHost* process = browser()
                                            ->tab_strip_model()
                                            ->GetActiveWebContents()
                                            ->GetMainFrame()
                                            ->GetProcess();
  content::RenderProcessHostWatcher crash_observer(
      process, content::RenderProcessHostWatcher::WATCH_FOR_PROCESS_EXIT);
  process->Shutdown(-1);
  crash_observer.Wait();

  // Attempt to navigate to a chrome://... URL.  This used to hang and never
  // commit in PlzNavigate mode.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIVersionURL));
}

// Check that a browser-initiated navigation to a cross-site URL that then
// redirects to a pdf hosted on another site works.
IN_PROC_BROWSER_TEST_F(ChromeNavigationBrowserTest, CrossSiteRedirectionToPDF) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.AddDefaultHandlers(
      base::FilePath(FILE_PATH_LITERAL("chrome/test/data")));
  ASSERT_TRUE(https_server.Start());

  GURL initial_url = embedded_test_server()->GetURL("/title1.html");
  GURL pdf_url = embedded_test_server()->GetURL("/pdf/test.pdf");
  GURL cross_site_redirecting_url =
      https_server.GetURL("/server-redirect?" + pdf_url.spec());
  ui_test_utils::NavigateToURL(browser(), initial_url);
  ui_test_utils::NavigateToURL(browser(), cross_site_redirecting_url);
  EXPECT_EQ(pdf_url, browser()
                         ->tab_strip_model()
                         ->GetActiveWebContents()
                         ->GetLastCommittedURL());
}

// TODO(csharrison): These tests should become tentative WPT, once the feature
// is enabled by default.
class NavigationConsumingTest : public ChromeNavigationBrowserTest {
  void SetUpCommandLine(base::CommandLine* cmd_line) override {
    ChromeNavigationBrowserTest::SetUpCommandLine(cmd_line);
    scoped_feature_.InitFromCommandLine("ConsumeGestureOnNavigation",
                                        std::string());
  }
  base::test::ScopedFeatureList scoped_feature_;
};

// The fullscreen API is spec'd to require a user activation (aka user gesture),
// so use that API to test if navigation consumes the activation.
// https://fullscreen.spec.whatwg.org/#allowed-to-request-fullscreen
IN_PROC_BROWSER_TEST_F(NavigationConsumingTest,
                       NavigationConsumesUserGesture_Fullscreen) {
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("/navigation_consumes_gesture.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Normally, fullscreen should work, as long as there is a user gesture.
  bool is_fullscreen = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      contents, "document.body.webkitRequestFullscreen();", &is_fullscreen));
  EXPECT_TRUE(is_fullscreen);

  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      contents, "document.webkitExitFullscreen();", &is_fullscreen));
  EXPECT_FALSE(is_fullscreen);

  EXPECT_TRUE(content::ExecuteScriptWithoutUserGestureAndExtractBool(
      contents, "document.body.webkitRequestFullscreen();", &is_fullscreen));
  EXPECT_FALSE(is_fullscreen);

  // However, starting a navigation should consume the gesture. Fullscreen
  // should not work afterwards. Make sure the navigation is synchronously
  // started via click().
  std::string script = R"(
    document.getElementsByTagName('a')[0].click();
    document.body.webkitRequestFullscreen();
  )";

  // Use the TestNavigationManager to ensure the navigation is not finished
  // before fullscreen can occur.
  content::TestNavigationManager nav_manager(
      contents, embedded_test_server()->GetURL("/title1.html"));
  EXPECT_TRUE(
      content::ExecuteScriptAndExtractBool(contents, script, &is_fullscreen));
  EXPECT_FALSE(is_fullscreen);
}

// Similar to the fullscreen test above, but checks that popups are successfully
// blocked if spawned after a navigation.
IN_PROC_BROWSER_TEST_F(NavigationConsumingTest,
                       NavigationConsumesUserGesture_Popups) {
  ui_test_utils::NavigateToURL(browser(),
                               embedded_test_server()->GetURL("/links.html"));
  content::WebContents* contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  // Normally, a popup should open fine if it is associated with a user gesture.
  bool did_open = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      contents, "window.domAutomationController.send(!!window.open());",
      &did_open));
  EXPECT_TRUE(did_open);

  // Starting a navigation should consume a gesture, but make sure that starting
  // a same-document navigation doesn't do the consuming.
  std::string same_document_script = R"(
    document.getElementById("ref").click();
    window.domAutomationController.send(!!window.open());
  )";
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      contents, same_document_script, &did_open));
  EXPECT_TRUE(did_open);

  // If the navigation is to a different document, the gesture should be
  // successfully consumed.
  std::string different_document_script = R"(
    document.getElementById("title1").click();
    window.domAutomationController.send(!!window.open());
  )";
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      contents, different_document_script, &did_open));
  EXPECT_FALSE(did_open);
}
