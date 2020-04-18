/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/public/linux_syscalls/sys/resource.h"

void test_rlimit_nofile() {
  puts("test_rlimit_nofile");

  // Get the current rlimit.
  // Set rlim_cur > rlim_max, here, so that we can make sure that the
  // data is properly set.
  struct rlimit old_rlimit = {1, 0};
  int rc = getrlimit(RLIMIT_NOFILE, &old_rlimit);
  ASSERT_EQ(0, rc);
  // Make sure old_rlimit is set properly, by rlim_cur <= rlim_max is ensured.
  ASSERT_LE(old_rlimit.rlim_cur, old_rlimit.rlim_max);

  // Set the limit to prohibit opening a new file.
  struct rlimit new_rlimit = {0, old_rlimit.rlim_max};
  rc = setrlimit(RLIMIT_NOFILE, &new_rlimit);
  ASSERT_EQ(0, rc);

  struct rlimit obtained_rlimit;
  rc = getrlimit(RLIMIT_NOFILE, &obtained_rlimit);
  ASSERT_EQ(0, rc);
  ASSERT_EQ(new_rlimit.rlim_cur, obtained_rlimit.rlim_cur);
  ASSERT_EQ(new_rlimit.rlim_max, obtained_rlimit.rlim_max);

  // Then dup() should fail with EMFILE.
  errno = 0;
  rc = dup(STDOUT_FILENO);
  ASSERT_EQ(-1, rc);
  ASSERT_EQ(EMFILE, errno);

  // Tear down.
  rc = setrlimit(RLIMIT_NOFILE, &old_rlimit);
  ASSERT_EQ(0, rc);
}

void test_rlimit_as() {
  puts("test_rlimit_as");

  // Get the current rlimit.
  // Set rlim_cur > rlim_max, here, so that we can make sure that the
  // data is properly set.
  struct rlimit old_rlimit = {1, 0};
  int rc = getrlimit(RLIMIT_AS, &old_rlimit);
  ASSERT_EQ(0, rc);
  // Make sure old_rlimit is set properly, by rlim_cur <= rlim_max is ensured.
  ASSERT_LE(old_rlimit.rlim_cur, old_rlimit.rlim_max);

  // Set the limit to prohibit allocating new memory.
  struct rlimit new_rlimit = {0, old_rlimit.rlim_max};
  rc = setrlimit(RLIMIT_AS, &new_rlimit);
  ASSERT_EQ(0, rc);

  struct rlimit obtained_rlimit;
  rc = getrlimit(RLIMIT_AS, &obtained_rlimit);
  ASSERT_EQ(0, rc);
  ASSERT_EQ(new_rlimit.rlim_cur, obtained_rlimit.rlim_cur);
  ASSERT_EQ(new_rlimit.rlim_max, obtained_rlimit.rlim_max);

  // Then mmap() should fail with ENOMEM.
  errno = 0;
  void *memory = mmap(0, 4192, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  ASSERT_EQ(MAP_FAILED, memory);
  ASSERT_EQ(ENOMEM, errno);

  // Tear down.
  rc = setrlimit(RLIMIT_AS, &old_rlimit);
  ASSERT_EQ(0, rc);
}

int main(int argc, char *argv[]) {
  test_rlimit_nofile();
  test_rlimit_as();
  puts("PASSED");
  return 0;
}
