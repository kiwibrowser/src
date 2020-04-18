/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/tests/minnacl/minimal_test_syscalls.h"


#define UNTYPED_SYSCALL(s) ((int (*)()) NACL_SYSCALL_ADDR(s))


static void SimpleAbort(void) {
  while (1) {
    /* Exit by causing a crash. */
    *(volatile int *) 0 = 0;
  }
}

void _start(void) {
  int retval = UNTYPED_SYSCALL(TEST_SYSCALL_INVOKE)();
  if (retval != 123) {
    /* This sandbox is so simple that we have no way of printing a
       failure message. */
    SimpleAbort();
  }
  UNTYPED_SYSCALL(TEST_SYSCALL_EXIT)();
  /* Should not reach here. */
  SimpleAbort();
}
