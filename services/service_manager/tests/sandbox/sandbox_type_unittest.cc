// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/sandbox/sandbox_type.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "services/service_manager/sandbox/switches.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {

TEST(SandboxTypeTest, Empty) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));

#if defined(OS_WIN)
  EXPECT_FALSE(
      command_line.HasSwitch(switches::kNoSandboxAndElevatedPrivileges));
  SetCommandLineFlagsForSandboxType(
      &command_line, SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES,
            SandboxTypeFromCommandLine(command_line));
#endif

  EXPECT_FALSE(command_line.HasSwitch(switches::kNoSandbox));
  SetCommandLineFlagsForSandboxType(&command_line, SANDBOX_TYPE_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
  EXPECT_TRUE(command_line.HasSwitch(switches::kNoSandbox));
}

TEST(SandboxTypeTest, Renderer) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType,
                                 switches::kRendererProcess);
  EXPECT_EQ(SANDBOX_TYPE_RENDERER, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_RENDERER, SandboxTypeFromCommandLine(command_line));

  EXPECT_FALSE(command_line.HasSwitch(switches::kNoSandbox));
  SetCommandLineFlagsForSandboxType(&command_line, SANDBOX_TYPE_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
  EXPECT_TRUE(command_line.HasSwitch(switches::kNoSandbox));
}

TEST(SandboxTypeTest, Utility) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType,
                                 switches::kUtilityProcess);
  EXPECT_EQ(SANDBOX_TYPE_UTILITY, SandboxTypeFromCommandLine(command_line));

  base::CommandLine command_line2(command_line);
  SetCommandLineFlagsForSandboxType(&command_line2, SANDBOX_TYPE_NETWORK);
  EXPECT_EQ(SANDBOX_TYPE_NETWORK, SandboxTypeFromCommandLine(command_line2));

  base::CommandLine command_line3(command_line);
  SetCommandLineFlagsForSandboxType(&command_line3, SANDBOX_TYPE_CDM);
  EXPECT_EQ(SANDBOX_TYPE_CDM, SandboxTypeFromCommandLine(command_line3));

  base::CommandLine command_line4(command_line);
  SetCommandLineFlagsForSandboxType(&command_line4, SANDBOX_TYPE_NO_SANDBOX);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line4));

  base::CommandLine command_line5(command_line);
  SetCommandLineFlagsForSandboxType(&command_line5, SANDBOX_TYPE_PPAPI);
  EXPECT_EQ(SANDBOX_TYPE_PPAPI, SandboxTypeFromCommandLine(command_line5));

  base::CommandLine command_line6(command_line);
  command_line6.AppendSwitchASCII(switches::kServiceSandboxType, "bogus");
  EXPECT_EQ(SANDBOX_TYPE_UTILITY, SandboxTypeFromCommandLine(command_line6));

  base::CommandLine command_line7(command_line);
  SetCommandLineFlagsForSandboxType(&command_line7,
                                    SANDBOX_TYPE_PDF_COMPOSITOR);
  EXPECT_EQ(SANDBOX_TYPE_PDF_COMPOSITOR,
            SandboxTypeFromCommandLine(command_line7));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
}

TEST(SandboxTypeTest, GPU) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType, switches::kGpuProcess);
  SetCommandLineFlagsForSandboxType(&command_line, SANDBOX_TYPE_GPU);
  EXPECT_EQ(SANDBOX_TYPE_GPU, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_GPU, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
}

TEST(SandboxTypeTest, PPAPIBroker) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType,
                                 switches::kPpapiBrokerProcess);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
}

TEST(SandboxTypeTest, PPAPIPlugin) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType,
                                 switches::kPpapiPluginProcess);
  SetCommandLineFlagsForSandboxType(&command_line, SANDBOX_TYPE_PPAPI);
  EXPECT_EQ(SANDBOX_TYPE_PPAPI, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_PPAPI, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
}

TEST(SandboxTypeTest, Nonesuch) {
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  command_line.AppendSwitchASCII(switches::kProcessType, "nonesuch");
  EXPECT_EQ(SANDBOX_TYPE_INVALID, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitchASCII(switches::kServiceSandboxType, "network");
  EXPECT_EQ(SANDBOX_TYPE_INVALID, SandboxTypeFromCommandLine(command_line));

  command_line.AppendSwitch(switches::kNoSandbox);
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, SandboxTypeFromCommandLine(command_line));
}

TEST(SandboxTypeTest, ElevatedPrivileges) {
  // Tests that the "no sandbox and elevated privileges" which is Windows
  // specific default to no sandbox on non Windows platforms.
  SandboxType elevated_type =
      UtilitySandboxTypeFromString(switches::kNoneSandboxAndElevatedPrivileges);
#if defined(OS_WIN)
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX_AND_ELEVATED_PRIVILEGES, elevated_type);
#else
  EXPECT_EQ(SANDBOX_TYPE_NO_SANDBOX, elevated_type);
#endif
}

}  // namespace service_manager
