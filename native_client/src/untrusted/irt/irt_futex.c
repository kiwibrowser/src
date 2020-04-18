/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"


static int nacl_irt_futex_wait(volatile int *addr, int value,
                               const struct timespec *abstime) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(futex_wait_abs)(addr, value,
                                                            abstime));
}

static int nacl_irt_futex_wake(volatile int *addr, int nwake, int *count) {
  int result = NACL_SYSCALL(futex_wake)(addr, nwake);
  if (result < 0) {
    *count = 0;
    return -result;
  }
  *count = result;
  return 0;
}

const struct nacl_irt_futex nacl_irt_futex = {
  nacl_irt_futex_wait,
  nacl_irt_futex_wake,
};

/*
 * This name is used inside the IRT itself and in libpthread_private,
 * by the private copies of nc_mutex and nc_cond.
 */
extern struct nacl_irt_futex __libnacl_irt_futex
  __attribute__((alias("nacl_irt_futex")));
