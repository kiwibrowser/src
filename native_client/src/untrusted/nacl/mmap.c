/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "native_client/src/untrusted/nacl/nacl_irt.h"

void *mmap(void *start, size_t length, int prot, int flags,
           int desc, off_t offset) {
  int error = __libnacl_irt_memory.mmap(&start, length, prot, flags,
                                        desc, offset);
  if (error) {
    errno = error;
    return MAP_FAILED;
  }
  return start;
}
