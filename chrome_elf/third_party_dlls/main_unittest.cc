// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/third_party_dlls/main.h"

#include <windows.h>

#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/test/test_timeouts.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace third_party_dlls {
namespace {

// NOTE: TestTimeouts::action_max_timeout() is not long enough here.
base::TimeDelta g_timeout = ::IsDebuggerPresent()
                                ? base::TimeDelta::FromMilliseconds(INFINITE)
                                : base::TimeDelta::FromMilliseconds(5000);

//------------------------------------------------------------------------------
// Third-party tests
//------------------------------------------------------------------------------

// These tests spawn a child test process to keep the hooking contained to a
// separate process.  This prevents test clashes in certain testing
// configurations.
TEST(ThirdParty, Main) {
  constexpr wchar_t exe_filename[] = L"third_party_dlls_test_exe.exe";

  // 1. Spawn the test process with NO blacklist.  Expect successful DLL load.
  const wchar_t* sys_dll_test = L"1";

  base::CommandLine cmd_line = base::CommandLine::FromString(exe_filename);
  cmd_line.AppendArgNative(sys_dll_test);

  base::Process proc =
      base::LaunchProcess(cmd_line, base::LaunchOptionsForTest());
  ASSERT_TRUE(proc.IsValid());

  int exit_code = 0;
  if (!proc.WaitForExitWithTimeout(g_timeout, &exit_code)) {
    // Timeout while waiting.  Try to cleanup.
    proc.Terminate(1, false);
    ADD_FAILURE();
    return;
  }
}

}  // namespace
}  // namespace third_party_dlls
