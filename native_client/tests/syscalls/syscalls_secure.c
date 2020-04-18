/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test is run without the sel_ldr -a flag in order to test the
 * default behaviour of certain syscalls.
 */

#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

/* Test that fstat masks the inode number when running without -a */
int test_fstat(void) {
  struct stat buf;
  int rc = fstat(1, &buf);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(buf.st_ino, NACL_FAKE_INODE_NUM);
  return 0;
}

int main(void) {
  int rtn = test_fstat();
  return rtn;
}
