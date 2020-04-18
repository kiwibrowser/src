/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifdef __GLIBC__
#include <sys/utsname.h>
#else
#include "native_client/src/untrusted/nacl/include/sys/utsname.h"
#endif

#include "native_client/src/include/nacl_assert.h"

int main(void) {
  struct utsname buf;
  int rv = uname(&buf);
#if defined(__GLIBC__) && __GLIBC__ == 2 && __GLIBC_MINOR__ == 9
  /*
   * The old glibc currently return ENOSYS.
   * TODO(sbc): Fix uname() implementation in glibc.
   */
  if (rv == 0) {
    printf("uname succeeded unexpectedly\n");
    return 1;
  }
  if (errno != ENOSYS) {
    printf("uname returned unexpected error: %d %s\n", errno, strerror(errno));
    return 1;
  }
#else
  ASSERT_EQ(rv, 0);
  printf("sysname: %s\n", buf.sysname);
  printf("nodename: %s\n", buf.nodename);
  printf("release: %s\n", buf.release);
  printf("version: %s\n", buf.version);
  printf("machine: %s\n", buf.machine);
  ASSERT_EQ(strcmp(buf.sysname, "NaCl"), 0);
#endif
  return 0;
}
