// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_switches.h"
#include "base/command_line.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "net/dns/mock_host_resolver.h"

namespace extensions {

class HistoryApiTest : public ExtensionApiTest {
 public:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();

    host_resolver()->AddRule("www.a.com", "127.0.0.1");
    host_resolver()->AddRule("www.b.com", "127.0.0.1");
  }
};

// Full text search indexing sometimes exceeds a timeout. (http://crbug/119505)
IN_PROC_BROWSER_TEST_F(HistoryApiTest, DISABLED_MiscSearch) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "misc_search.html")) << message_;
}

// Same could happen here without the FTS (http://crbug/119505)
IN_PROC_BROWSER_TEST_F(HistoryApiTest, DISABLED_TimedSearch) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "timed_search.html")) << message_;
}

IN_PROC_BROWSER_TEST_F(HistoryApiTest, Delete) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "delete.html")) << message_;
}

IN_PROC_BROWSER_TEST_F(HistoryApiTest, DeleteProhibited) {
  browser()->profile()->GetPrefs()->
      SetBoolean(prefs::kAllowDeletingBrowserHistory, false);
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "delete_prohibited.html")) <<
      message_;
}

IN_PROC_BROWSER_TEST_F(HistoryApiTest, GetVisits) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "get_visits.html")) << message_;
}

IN_PROC_BROWSER_TEST_F(HistoryApiTest, SearchAfterAdd) {
  ASSERT_TRUE(StartEmbeddedTestServer());
  ASSERT_TRUE(RunExtensionSubtest("history", "search_after_add.html"))
      << message_;
}

}  // namespace extensions
