/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_mutex_create(int *mutex_handle) {
  int rv = NACL_SYSCALL(mutex_create)();
  if (rv < 0)
    return -rv;
  *mutex_handle = rv;
  return 0;
}

/*
 * Today a mutex handle is just an fd and we destroy it with close.
 * But this might not always be so.
 */
static int nacl_irt_mutex_destroy(int mutex_handle) {
  return -NACL_SYSCALL(close)(mutex_handle);
}

static int nacl_irt_mutex_lock(int mutex_handle) {
  return NACL_GC_WRAP_SYSCALL(-NACL_SYSCALL(mutex_lock)(mutex_handle));
}

static int nacl_irt_mutex_unlock(int mutex_handle) {
  return -NACL_SYSCALL(mutex_unlock)(mutex_handle);
}

static int nacl_irt_mutex_trylock(int mutex_handle) {
  return -NACL_SYSCALL(mutex_trylock)(mutex_handle);
}

const struct nacl_irt_mutex nacl_irt_mutex = {
  nacl_irt_mutex_create,
  nacl_irt_mutex_destroy,
  nacl_irt_mutex_lock,
  nacl_irt_mutex_unlock,
  nacl_irt_mutex_trylock,
};
