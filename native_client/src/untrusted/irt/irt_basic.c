/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static void nacl_irt_exit(int status) {
  NACL_SYSCALL(exit)(status);
  __builtin_trap();
}

static int nacl_irt_gettod(struct timeval *tv) {
  return -NACL_SYSCALL(gettimeofday)(tv);
}

static int nacl_irt_clock(nacl_irt_clock_t *ticks) {
  *ticks = NACL_SYSCALL(clock)();  /* Cannot fail.  */
  return 0;
}

static int nacl_irt_nanosleep(const struct timespec *req,
                              struct timespec *rem) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(nanosleep)(req, rem));
}

static int nacl_irt_sched_yield(void) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(sched_yield)());
}

static int nacl_irt_sysconf(int name, int *value) {
  return -NACL_SYSCALL(sysconf)(name, value);
}

const struct nacl_irt_basic nacl_irt_basic = {
  nacl_irt_exit,
  nacl_irt_gettod,
  nacl_irt_clock,
  nacl_irt_nanosleep,
  nacl_irt_sched_yield,
  nacl_irt_sysconf,
};
