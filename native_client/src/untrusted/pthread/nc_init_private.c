/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/nacl_irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"
#include "native_client/src/untrusted/pthread/pthread_internal.h"

static int nacl_irt_thread_create(void (*start_func)(void), void *stack,
                                  void *thread_ptr) {
#if defined(NACL_IN_IRT)
  /*
   * We want the first TLS to point to an unmapped location.  The
   * thread_create() syscall rejects a zero argument for the first
   * TLS, so use a non-zero value in the unmapped first 64k page.
   */
  void *user_tls = (void *) 0x1000;
  return -NACL_SYSCALL(thread_create)((void *) start_func, stack,
                                      user_tls, thread_ptr);
#else
  return -NACL_SYSCALL(thread_create)((void *) start_func, stack,
                                      thread_ptr, 0);
#endif
}

static void nacl_irt_thread_exit(int32_t *stack_flag) {
  NACL_SYSCALL(thread_exit)(stack_flag);
  __builtin_trap();
}

static int nacl_irt_thread_nice(const int nice) {
  return -NACL_SYSCALL(thread_nice)(nice);
}

void __nc_initialize_interfaces(void) {
  const struct nacl_irt_thread init = {
    nacl_irt_thread_create,
    nacl_irt_thread_exit,
    nacl_irt_thread_nice,
  };
  __libnacl_irt_thread = init;
}
