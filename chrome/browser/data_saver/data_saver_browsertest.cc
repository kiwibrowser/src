// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_test_base.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

class DataSaverBrowserTest : public InProcessBrowserTest {
 protected:
  void EnableDataSaver(bool enabled) {
    PrefService* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(prefs::kDataSaverEnabled, enabled);
    // Give the setting notification a chance to propagate.
    content::RunAllPendingInMessageLoop();
  }

  void VerifySaveDataHeader(const std::string& expected_header_value) {
    ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/echoheader?Save-Data"));
    std::string header_value;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        browser()->tab_strip_model()->GetActiveWebContents(),
        "window.domAutomationController.send(document.body.textContent);",
        &header_value));
    EXPECT_EQ(expected_header_value, header_value);
  }
};

IN_PROC_BROWSER_TEST_F(DataSaverBrowserTest, DataSaverEnabled) {
  ASSERT_TRUE(embedded_test_server()->Start());
  EnableDataSaver(true);
  VerifySaveDataHeader("on");
}

IN_PROC_BROWSER_TEST_F(DataSaverBrowserTest, DataSaverDisabled) {
  ASSERT_TRUE(embedded_test_server()->Start());
  EnableDataSaver(false);
  VerifySaveDataHeader("None");
}

class DataSaverWithServerBrowserTest : public InProcessBrowserTest {
 protected:
  void Init() {
    test_server_.reset(new net::EmbeddedTestServer());
    test_server_->RegisterRequestHandler(
        base::Bind(&DataSaverWithServerBrowserTest::VerifySaveDataHeader,
                   base::Unretained(this)));
    test_server_->ServeFilesFromSourceDirectory("chrome/test/data");
  }
  void EnableDataSaver(bool enabled) {
    PrefService* prefs = browser()->profile()->GetPrefs();
    prefs->SetBoolean(prefs::kDataSaverEnabled, enabled);
    // Give the setting notification a chance to propagate.
    content::RunAllPendingInMessageLoop();
  }

  std::unique_ptr<net::test_server::HttpResponse> VerifySaveDataHeader(
      const net::test_server::HttpRequest& request) {
    auto save_data_header_it = request.headers.find("save-data");

    if (request.relative_url == "/favicon.ico") {
      // Favicon request could be received for the previous page load.
      return std::unique_ptr<net::test_server::HttpResponse>();
    }

    if (!expected_save_data_header_.empty()) {
      EXPECT_TRUE(save_data_header_it != request.headers.end())
          << request.relative_url;
      EXPECT_EQ(expected_save_data_header_, save_data_header_it->second)
          << request.relative_url;
    } else {
      EXPECT_TRUE(save_data_header_it == request.headers.end())
          << request.relative_url;
    }
    return std::unique_ptr<net::test_server::HttpResponse>();
  }

  std::unique_ptr<net::EmbeddedTestServer> test_server_;
  std::string expected_save_data_header_;
};

IN_PROC_BROWSER_TEST_F(DataSaverWithServerBrowserTest, ReloadPage) {
  Init();
  ASSERT_TRUE(test_server_->Start());
  EnableDataSaver(true);

  expected_save_data_header_ = "on";
  ui_test_utils::NavigateToURL(browser(),
                               test_server_->GetURL("/google/google.html"));

  // Reload the webpage and expect the main and the subresources will get the
  // correct save-data header.
  expected_save_data_header_ = "on";
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  content::WaitForLoadStop(
      browser()->tab_strip_model()->GetActiveWebContents());

  // Reload the webpage with data saver disabled, and expect all the resources
  // will get no save-data header.
  EnableDataSaver(false);
  expected_save_data_header_ = "";
  chrome::Reload(browser(), WindowOpenDisposition::CURRENT_TAB);
  content::WaitForLoadStop(
      browser()->tab_strip_model()->GetActiveWebContents());
}
