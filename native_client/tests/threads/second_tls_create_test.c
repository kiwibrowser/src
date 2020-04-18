/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static char thread_stack[8192] __attribute__((aligned(32)));

static char thread_tls[32] __attribute__((aligned(32)));

static void fail(const char *msg) {
  NACL_SYSCALL(write)(2, msg, strlen(msg));
  NACL_SYSCALL(exit)(1);
}

static void test_thread(void) {
  if (NACL_SYSCALL(tls_get)() != thread_tls)
    fail("first TLS wrong!\n");
  else if (NACL_SYSCALL(second_tls_get)() != &thread_tls[2])
    fail("second TLS wrong!\n");
  else
    NACL_SYSCALL(exit)(0);
}

int main(void) {
  int error = -NACL_SYSCALL(thread_create)((void *) (uintptr_t) &test_thread,
                                           &thread_stack[sizeof thread_stack],
                                           thread_tls,
                                           &thread_tls[2]);

  if (error) {
    errno = -error;
    perror("thread_create");
    NACL_SYSCALL(exit)(-1);
  }

  /*
   * Wait for the new thread to spin up and decide our fate.
   */
  while (1)
    NACL_SYSCALL(sched_yield)();

  /*NOTREACHED*/
  NACL_SYSCALL(exit)(-1);
}
