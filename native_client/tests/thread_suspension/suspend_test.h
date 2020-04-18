/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_TESTS_THREAD_SUSPENSION_SUSPEND_TEST_H_
#define NATIVE_CLIENT_TESTS_THREAD_SUSPENSION_SUSPEND_TEST_H_


#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * This is used by both untrusted and trusted code so we use
 * explicitly-sized types for the fields.
 */
struct SuspendTestShm {
  volatile uint32_t var;
  volatile uint32_t should_exit;  /* Boolean */
  uint32_t continue_after_suspension_func;
  struct NaClSignalContext expected_regs;
};


#endif
