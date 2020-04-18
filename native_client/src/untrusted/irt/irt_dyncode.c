/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_dyncode_create(void *dest, const void *src, size_t size) {
  return -NACL_SYSCALL(dyncode_create)(dest, src, size);
}

static int nacl_irt_dyncode_modify(void *dest, const void *src, size_t size) {
  return -NACL_SYSCALL(dyncode_modify)(dest, src, size);
}

static int nacl_irt_dyncode_delete(void *dest, size_t size) {
  return -NACL_SYSCALL(dyncode_delete)(dest, size);
}

const struct nacl_irt_dyncode nacl_irt_dyncode = {
  nacl_irt_dyncode_create,
  nacl_irt_dyncode_modify,
  nacl_irt_dyncode_delete,
};
