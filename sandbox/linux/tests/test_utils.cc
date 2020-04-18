// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/tests/test_utils.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"

namespace sandbox {

bool TestUtils::CurrentProcessHasChildren() {
  siginfo_t process_info;
  int ret = HANDLE_EINTR(
      waitid(P_ALL, 0, &process_info, WEXITED | WNOHANG | WNOWAIT));
  if (-1 == ret) {
    PCHECK(ECHILD == errno);
    return false;
  } else {
    return true;
  }
}

void TestUtils::HandlePostForkReturn(pid_t pid) {
  const int kChildExitCode = 1;
  if (pid > 0) {
    int status = 0;
    PCHECK(pid == HANDLE_EINTR(waitpid(pid, &status, 0)));
    CHECK(WIFEXITED(status));
    CHECK_EQ(kChildExitCode, WEXITSTATUS(status));
  } else if (pid == 0) {
    _exit(kChildExitCode);
  }
}

}  // namespace sandbox
