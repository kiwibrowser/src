/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

void thread_func(void) {
  NACL_SYSCALL(thread_exit)(NULL);
  /* Run forever just in case thread_exit fails and returns. */
  while (1) { }
}

int main(void) {
  char *func_addr = (char *) (uintptr_t) thread_func;
  char *bad_func_addr;
  int stack_size = 0x10000;
  char *stack;
  char *stack_top;
  char *tls;
  int rc;

  stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
               MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  assert(stack != MAP_FAILED);
  stack_top = stack + stack_size;

  /* We do not care about TLS for this test, but sel_ldr rejects a
     zero tls argument, so use an arbitrary non-zero value. */
  tls = (char *) 0x1000;

  /* First do a sanity check to ensure that we can successfully call
     the thread_create syscall directly. */
  rc = NACL_SYSCALL(thread_create)(func_addr, stack_top, tls, 0);
  assert(rc == 0);

  rc = NACL_SYSCALL(thread_create)(func_addr + 1, stack_top, tls, 0);
  assert(rc == -EFAULT);

  /* Addresses above 1GB are outside of our address space. */
  bad_func_addr = (char *) 0x40000000;
  rc = NACL_SYSCALL(thread_create)(bad_func_addr, stack_top, tls, 0);
  assert(rc == -EFAULT);

  return 0;
}
