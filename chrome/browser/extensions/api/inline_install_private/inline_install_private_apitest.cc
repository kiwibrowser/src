// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/extensions/api/inline_install_private/inline_install_private_api.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/webstore_installer_test.h"
#include "extensions/test/extension_test_message_listener.h"

namespace extensions {

class InlineInstallPrivateApiTestBase : public WebstoreInstallerTest {
 public:
  InlineInstallPrivateApiTestBase(const std::string& basedir,
                                  const std::string& extension_file) :
      WebstoreInstallerTest("cws.com",
                            basedir,
                            extension_file,
                            "test1.com",
                            "test2.com") {
  }

  void Run(const std::string& testName) {
    AutoAcceptInstall();
    base::FilePath test_driver_path = test_data_dir_.AppendASCII(
        "api_test/inline_install_private/test_driver");

    ExtensionTestMessageListener ready_listener("ready", true);
    ExtensionTestMessageListener success_listener("success", false);
    LoadExtension(test_driver_path);
    EXPECT_TRUE(ready_listener.WaitUntilSatisfied());
    ready_listener.Reply(testName);
    ASSERT_TRUE(success_listener.WaitUntilSatisfied());
  }
};

class InlineInstallPrivateApiTestApp :
      public InlineInstallPrivateApiTestBase {
 public:
  InlineInstallPrivateApiTestApp() :
      InlineInstallPrivateApiTestBase(
          "extensions/api_test/inline_install_private",
          "app.crx") {}
};

IN_PROC_BROWSER_TEST_F(InlineInstallPrivateApiTestApp, SuccessfulInstall) {
  ExtensionFunction::ScopedUserGestureForTests gesture;
  Run("successfulInstall");
}

IN_PROC_BROWSER_TEST_F(InlineInstallPrivateApiTestApp, BackgroundInstall) {
  ExtensionFunction::ScopedUserGestureForTests gesture;
  Run("backgroundInstall");
}

IN_PROC_BROWSER_TEST_F(InlineInstallPrivateApiTestApp, NoGesture) {
  Run("noGesture");
}

class InlineInstallPrivateApiTestExtension :
      public InlineInstallPrivateApiTestBase {
 public:
  InlineInstallPrivateApiTestExtension() :
      InlineInstallPrivateApiTestBase(
          "extensions/api_test/webstore_inline_install",
          "extension.crx") {}
};

IN_PROC_BROWSER_TEST_F(InlineInstallPrivateApiTestExtension, OnlyApps) {
  ExtensionFunction::ScopedUserGestureForTests gesture;
  Run("onlyApps");
}

}  // namespace extensions
