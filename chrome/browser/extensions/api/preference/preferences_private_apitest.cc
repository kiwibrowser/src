// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "extensions/common/switches.h"

namespace extensions {

class PreferencesPrivateApiTest : public ExtensionApiTest {
 public:
  PreferencesPrivateApiTest() {}
  ~PreferencesPrivateApiTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(switches::kWhitelistedExtensionID,
                                    "cpfhkdbjfdgdebcjlifoldbijinjfifp");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PreferencesPrivateApiTest);
};

IN_PROC_BROWSER_TEST_F(PreferencesPrivateApiTest, TestEasyUnlockEvent) {
  ASSERT_TRUE(RunExtensionTest("preferences_private")) << message_;
}

}  // namespace extensions
