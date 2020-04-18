// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/browser_watcher/exit_code_watcher_win.h"

#include <stdint.h>

#include <utility>

#include "base/command_line.h"
#include "base/process/process.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_reg_util_win.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/win/scoped_handle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace browser_watcher {

namespace {

const base::char16 kRegistryPath[] = L"Software\\ExitCodeWatcherTest";

MULTIPROCESS_TEST_MAIN(Sleeper) {
  // Sleep forever - the test harness will kill this process to give it an
  // exit code.
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(INFINITE));
  return 1;
}

class ScopedSleeperProcess {
 public:
   ScopedSleeperProcess() : is_killed_(false) {
  }

  ~ScopedSleeperProcess() {
    if (process_.IsValid()) {
      process_.Terminate(-1, false);
      EXPECT_TRUE(process_.WaitForExit(nullptr));
    }
  }

  void Launch() {
    ASSERT_FALSE(process_.IsValid());

    base::CommandLine cmd_line(base::GetMultiProcessTestChildBaseCommandLine());
    base::LaunchOptions options;
    options.start_hidden = true;
    process_ = base::SpawnMultiProcessTestChild("Sleeper", cmd_line, options);
    ASSERT_TRUE(process_.IsValid());
  }

  void Kill(int exit_code, bool wait) {
    ASSERT_TRUE(process_.IsValid());
    ASSERT_FALSE(is_killed_);
    process_.Terminate(exit_code, false);
    int seen_exit_code = 0;
    EXPECT_TRUE(process_.WaitForExit(&seen_exit_code));
    EXPECT_EQ(exit_code, seen_exit_code);
    is_killed_ = true;
  }

  const base::Process& process() const { return process_; }

 private:
  base::Process process_;
  bool is_killed_;
};

class ExitCodeWatcherTest : public testing::Test {
 public:
  typedef testing::Test Super;

  static const int kExitCode = 0xCAFEBABE;

  ExitCodeWatcherTest() : cmd_line_(base::CommandLine::NO_PROGRAM) {}

  void SetUp() override {
    Super::SetUp();

    ASSERT_NO_FATAL_FAILURE(
        override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
  }

  base::Process OpenSelfWithAccess(uint32_t access) {
    return base::Process::OpenWithAccess(base::GetCurrentProcId(), access);
  }

  void VerifyWroteExitCode(base::ProcessId proc_id, int exit_code) {
    base::win::RegistryValueIterator it(
          HKEY_CURRENT_USER, kRegistryPath);

    ASSERT_EQ(1u, it.ValueCount());
    base::win::RegKey key(HKEY_CURRENT_USER,
                          kRegistryPath,
                          KEY_QUERY_VALUE);

    // The value name should encode the process id at the start.
    EXPECT_TRUE(base::StartsWith(
        it.Name(),
        base::StringPrintf(L"%d-", proc_id),
        base::CompareCase::SENSITIVE));
    DWORD value = 0;
    ASSERT_EQ(ERROR_SUCCESS, key.ReadValueDW(it.Name(), &value));
    ASSERT_EQ(exit_code, static_cast<int>(value));
  }

 protected:
  base::CommandLine cmd_line_;
  registry_util::RegistryOverrideManager override_manager_;
};

}  // namespace

TEST_F(ExitCodeWatcherTest, ExitCodeWatcherInvalidHandleFailsInit) {
  ExitCodeWatcher watcher(kRegistryPath);

  // A waitable event has a non process-handle.
  base::Process event(::CreateEvent(nullptr, false, false, nullptr));

  // A non-process handle should fail.
  EXPECT_FALSE(watcher.Initialize(std::move(event)));
}

TEST_F(ExitCodeWatcherTest, ExitCodeWatcherNoAccessHandleFailsInit) {
  ExitCodeWatcher watcher(kRegistryPath);

  // Open a SYNCHRONIZE-only handle to this process.
  base::Process self = OpenSelfWithAccess(SYNCHRONIZE);
  ASSERT_TRUE(self.IsValid());

  // A process handle with insufficient access should fail.
  EXPECT_FALSE(watcher.Initialize(std::move(self)));
}

TEST_F(ExitCodeWatcherTest, ExitCodeWatcherSucceedsInit) {
  ExitCodeWatcher watcher(kRegistryPath);

  // Open a handle to this process with sufficient access for the watcher.
  base::Process self =
      OpenSelfWithAccess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION);
  ASSERT_TRUE(self.IsValid());

  // A process handle with sufficient access should succeed init.
  EXPECT_TRUE(watcher.Initialize(std::move(self)));
}

TEST_F(ExitCodeWatcherTest, ExitCodeWatcherOnExitedProcess) {
  ScopedSleeperProcess sleeper;
  ASSERT_NO_FATAL_FAILURE(sleeper.Launch());

  ExitCodeWatcher watcher(kRegistryPath);

  EXPECT_TRUE(watcher.Initialize(sleeper.process().Duplicate()));

  // Verify that the watcher wrote a sentinel for the process.
  VerifyWroteExitCode(sleeper.process().Pid(), STILL_ACTIVE);

  // Kill the sleeper, and make sure it's exited before we continue.
  ASSERT_NO_FATAL_FAILURE(sleeper.Kill(kExitCode, true));

  watcher.WaitForExit();
  EXPECT_EQ(kExitCode, watcher.exit_code());

  VerifyWroteExitCode(sleeper.process().Pid(), kExitCode);
}

}  // namespace browser_watcher
