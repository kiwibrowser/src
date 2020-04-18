// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/display_source/display_source_apitestbase.h"
#include "extensions/shell/test/shell_apitest.h"

namespace extensions {

class DisplaySourceApiTest : public ShellApiTest {
  public:
   DisplaySourceApiTest() = default;

  private:
   void SetUpOnMainThread() override {
     ShellApiTest::SetUpOnMainThread();
     InitMockDisplaySourceConnectionDelegate(browser_context());
     content::RunAllPendingInMessageLoop();
    }
};

IN_PROC_BROWSER_TEST_F(DisplaySourceApiTest, DisplaySourceExtension) {
  ASSERT_TRUE(RunAppTest("api_test/display_source/api")) << message_;
}

}  // namespace extensions
