/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_cond_create(int *cond_handle) {
  int rv = NACL_SYSCALL(cond_create)();
  if (rv < 0)
    return -rv;
  *cond_handle = rv;
  return 0;
}

/*
 * Today a cond handle is just an fd and we destroy it with close.
 * But this might not always be so.
 */
static int nacl_irt_cond_destroy(int cond_handle) {
  return -NACL_SYSCALL(close)(cond_handle);
}

static int nacl_irt_cond_signal(int cond_handle) {
  return -NACL_SYSCALL(cond_signal)(cond_handle);
}

static int nacl_irt_cond_broadcast(int cond_handle) {
  return -NACL_SYSCALL(cond_broadcast)(cond_handle);
}

static int nacl_irt_cond_wait(int cond_handle, int mutex_handle) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(cond_wait)(cond_handle,
                                                       mutex_handle));
}

static int nacl_irt_cond_timed_wait_abs(int cond_handle, int mutex_handle,
                                        const struct timespec *abstime) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(cond_timed_wait_abs)(cond_handle,
                                                                 mutex_handle,
                                                                 abstime));
}

const struct nacl_irt_cond nacl_irt_cond = {
  nacl_irt_cond_create,
  nacl_irt_cond_destroy,
  nacl_irt_cond_signal,
  nacl_irt_cond_broadcast,
  nacl_irt_cond_wait,
  nacl_irt_cond_timed_wait_abs,
};
