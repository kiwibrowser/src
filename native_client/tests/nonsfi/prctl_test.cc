/*
 * Copyright 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <string.h>

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/public/linux_syscalls/sys/prctl.h"

void TestPRGetName() {
  puts("TestPRGetName()");

  char buf[17];
  int rc = prctl(PR_GET_NAME, (uintptr_t) buf, 0, 0, 0);
  ASSERT_EQ(rc, 0);

  puts("PASSED");
}

void TestPRDumpable() {
  puts("TestPRDumpable()");

  int rc = prctl(PR_SET_DUMPABLE, 0);
  ASSERT_EQ(rc, 0);
  rc = prctl(PR_GET_DUMPABLE);
  ASSERT_EQ(rc, 0);
  rc = prctl(PR_SET_DUMPABLE, 1);
  ASSERT_EQ(rc, 0);
  rc = prctl(PR_GET_DUMPABLE);
  ASSERT_EQ(rc, 1);

  puts("PASSED");
}

int main(int argc, char *argv[]) {
  TestPRGetName();
  TestPRDumpable();
  return 0;
}
