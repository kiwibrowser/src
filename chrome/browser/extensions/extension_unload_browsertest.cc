// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/base/window_open_disposition.h"

namespace extensions {

class ExtensionUnloadBrowserTest : public ExtensionBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("maps.google.com", "127.0.0.1");
  }
};

IN_PROC_BROWSER_TEST_F(ExtensionUnloadBrowserTest, TestUnload) {
  // Load an extension that installs unload and beforeunload listeners.
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("unload_listener"));
  ASSERT_TRUE(extension);
  std::string id = extension->id();
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  GURL initial_tab_url =
      browser()->tab_strip_model()->GetWebContentsAt(0)->GetLastCommittedURL();
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), extension->GetResourceURL("page.html"),
      WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  EXPECT_EQ(2, browser()->tab_strip_model()->count());
  DisableExtension(id);
  // There should only be one remaining web contents - the initial one.
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(
      initial_tab_url,
      browser()->tab_strip_model()->GetWebContentsAt(0)->GetLastCommittedURL());
}

// After an extension is uninstalled, network requests from its content scripts
// should fail but not kill the renderer process.
IN_PROC_BROWSER_TEST_F(ExtensionUnloadBrowserTest, UnloadWithContentScripts) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Load an extension with a content script that has a button to send XHRs.
  const Extension* extension =
      LoadExtension(test_data_dir_.AppendASCII("xhr_from_content_script"));
  ASSERT_TRUE(extension);
  std::string id = extension->id();
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  GURL test_url = embedded_test_server()->GetURL("/title1.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  // Sending an XHR with the extension's Origin header should succeed when the
  // extension is installed.
  const char kSendXhrScript[] = "document.getElementById('xhrButton').click();";
  bool xhr_result = false;
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(), kSendXhrScript,
      &xhr_result));
  EXPECT_TRUE(xhr_result);

  DisableExtension(id);

  // The tab should still be open with the content script injected.
  ASSERT_EQ(1, browser()->tab_strip_model()->count());
  EXPECT_EQ(
      test_url,
      browser()->tab_strip_model()->GetWebContentsAt(0)->GetLastCommittedURL());

  // Sending an XHR with the extension's Origin header should fail but not kill
  // the tab.
  EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
      browser()->tab_strip_model()->GetActiveWebContents(), kSendXhrScript,
      &xhr_result));
  EXPECT_FALSE(xhr_result);

  // Ensure the process has not been killed.
  EXPECT_TRUE(browser()
                  ->tab_strip_model()
                  ->GetActiveWebContents()
                  ->GetMainFrame()
                  ->IsRenderFrameLive());
}

// TODO(devlin): Investigate what to do for embedded iframes.

}  // namespace extensions
