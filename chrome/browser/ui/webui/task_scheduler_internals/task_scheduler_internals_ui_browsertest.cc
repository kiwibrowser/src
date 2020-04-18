// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/base/web_ui_browser_test.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "ui/base/window_open_disposition.h"

class TaskSchedulerInternalsWebUIBrowserTest : public WebUIBrowserTest {
 public:
  TaskSchedulerInternalsWebUIBrowserTest() = default;
  ~TaskSchedulerInternalsWebUIBrowserTest() override = default;

  void SetUpOnMainThread() override {
    WebUIBrowserTest::SetUpOnMainThread();
    std::string url_string(content::kChromeUIScheme);
    url_string += "://";
    url_string += chrome::kChromeUITaskSchedulerInternalsHost;
    ui_test_utils::NavigateToURLWithDisposition(
        browser(),
        GURL(url_string),
        WindowOpenDisposition::CURRENT_TAB,
        ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TaskSchedulerInternalsWebUIBrowserTest);
};

// Test that navigation to the internals page works.
IN_PROC_BROWSER_TEST_F(TaskSchedulerInternalsWebUIBrowserTest, Navigate) {
  EXPECT_EQ(base::ASCIIToUTF16("Task Scheduler Internals"),
            browser()->tab_strip_model()->GetActiveWebContents()->GetTitle());
}
