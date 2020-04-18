/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_dyncode.h"

#include <errno.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

int nacl_dyncode_create(void *dest, const void *src, size_t size) {
  int error = -NACL_SYSCALL(dyncode_create)(dest, src, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

int nacl_dyncode_modify(void *dest, const void *src, size_t size) {
  int error = -NACL_SYSCALL(dyncode_modify)(dest, src, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}

int nacl_dyncode_delete(void *dest, size_t size) {
  int error = -NACL_SYSCALL(dyncode_delete)(dest, size);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
