/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/nacl/nacl_list_mappings.h"

#include <errno.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

int nacl_list_mappings(struct NaClMemMappingInfo *regions, size_t count,
                       size_t *result_count) {
  int error = NACL_SYSCALL(list_mappings)(regions, count);
  if (error < 0) {
    errno = -error;
    return -1;
  }
  *result_count = error;
  return 0;
}
