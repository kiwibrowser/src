// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"
#include "extensions/browser/api/display_source/display_source_apitestbase.h"
#include "extensions/common/switches.h"

namespace extensions {

class ChromeDisplaySourceApiTest :
  public ExtensionApiTest {
 public:
  ChromeDisplaySourceApiTest() = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        extensions::switches::kWhitelistedExtensionID,
          "hajchajepegmmcdjhpfnhmblfhigkcmj");
    command_line->AppendSwitchASCII(::switches::kWindowSize, "300,300");
  }

 private:
  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    InitMockDisplaySourceConnectionDelegate(profile());
    content::RunAllPendingInMessageLoop();
  }
};

IN_PROC_BROWSER_TEST_F(ChromeDisplaySourceApiTest, DisplaySourceExtension) {
  ASSERT_TRUE(RunExtensionSubtest("display_source",
                                  "wifi_display_session.html")) << message_;
}

}  // namespace extensions
