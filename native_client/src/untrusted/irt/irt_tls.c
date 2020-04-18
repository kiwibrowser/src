/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>

#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_tls_init(void *thread_ptr) {
  return -NACL_SYSCALL(tls_init)(thread_ptr);
}

const struct nacl_irt_tls nacl_irt_tls = {
  nacl_irt_tls_init,
  NACL_SYSCALL(tls_get),
};
