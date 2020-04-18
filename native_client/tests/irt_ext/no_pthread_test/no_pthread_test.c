/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* This simple test tests that supplying thread functions without
 * pthreads does nothing.
 */

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/untrusted/irt/irt.h"
#include "native_client/src/untrusted/irt/irt_extension.h"
#include "native_client/tests/irt_ext/error_report.h"

int main(void) {
  struct nacl_irt_thread thread_calls = { NULL };
  size_t bytes = nacl_interface_ext_supply(NACL_IRT_THREAD_v0_1,
                                           &thread_calls, sizeof(thread_calls));
  ASSERT_MSG(0 == bytes, "Expected to unsuccessfully supply interface");
  return 0;
}
