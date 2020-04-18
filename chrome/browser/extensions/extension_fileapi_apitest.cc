// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_FileAPI) {
  ASSERT_TRUE(RunExtensionTest("fileapi")) << message_;
}

// crbug.com/671144
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_XHROnPersistentFileSystem) {
  ASSERT_TRUE(RunPlatformAppTest("xhr_persistent_fs")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, RequestQuotaInBackgroundPage) {
  ASSERT_TRUE(RunExtensionTest("request_quota_background")) << message_;
}

}  // namespace extensions
