/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

void *sysbrk(void *new_break) {
  return NACL_SYSCALL(brk)(new_break);
}
