/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <fcntl.h>

#include "native_client/src/untrusted/nacl/nacl_random.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_private.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

int nacl_secure_random_init(void) {
  return 0;
}

int nacl_secure_random(void *buf, size_t count, size_t *nread) {
  int rv = NACL_GC_WRAP_SYSCALL(NACL_SYSCALL(get_random_bytes)(buf, count));
  if (rv != 0)
    return -rv;
  *nread = count;
  return 0;
}

const struct nacl_irt_random nacl_irt_random = {
  nacl_secure_random,
};
