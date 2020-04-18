/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"

void ClosePipe(int fds[2]) {
  int rc = close(fds[0]);
  ASSERT_EQ(rc, 0);
  rc = close(fds[1]);
  ASSERT_EQ(rc, 0);
}

int main(int argc, char *argv[]) {
  int fds[2];
  int rc = pipe(fds);
  ASSERT_EQ(rc, 0);
  pid_t pid = fork();
  if (pid == 0) {
    int msg = 0;
    ssize_t num_read = read(fds[0], &msg, sizeof(msg));
    ASSERT_EQ(num_read, sizeof(msg));
    ClosePipe(fds);

    ASSERT_EQ(msg, 42);
    _exit(msg);
    ASSERT(false);
  }

  /* In the parent process. */
  ASSERT_GT(pid, 0);

  int msg = 42;
  ssize_t num_written = write(fds[1], &msg, sizeof(msg));
  ASSERT_EQ(num_written, sizeof(msg));
  ClosePipe(fds);

  int status;
  pid_t pid_waited = waitpid(pid, &status, 0);
  ASSERT_EQ(pid_waited, pid);
  ASSERT(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), msg);
  return 0;
}
