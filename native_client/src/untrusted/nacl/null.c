/*
 * Copyright (c) 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Stub routine for wait for newlib support.
 */
#include <errno.h>
#include <sys/types.h>

#include "native_client/src/trusted/service_runtime/include/sys/nacl_syscalls.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

void null_syscall(void) {
  NACL_SYSCALL(null)();
}
