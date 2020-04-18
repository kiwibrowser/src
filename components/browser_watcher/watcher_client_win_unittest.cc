// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/watcher_client_win.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <string>

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/process/kill.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_reg_util_win.h"
#include "base/win/scoped_handle.h"
#include "components/browser_watcher/exit_code_watcher_win.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace browser_watcher {

namespace {

// Command line switch used to communiate to the child test.
const char kParentHandle[] = "parent-handle";

bool IsValidParentProcessHandle(base::CommandLine& cmd_line,
                                const char* switch_name) {
  std::string str_handle =
      cmd_line.GetSwitchValueASCII(switch_name);

  size_t integer_handle = 0;
  if (!base::StringToSizeT(str_handle, &integer_handle))
    return false;

  base::ProcessHandle handle =
      reinterpret_cast<base::ProcessHandle>(integer_handle);
  // Verify that we can get the associated process id.
  base::ProcessId parent_id = base::GetProcId(handle);
  if (parent_id == 0) {
    // Unable to get the parent pid - perhaps insufficient permissions.
    return false;
  }

  // Make sure the handle grants SYNCHRONIZE by waiting on it.
  DWORD err = ::WaitForSingleObject(handle, 0);
  if (err != WAIT_OBJECT_0 && err != WAIT_TIMEOUT) {
    // Unable to wait on the handle - perhaps insufficient permissions.
    return false;
  }

  return true;
}

std::string HandleToString(HANDLE handle) {
  // A HANDLE is a void* pointer, which is the same size as a size_t,
  // so we can use reinterpret_cast<> on it.
  size_t integer_handle = reinterpret_cast<size_t>(handle);
  return base::NumberToString(integer_handle);
}

MULTIPROCESS_TEST_MAIN(VerifyParentHandle) {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();

  // Make sure we got a valid parent process handle from the watcher client.
  if (!IsValidParentProcessHandle(*cmd_line, kParentHandle)) {
    LOG(ERROR) << "Invalid or missing parent-handle.";
    return 1;
  }

  return 0;
}

class WatcherClientTest : public base::MultiProcessTest {
 public:
  void SetUp() override {
    // Open an inheritable handle on our own process to test handle leakage.
    self_.Set(::OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                            TRUE,  // Ineritable handle.
                            base::GetCurrentProcId()));

    ASSERT_TRUE(self_.IsValid());
  }

  // Get a base command line to launch back into this test fixture.
  base::CommandLine GetBaseCommandLine(HANDLE parent_handle) {
    base::CommandLine ret = base::GetMultiProcessTestChildBaseCommandLine();

    ret.AppendSwitchASCII(switches::kTestChildProcess, "VerifyParentHandle");
    ret.AppendSwitchASCII(kParentHandle, HandleToString(parent_handle));

    return ret;
  }

  WatcherClient::CommandLineGenerator GetBaseCommandLineGenerator() {
    return base::Bind(&WatcherClientTest::GetBaseCommandLine,
                      base::Unretained(this));
  }

  void AssertSuccessfulExitCode(base::Process process) {
    ASSERT_TRUE(process.IsValid());
    int exit_code = 0;
    if (!process.WaitForExit(&exit_code))
      FAIL() << "Process::WaitForExit failed.";
    ASSERT_EQ(0, exit_code);
  }

  // Inheritable process handle used for testing.
  base::win::ScopedHandle self_;
};

}  // namespace

// TODO(siggi): More testing - test WatcherClient base implementation.

TEST_F(WatcherClientTest, LaunchWatcherSucceeds) {
  WatcherClient client(GetBaseCommandLineGenerator());

  client.LaunchWatcher();

  ASSERT_NO_FATAL_FAILURE(
      AssertSuccessfulExitCode(client.process().Duplicate()));
}

}  // namespace browser_watcher
