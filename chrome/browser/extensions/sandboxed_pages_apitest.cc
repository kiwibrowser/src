// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, SandboxedPages) {
  EXPECT_TRUE(RunExtensionSubtest("sandboxed_pages", "main.html")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, SandboxedPagesCSP) {
  ASSERT_TRUE(StartEmbeddedTestServer());

  // This app attempts to load remote web content inside a sandboxed page.
  // Loading web content will fail because of CSP. In addition to that we will
  // show manifest warnings, hence the kFlagIgnoreManifestWarnings.
  EXPECT_TRUE(RunExtensionSubtest("sandboxed_pages_csp", "main.html",
                                  kFlagIgnoreManifestWarnings))
      << message_;
}

}  // namespace extensions
