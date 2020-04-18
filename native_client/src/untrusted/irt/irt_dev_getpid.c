/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/untrusted/irt/irt_dev.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_dev_getpid_func(int *pid) {
  int rv = NACL_SYSCALL(getpid)();
  if (rv < 0)
    return -rv;
  *pid = rv;
  return 0;
}

const struct nacl_irt_dev_getpid nacl_irt_dev_getpid = {
  nacl_irt_dev_getpid_func,
};
