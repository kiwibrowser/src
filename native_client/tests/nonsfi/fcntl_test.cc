/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"

void test_getsetfl(const char *test_file) {
  puts("test for F_GETFL and F_SETFL");

  int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  ASSERT_GE(fd, 0);

  int rc = fcntl(fd, F_SETFL, O_NONBLOCK);
  ASSERT_EQ(rc, 0);
  rc = fcntl(fd, F_GETFL);
  ASSERT_GE(rc, 0);
  /*
   * We only test the O_NONBLOCK bit because the user mode qemu does
   * not return O_WRONLY with O_NONBLOCK.
   */
  ASSERT_EQ(rc & O_NONBLOCK, O_NONBLOCK);

  rc = close(fd);
  ASSERT_EQ(rc, 0);
}

void test_getsetfd(const char *test_file) {
  puts("test for F_GETFD and F_SETFD");

  int fd = open(test_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  ASSERT_GE(fd, 0);

  int rc = fcntl(fd, F_GETFD);
  ASSERT_EQ(rc, 0);
  rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
  ASSERT_EQ(rc, 0);
  rc = fcntl(fd, F_GETFD);
  ASSERT_EQ(rc, FD_CLOEXEC);

  rc = close(fd);
  ASSERT_EQ(rc, 0);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Please specify the test file name\n");
    exit(-1);
  }

  const char *test_file = argv[1];
  test_getsetfl(test_file);
  test_getsetfd(test_file);

  puts("PASSED");
  return 0;
}
