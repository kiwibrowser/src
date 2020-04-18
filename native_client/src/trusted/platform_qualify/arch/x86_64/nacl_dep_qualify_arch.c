/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>

#include "native_client/src/include/build_config.h"

#if !NACL_WINDOWS /* stdint is neither available nor needed on Windows */
#include <stdint.h>
#endif

#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"
#include "native_client/src/include/nacl_macros.h"

/* Assembled equivalent of "ret" */
#define INST_RET 0xC3

int NaClCheckDEP(void) {
  /*
   * We require DEP, so forward this call to the OS-specific check routine.
   */
  return NaClAttemptToExecuteData();
}

nacl_void_thunk NaClGenerateThunk(uint8_t *buf, size_t size_in_bytes) {
  /*
   * Place a "ret" at buf.
   */
  if (size_in_bytes < 1) return 0;

  *buf = (uint8_t) INST_RET;

  /*
   * ISO C prevents a direct data->function cast, because the pointers aren't
   * guaranteed to be the same size.  For our platforms this is fine, but we
   * verify at compile time anyway before tricking the compiler:
   */
  NACL_ASSERT_SAME_SIZE(uint8_t *, nacl_void_thunk);
  return (nacl_void_thunk) (uintptr_t) buf;
}
