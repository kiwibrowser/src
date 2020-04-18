/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_sem_create(int *sem_handle, int32_t value) {
  int rv = NACL_SYSCALL(sem_create)(value);
  if (rv < 0)
    return -rv;
  *sem_handle = rv;
  return 0;
}

/*
 * Today a semaphore handle is just an fd and we destroy it with close.
 * But this might not always be so.
 */
static int nacl_irt_sem_destroy(int sem_handle) {
  return -NACL_SYSCALL(close)(sem_handle);
}

static int nacl_irt_sem_post(int sem_handle) {
  return -NACL_SYSCALL(sem_post)(sem_handle);
}

static int nacl_irt_sem_wait(int sem_handle) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(sem_wait)(sem_handle));
}

const struct nacl_irt_sem nacl_irt_sem = {
  nacl_irt_sem_create,
  nacl_irt_sem_destroy,
  nacl_irt_sem_post,
  nacl_irt_sem_wait,
};
