// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <string>

#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace content {

namespace {

class ResourceSchedulerBrowserTest : public ContentBrowserTest {
 protected:
  ResourceSchedulerBrowserTest() {}
  ~ResourceSchedulerBrowserTest() override {}

  void SetUpInProcessBrowserTestFixture() override {
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ResourceSchedulerBrowserTest);
};

IN_PROC_BROWSER_TEST_F(ResourceSchedulerBrowserTest,
                       ResourceLoadingExperimentIncognito) {
  GURL url(embedded_test_server()->GetURL(
      "/resource_loading/resource_loading_non_mobile.html"));

  Shell* otr_browser = CreateOffTheRecordBrowser();
  EXPECT_TRUE(NavigateToURL(otr_browser, url));
  int data = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(otr_browser, "getResourceNumber()", &data));
  EXPECT_EQ(9, data);
}

IN_PROC_BROWSER_TEST_F(ResourceSchedulerBrowserTest,
                       ResourceLoadingExperimentNormal) {
  GURL url(embedded_test_server()->GetURL(
      "/resource_loading/resource_loading_non_mobile.html"));
  Shell* browser = shell();
  EXPECT_TRUE(NavigateToURL(browser, url));
  int data = -1;
  EXPECT_TRUE(
      ExecuteScriptAndExtractInt(browser, "getResourceNumber()", &data));
  EXPECT_EQ(9, data);
}

}  // anonymous namespace

}  // namespace content
