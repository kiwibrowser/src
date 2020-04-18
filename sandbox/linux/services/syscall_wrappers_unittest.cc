// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/services/syscall_wrappers.h"

#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "build/build_config.h"
#include "sandbox/linux/system_headers/linux_signal.h"
#include "sandbox/linux/tests/test_utils.h"
#include "sandbox/linux/tests/unit_tests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sandbox {

namespace {

TEST(SyscallWrappers, BasicSyscalls) {
  EXPECT_EQ(getpid(), sys_getpid());
}

TEST(SyscallWrappers, CloneBasic) {
  pid_t child = sys_clone(SIGCHLD);
  TestUtils::HandlePostForkReturn(child);
  EXPECT_LT(0, child);
}

TEST(SyscallWrappers, CloneParentSettid) {
  pid_t ptid = 0;
  pid_t child = sys_clone(CLONE_PARENT_SETTID | SIGCHLD, nullptr, &ptid,
                          nullptr, nullptr);
  TestUtils::HandlePostForkReturn(child);
  EXPECT_LT(0, child);
  EXPECT_EQ(child, ptid);
}

TEST(SyscallWrappers, CloneChildSettid) {
  pid_t ctid = 0;
  pid_t pid =
      sys_clone(CLONE_CHILD_SETTID | SIGCHLD, nullptr, nullptr, &ctid, nullptr);

  const int kSuccessExit = 0;
  if (0 == pid) {
    // In child.
    if (sys_getpid() == ctid)
      _exit(kSuccessExit);
    _exit(1);
  }

  ASSERT_NE(-1, pid);
  int status = 0;
  ASSERT_EQ(pid, HANDLE_EINTR(waitpid(pid, &status, 0)));
  ASSERT_TRUE(WIFEXITED(status));
  EXPECT_EQ(kSuccessExit, WEXITSTATUS(status));
}

TEST(SyscallWrappers, GetRESUid) {
  uid_t ruid, euid, suid;
  uid_t sys_ruid, sys_euid, sys_suid;
  ASSERT_EQ(0, getresuid(&ruid, &euid, &suid));
  ASSERT_EQ(0, sys_getresuid(&sys_ruid, &sys_euid, &sys_suid));
  EXPECT_EQ(ruid, sys_ruid);
  EXPECT_EQ(euid, sys_euid);
  EXPECT_EQ(suid, sys_suid);
}

TEST(SyscallWrappers, GetRESGid) {
  gid_t rgid, egid, sgid;
  gid_t sys_rgid, sys_egid, sys_sgid;
  ASSERT_EQ(0, getresgid(&rgid, &egid, &sgid));
  ASSERT_EQ(0, sys_getresgid(&sys_rgid, &sys_egid, &sys_sgid));
  EXPECT_EQ(rgid, sys_rgid);
  EXPECT_EQ(egid, sys_egid);
  EXPECT_EQ(sgid, sys_sgid);
}

TEST(SyscallWrappers, LinuxSigSet) {
  sigset_t sigset;
  ASSERT_EQ(0, sigemptyset(&sigset));
  ASSERT_EQ(0, sigaddset(&sigset, LINUX_SIGSEGV));
  ASSERT_EQ(0, sigaddset(&sigset, LINUX_SIGBUS));
  uint64_t linux_sigset = 0;
  std::memcpy(&linux_sigset, &sigset,
              std::min(sizeof(sigset), sizeof(linux_sigset)));
  EXPECT_EQ((1ULL << (LINUX_SIGSEGV - 1)) | (1ULL << (LINUX_SIGBUS - 1)),
            linux_sigset);
}

}  // namespace

}  // namespace sandbox
