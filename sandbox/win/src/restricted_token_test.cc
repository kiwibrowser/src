// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Integration tests for restricted tokens.

#include <stddef.h>
#include <string>

#include "base/strings/stringprintf.h"
#include "base/win/scoped_handle.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/target_services.h"
#include "sandbox/win/tests/common/controller.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

namespace {

int RunOpenProcessTest(bool unsandboxed,
                       bool lockdown_dacl,
                       DWORD access_mask) {
  TestRunner runner(JOB_NONE, USER_RESTRICTED_SAME_ACCESS, USER_LOCKDOWN);
  runner.GetPolicy()->SetDelayedIntegrityLevel(INTEGRITY_LEVEL_UNTRUSTED);
  runner.GetPolicy()->SetIntegrityLevel(INTEGRITY_LEVEL_LOW);
  if (lockdown_dacl)
    runner.GetPolicy()->SetLockdownDefaultDacl();
  runner.SetAsynchronous(true);
  // This spins up a renderer level process, we don't care about the result.
  runner.RunTest(L"IntegrationTestsTest_args 1");

  TestRunner runner2(JOB_NONE, USER_RESTRICTED_SAME_ACCESS, USER_LIMITED);
  runner2.GetPolicy()->SetDelayedIntegrityLevel(INTEGRITY_LEVEL_LOW);
  runner2.GetPolicy()->SetIntegrityLevel(INTEGRITY_LEVEL_LOW);
  runner2.SetUnsandboxed(unsandboxed);
  return runner2.RunTest(
      base::StringPrintf(L"RestrictedTokenTest_openprocess %d 0x%08X",
                         runner.process_id(), access_mask)
          .c_str());
}

}  // namespace

// Opens a process based on a PID and access mask passed on the command line.
// Returns SBOX_TEST_SUCCEEDED if process opened successfully.
SBOX_TESTS_COMMAND int RestrictedTokenTest_openprocess(int argc,
                                                       wchar_t** argv) {
  if (argc < 2)
    return SBOX_TEST_NOT_FOUND;
  DWORD pid = _wtoi(argv[0]);
  if (pid == 0)
    return SBOX_TEST_NOT_FOUND;
  DWORD desired_access = wcstoul(argv[1], nullptr, 0);
  base::win::ScopedHandle process_handle(
      ::OpenProcess(desired_access, false, pid));
  if (process_handle.IsValid())
    return SBOX_TEST_SUCCEEDED;

  return SBOX_TEST_DENIED;
}

TEST(RestrictedTokenTest, OpenLowPrivilegedProcess) {
  // Test limited privilege to renderer open.
  ASSERT_EQ(SBOX_TEST_SUCCEEDED,
            RunOpenProcessTest(false, false, GENERIC_READ | GENERIC_WRITE));
  // Test limited privilege to renderer open with lockdowned DACL.
  ASSERT_EQ(SBOX_TEST_DENIED,
            RunOpenProcessTest(false, true, GENERIC_READ | GENERIC_WRITE));
  // Ensure we also can't get any access to the process.
  ASSERT_EQ(SBOX_TEST_DENIED, RunOpenProcessTest(false, true, MAXIMUM_ALLOWED));
  // Also check for explicit owner allowed WRITE_DAC right.
  ASSERT_EQ(SBOX_TEST_DENIED, RunOpenProcessTest(false, true, WRITE_DAC));
  // Ensure unsandboxed process can still open the renderer for all access.
  ASSERT_EQ(SBOX_TEST_SUCCEEDED,
            RunOpenProcessTest(true, true, PROCESS_ALL_ACCESS));
}

}  // namespace sandbox
