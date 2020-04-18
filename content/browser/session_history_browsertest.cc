// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// Handles |request| by serving a response with title set to request contents.
std::unique_ptr<net::test_server::HttpResponse> HandleEchoTitleRequest(
    const std::string& echotitle_path,
    const net::test_server::HttpRequest& request) {
  if (!base::StartsWith(request.relative_url, echotitle_path,
                        base::CompareCase::SENSITIVE))
    return std::unique_ptr<net::test_server::HttpResponse>();

  std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
      new net::test_server::BasicHttpResponse);
  http_response->set_code(net::HTTP_OK);
  http_response->set_content(
      base::StringPrintf(
          "<html><head><title>%s</title></head></html>",
          request.content.c_str()));
  return std::move(http_response);
}

}  // namespace

class SessionHistoryTest : public ContentBrowserTest {
 protected:
  SessionHistoryTest() {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");

    SetupCrossSiteRedirector(embedded_test_server());
    embedded_test_server()->RegisterRequestHandler(
        base::Bind(&HandleEchoTitleRequest, "/echotitle"));

    ASSERT_TRUE(embedded_test_server()->Start());
    NavigateToURL(shell(), GURL(url::kAboutBlankURL));
  }

  // Simulate clicking a link.  Only works on the frames.html testserver page.
  void ClickLink(const std::string& node_id) {
    GURL url("javascript:clickLink('" + node_id + "')");
    NavigateToURL(shell(), url);
  }

  // Simulate filling in form data.  Only works on the frames.html page with
  // subframe = form.html, and on form.html itself.
  void FillForm(const std::string& node_id, const std::string& value) {
    GURL url("javascript:fillForm('" + node_id + "', '" + value + "')");
    // This will return immediately, but since the JS executes synchronously
    // on the renderer, it will complete before the next navigate message is
    // processed.
    NavigateToURL(shell(), url);
  }

  // Simulate submitting a form.  Only works on the frames.html page with
  // subframe = form.html, and on form.html itself.
  void SubmitForm(const std::string& node_id) {
    GURL url("javascript:submitForm('" + node_id + "')");
    NavigateToURL(shell(), url);
  }

  // Navigate session history using history.go(distance).
  void JavascriptGo(const std::string& distance) {
    GURL url("javascript:history.go('" + distance + "')");
    NavigateToURL(shell(), url);
  }

  std::string GetTabTitle() {
    return base::UTF16ToASCII(shell()->web_contents()->GetTitle());
  }

  GURL GetTabURL() {
    return shell()->web_contents()->GetLastCommittedURL();
  }

  GURL GetURL(const std::string& file) {
    return embedded_test_server()->GetURL(
        std::string("/session_history/") + file);
  }

  void NavigateAndCheckTitle(const char* filename,
                             const std::string& expected_title) {
    base::string16 expected_title16(base::ASCIIToUTF16(expected_title));
    TitleWatcher title_watcher(shell()->web_contents(), expected_title16);
    NavigateToURL(shell(), GetURL(filename));
    ASSERT_EQ(expected_title16, title_watcher.WaitAndGetTitle());
  }

  bool CanGoBack() {
    return shell()->web_contents()->GetController().CanGoBack();
  }

  bool CanGoForward() {
    return shell()->web_contents()->GetController().CanGoForward();
  }

  void GoBack() {
    WindowedNotificationObserver load_stop_observer(
        NOTIFICATION_LOAD_STOP,
        NotificationService::AllSources());
    shell()->web_contents()->GetController().GoBack();
    load_stop_observer.Wait();
  }

  void GoForward() {
    WindowedNotificationObserver load_stop_observer(
        NOTIFICATION_LOAD_STOP,
        NotificationService::AllSources());
    shell()->web_contents()->GetController().GoForward();
    load_stop_observer.Wait();
  }
};

class SessionHistoryScrollAnchorTest : public SessionHistoryTest {
 protected:
  SessionHistoryScrollAnchorTest() = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    SessionHistoryTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII("enable-blink-features",
                                    "ScrollAnchorSerialization");
  }
};

// If this flakes, use http://crbug.com/61619 on windows and
// http://crbug.com/102094 on mac.
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, BasicBackForward) {
  ASSERT_FALSE(CanGoBack());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot1.html", "bot1"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot2.html", "bot2"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot3.html", "bot3"));

  // history is [blank, bot1, bot2, *bot3]

  GoBack();
  EXPECT_EQ("bot2", GetTabTitle());

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());

  GoForward();
  EXPECT_EQ("bot2", GetTabTitle());

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot3.html", "bot3"));

  // history is [blank, bot1, *bot3]

  ASSERT_FALSE(CanGoForward());
  EXPECT_EQ("bot3", GetTabTitle());

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());

  GoBack();
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  ASSERT_FALSE(CanGoBack());
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  GoForward();
  EXPECT_EQ("bot1", GetTabTitle());

  GoForward();
  EXPECT_EQ("bot3", GetTabTitle());
}

// Test that back/forward works when navigating in subframes.
// If this flakes, use http://crbug.com/48833
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, FrameBackForward) {
  ASSERT_FALSE(CanGoBack());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("frames.html", "bot1"));

  ClickLink("abot2");
  EXPECT_EQ("bot2", GetTabTitle());
  GURL frames(GetURL("frames.html"));
  EXPECT_EQ(frames, GetTabURL());

  ClickLink("abot3");
  EXPECT_EQ("bot3", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, bot2, *bot3]

  GoBack();
  EXPECT_EQ("bot2", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());
  EXPECT_EQ(GURL(url::kAboutBlankURL), GetTabURL());

  GoForward();
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoForward();
  EXPECT_EQ("bot2", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  ClickLink("abot1");
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, bot2, *bot1]

  ASSERT_FALSE(CanGoForward());
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("bot2", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());
}

// Test that back/forward preserves POST data and document state in subframes.
// If this flakes use http://crbug.com/61619
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, FrameFormBackForward) {
  ASSERT_FALSE(CanGoBack());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("frames.html", "bot1"));

  ClickLink("aform");
  EXPECT_EQ("form", GetTabTitle());
  GURL frames(GetURL("frames.html"));
  EXPECT_EQ(frames, GetTabURL());

  SubmitForm("isubmit");
  EXPECT_EQ("text=&select=a", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("form", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, *form, post]

  ClickLink("abot2");
  EXPECT_EQ("bot2", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, form, *bot2]

  GoBack();
  EXPECT_EQ("form", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  SubmitForm("isubmit");
  EXPECT_EQ("text=&select=a", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, form, *post]

  // TODO(mpcomplete): reenable this when WebKit bug 10199 is fixed:
  // "returning to a POST result within a frame does a GET instead of a POST"
  ClickLink("abot2");
  EXPECT_EQ("bot2", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("text=&select=a", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());
}

// TODO(mpcomplete): enable this when Bug 734372 is fixed:
// "Doing a session history navigation does not restore newly-created subframe
// document state"
// Test that back/forward preserves POST data and document state when navigating
// across frames (ie, from frame -> nonframe).
// Hangs, see http://crbug.com/45058.
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, CrossFrameFormBackForward) {
  ASSERT_FALSE(CanGoBack());

  GURL frames(GetURL("frames.html"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("frames.html", "bot1"));

  ClickLink("aform");
  EXPECT_EQ("form", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  SubmitForm("isubmit");
  EXPECT_EQ("text=&select=a", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  GoBack();
  EXPECT_EQ("form", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  // history is [blank, bot1, *form, post]

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot2.html", "bot2"));

  // history is [blank, bot1, form, *bot2]

  GoBack();
  EXPECT_EQ("bot1", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());

  SubmitForm("isubmit");
  EXPECT_EQ("text=&select=a", GetTabTitle());
  EXPECT_EQ(frames, GetTabURL());
}

// Test that back/forward entries are created for reference fragment
// navigations. Bug 730379.
// If this flakes use http://crbug.com/61619.
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, FragmentBackForward) {
  ASSERT_FALSE(CanGoBack());

  GURL fragment(GetURL("fragment.html"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("fragment.html", "fragment"));

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("fragment.html#a", "fragment"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("fragment.html#b", "fragment"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("fragment.html#c", "fragment"));

  // history is [blank, fragment, fragment#a, fragment#b, *fragment#c]

  GoBack();
  EXPECT_EQ(GetURL("fragment.html#b"), GetTabURL());

  GoBack();
  EXPECT_EQ(GetURL("fragment.html#a"), GetTabURL());

  GoBack();
  EXPECT_EQ(GetURL("fragment.html"), GetTabURL());

  GoForward();
  EXPECT_EQ(GetURL("fragment.html#a"), GetTabURL());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot3.html", "bot3"));

  // history is [blank, fragment, fragment#a, bot3]

  ASSERT_FALSE(CanGoForward());
  EXPECT_EQ(GetURL("bot3.html"), GetTabURL());

  GoBack();
  EXPECT_EQ(GetURL("fragment.html#a"), GetTabURL());

  GoBack();
  EXPECT_EQ(GetURL("fragment.html"), GetTabURL());
}

// Test that the javascript window.history object works.
// NOTE: history.go(N) does not do anything if N is outside the bounds of the
// back/forward list (such as trigger our start/stop loading events).  This
// means the test will hang if it attempts to navigate too far forward or back,
// since we'll be waiting forever for a load stop event.
//
// TODO(brettw) bug 50648: fix flakyness. This test seems like it was failing
// about 1/4 of the time on Vista by failing to execute JavascriptGo (see bug).
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, JavascriptHistory) {
  ASSERT_FALSE(CanGoBack());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot1.html", "bot1"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot2.html", "bot2"));
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot3.html", "bot3"));

  // history is [blank, bot1, bot2, *bot3]

  JavascriptGo("-1");
  EXPECT_EQ("bot2", GetTabTitle());

  JavascriptGo("-1");
  EXPECT_EQ("bot1", GetTabTitle());

  JavascriptGo("1");
  EXPECT_EQ("bot2", GetTabTitle());

  JavascriptGo("-1");
  EXPECT_EQ("bot1", GetTabTitle());

  JavascriptGo("2");
  EXPECT_EQ("bot3", GetTabTitle());

  // history is [blank, bot1, bot2, *bot3]

  JavascriptGo("-3");
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  ASSERT_FALSE(CanGoBack());
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  JavascriptGo("1");
  EXPECT_EQ("bot1", GetTabTitle());

  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle("bot3.html", "bot3"));

  // history is [blank, bot1, *bot3]

  ASSERT_FALSE(CanGoForward());
  EXPECT_EQ("bot3", GetTabTitle());

  JavascriptGo("-1");
  EXPECT_EQ("bot1", GetTabTitle());

  JavascriptGo("-1");
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  ASSERT_FALSE(CanGoBack());
  EXPECT_EQ(std::string(url::kAboutBlankURL), GetTabTitle());

  JavascriptGo("1");
  EXPECT_EQ("bot1", GetTabTitle());

  JavascriptGo("1");
  EXPECT_EQ("bot3", GetTabTitle());

  // TODO(creis): Test that JavaScript history navigations work across tab
  // types.  For example, load about:network in a tab, then a real page, then
  // try to go back and forward with JavaScript.  Bug 1136715.
  // (Hard to test right now, because pages like about:network cause the
  // TabProxy to hang.  This is because they do not appear to use the
  // NotificationService.)
}

// This test is failing consistently. See http://crbug.com/22560
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, LocationReplace) {
  // Test that using location.replace doesn't leave the title of the old page
  // visible.
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle(
      "replace.html?bot1.html", "bot1"));
}

IN_PROC_BROWSER_TEST_F(SessionHistoryTest, LocationChangeInSubframe) {
  ASSERT_NO_FATAL_FAILURE(NavigateAndCheckTitle(
      "location_redirect.html", "Default Title"));

  NavigateToURL(shell(), GURL("javascript:void(frames[0].navigate())"));
  EXPECT_EQ("foo", GetTabTitle());

  GoBack();
  EXPECT_EQ("Default Title", GetTabTitle());
}

IN_PROC_BROWSER_TEST_F(SessionHistoryScrollAnchorTest,
                       LocationChangeInSubframe) {
  ASSERT_NO_FATAL_FAILURE(
      NavigateAndCheckTitle("location_redirect.html", "Default Title"));

  NavigateToURL(shell(), GURL("javascript:void(frames[0].navigate())"));
  EXPECT_EQ("foo", GetTabTitle());

  GoBack();
  EXPECT_EQ("Default Title", GetTabTitle());
}

// http://code.google.com/p/chromium/issues/detail?id=56267
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, HistoryLength) {
  int length;
  ASSERT_TRUE(ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(history.length)", &length));
  EXPECT_EQ(1, length);

  NavigateToURL(shell(), GetURL("title1.html"));

  ASSERT_TRUE(ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(history.length)", &length));
  EXPECT_EQ(2, length);

  // Now test that history.length is updated when the navigation is committed.
  NavigateToURL(shell(), GetURL("record_length.html"));

  ASSERT_TRUE(ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(history.length)", &length));
  EXPECT_EQ(3, length);

  GoBack();
  GoBack();

  // Ensure history.length is properly truncated.
  NavigateToURL(shell(), GetURL("title2.html"));

  ASSERT_TRUE(ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(history.length)", &length));
  EXPECT_EQ(2, length);
}

// Test that verifies that a cross-process transfer doesn't lose session
// history state - https://crbug.com/613004.
//
// Trigerring a cross-process transfer via embedded_test_server requires use of
// a HTTP redirect response (to preserve port number).  Therefore the test ends
// up accidentally testing redirection logic as well - in particular, the test
// uses 307 (rather than 302) redirect to preserve the body of HTTP POST across
// redirects (as mandated by https://tools.ietf.org/html/rfc7231#section-6.4.7).
IN_PROC_BROWSER_TEST_F(SessionHistoryTest, GoBackToCrossSitePostWithRedirect) {
  GURL form_url(embedded_test_server()->GetURL(
      "a.com", "/form_that_posts_cross_site.html"));
  GURL redirect_target_url(embedded_test_server()->GetURL("x.com", "/echoall"));
  GURL page_to_go_back_from(
      embedded_test_server()->GetURL("c.com", "/title1.html"));

  // Navigate to the page with form that posts via 307 redirection to
  // |redirect_target_url| (cross-site from |form_url|).
  EXPECT_TRUE(NavigateToURL(shell(), form_url));

  // Submit the form.
  TestNavigationObserver form_post_observer(shell()->web_contents(), 1);
  EXPECT_TRUE(
      ExecuteScript(shell(), "document.getElementById('text-form').submit();"));
  form_post_observer.Wait();

  // Verify that we arrived at the expected, redirected location.
  EXPECT_EQ(redirect_target_url,
            shell()->web_contents()->GetLastCommittedURL());

  // Verify that POST body got preserved by 307 redirect.  This expectation
  // comes from: https://tools.ietf.org/html/rfc7231#section-6.4.7
  std::string body;
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      shell(),
      "window.domAutomationController.send("
      "document.getElementsByTagName('pre')[0].innerText);",
      &body));
  EXPECT_EQ("text=value\n", body);

  // Navigate to a page from yet another site.
  EXPECT_TRUE(NavigateToURL(shell(), page_to_go_back_from));

  // Go back - this should resubmit form's post data.
  TestNavigationObserver back_nav_observer(shell()->web_contents(), 1);
  shell()->web_contents()->GetController().GoBack();
  back_nav_observer.Wait();

  // Again verify that we arrived at the expected, redirected location.
  EXPECT_EQ(redirect_target_url,
            shell()->web_contents()->GetLastCommittedURL());

  // Again verify that POST body got preserved by 307 redirect.
  std::string body_after_back_navigation;
  EXPECT_TRUE(ExecuteScriptAndExtractString(
      shell(),
      "window.domAutomationController.send("
      "document.getElementsByTagName('pre')[0].innerText);",
      &body_after_back_navigation));
  EXPECT_EQ("text=value\n", body_after_back_navigation);
}

}  // namespace content
