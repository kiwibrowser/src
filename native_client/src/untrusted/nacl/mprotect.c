/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

int mprotect(void *start, size_t length, int prot) {
  int error = __libnacl_irt_memory.mprotect(start, length, prot);
  if (error) {
    errno = error;
    return -1;
  }
  return 0;
}
