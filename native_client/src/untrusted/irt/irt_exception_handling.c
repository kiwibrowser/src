/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_exception_handler(NaClExceptionHandler handler,
                                      NaClExceptionHandler *old_handler) {
  return -NACL_SYSCALL(exception_handler)(handler, old_handler);
}

static int nacl_irt_exception_stack(void *stack, size_t size) {
  return -NACL_SYSCALL(exception_stack)(stack, size);
}

static int nacl_irt_exception_clear_flag(void) {
  return -NACL_SYSCALL(exception_clear_flag)();
}

const struct nacl_irt_exception_handling nacl_irt_exception_handling = {
  nacl_irt_exception_handler,
  nacl_irt_exception_stack,
  nacl_irt_exception_clear_flag,
};
