// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace content {

namespace {

class DoNotTrackTest : public ContentBrowserTest {
 protected:
  void EnableDoNotTrack() {
    RendererPreferences* prefs =
        shell()->web_contents()->GetMutableRendererPrefs();
    EXPECT_FALSE(prefs->enable_do_not_track);
    prefs->enable_do_not_track = true;
  }

  void ExpectPageTextEq(const std::string& expected_content) {
    std::string text;
    ASSERT_TRUE(ExecuteScriptAndExtractString(
        shell(),
        "window.domAutomationController.send(document.body.innerText);",
        &text));
    EXPECT_EQ(expected_content, text);
  }

  std::string GetDOMDoNotTrackProperty() {
    std::string value;
    EXPECT_TRUE(ExecuteScriptAndExtractString(
        shell(),
        "window.domAutomationController.send("
        "    navigator.doNotTrack === null ? '' : navigator.doNotTrack)",
        &value));
    return value;
  }
};

// Checks that the DNT header is not sent by default.
IN_PROC_BROWSER_TEST_F(DoNotTrackTest, NotEnabled) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/echoheader?DNT");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  ExpectPageTextEq("None");
  // And the DOM property is not set.
  EXPECT_EQ("", GetDOMDoNotTrackProperty());
}

// Checks that the DNT header is sent when the corresponding preference is set.
IN_PROC_BROWSER_TEST_F(DoNotTrackTest, Simple) {
  ASSERT_TRUE(embedded_test_server()->Start());
  EnableDoNotTrack();
  GURL url = embedded_test_server()->GetURL("/echoheader?DNT");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  ExpectPageTextEq("1");
}

// Checks that the DNT header is preserved during redirects.
IN_PROC_BROWSER_TEST_F(DoNotTrackTest, Redirect) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL final_url = embedded_test_server()->GetURL("/echoheader?DNT");
  GURL url = embedded_test_server()->GetURL(std::string("/server-redirect?") +
                                            final_url.spec());
  EnableDoNotTrack();
  // We don't check the result NavigateToURL as it returns true only if the
  // final URL is equal to the passed URL.
  NavigateToURL(shell(), url);
  ExpectPageTextEq("1");
}

// Checks that the DOM property is set when the corresponding preference is set.
IN_PROC_BROWSER_TEST_F(DoNotTrackTest, DOMProperty) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/echo");
  EnableDoNotTrack();
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ("1", GetDOMDoNotTrackProperty());
}

}  // namespace

}  // namespace content