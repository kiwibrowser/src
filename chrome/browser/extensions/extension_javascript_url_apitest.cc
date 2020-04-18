// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "net/dns/mock_host_resolver.h"

class JavscriptApiTest : public extensions::ExtensionApiTest {
 public:
  void SetUpOnMainThread() override {
    extensions::ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(StartEmbeddedTestServer());
  }
};

// If crashing, mark disabled and update http://crbug.com/63589.
IN_PROC_BROWSER_TEST_F(JavscriptApiTest, JavaScriptURLPermissions) {
  ASSERT_TRUE(RunExtensionTest("tabs/javascript_url_permissions")) << message_;
}

IN_PROC_BROWSER_TEST_F(JavscriptApiTest, JavasScriptEncodedURL) {
  ASSERT_TRUE(RunExtensionTest("tabs/javascript_url_encoded")) << message_;
}
