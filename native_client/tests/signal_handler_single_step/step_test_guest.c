/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/tests/signal_handler_single_step/step_test_syscalls.h"


#define UNTYPED_SYSCALL(s) ((int (*)()) NACL_SYSCALL_ADDR(s))


void _start(void) {
  while (1) {
    UNTYPED_SYSCALL(SINGLE_STEP_TEST_SYSCALL)();
  }
}
