/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_interfaces.h"
#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

static int nacl_irt_clock_getres(nacl_irt_clockid_t clk_id,
                                 struct timespec *res) {
  return -NACL_SYSCALL(clock_getres)(clk_id, res);
}

static int nacl_irt_clock_gettime(nacl_irt_clockid_t clk_id,
                                  struct timespec *tp) {
  return -NACL_SYSCALL(clock_gettime)(clk_id, tp);
}

const struct nacl_irt_clock nacl_irt_clock = {
  nacl_irt_clock_getres,
  nacl_irt_clock_gettime,
};
