// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "extensions/common/switches.h"

class ExtensionTerminalPrivateApiTest : public extensions::ExtensionApiTest {
  void SetUpCommandLine(base::CommandLine* command_line) override {
    extensions::ExtensionApiTest::SetUpCommandLine(command_line);
    command_line->AppendSwitchASCII(
        extensions::switches::kWhitelistedExtensionID,
        "kidcpjlbjdmcnmccjhjdckhbngnhnepk");
  }
};

IN_PROC_BROWSER_TEST_F(ExtensionTerminalPrivateApiTest, TerminalTest) {
  EXPECT_TRUE(RunExtensionSubtest("terminal/component_extension", "test.html"))
      << message_;
};
