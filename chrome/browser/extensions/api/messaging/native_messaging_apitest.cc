// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_messaging_test_util.h"
#include "chrome/browser/extensions/extension_apitest.h"

namespace extensions {

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, NativeMessagingBasic) {
  extensions::ScopedTestNativeMessagingHost test_host;
  ASSERT_NO_FATAL_FAILURE(test_host.RegisterTestHost(false));
  ASSERT_TRUE(RunExtensionTest("native_messaging")) << message_;
}

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, UserLevelNativeMessaging) {
  extensions::ScopedTestNativeMessagingHost test_host;
  ASSERT_NO_FATAL_FAILURE(test_host.RegisterTestHost(true));
  ASSERT_TRUE(RunExtensionTest("native_messaging")) << message_;
}

}  // namespace extensions
