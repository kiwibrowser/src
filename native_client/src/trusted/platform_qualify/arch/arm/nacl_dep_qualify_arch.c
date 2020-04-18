/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <stdint.h>
#include "native_client/src/trusted/platform_qualify/nacl_dep_qualify.h"
#include "native_client/src/include/nacl_macros.h"

/* Assembled equivalent of "bx lr" */
#define INST_BX_LR 0xE12FFF1E

int NaClCheckDEP(void) {
  /*
   * We require DEP, so forward this call to the OS-specific check routine.
   */
  return NaClAttemptToExecuteData();
}

nacl_void_thunk NaClGenerateThunk(uint8_t *buf, size_t size_in_bytes) {
  /*
   * Place a "bx lr" at the next aligned address after buf.  Instructions
   * are always little-endian, regardless of data setting.
   */
  uint8_t *aligned_buf = (uint8_t *) (((uintptr_t) buf + 3) & ~3);

  if (aligned_buf + 4 > buf + size_in_bytes) return 0;

  aligned_buf[0] = (uint8_t) (INST_BX_LR >> 0);
  aligned_buf[1] = (uint8_t) (INST_BX_LR >> 8);
  aligned_buf[2] = (uint8_t) (INST_BX_LR >> 16);
  aligned_buf[3] = (uint8_t) (INST_BX_LR >> 24);

  /*
   * ISO C prevents a direct data->function cast, because the pointers aren't
   * guaranteed to be the same size.  For our platforms this is fine, but we
   * verify at compile time anyway before tricking the compiler:
   */
  NACL_ASSERT_SAME_SIZE(uint8_t *, nacl_void_thunk);
  return (nacl_void_thunk) (uintptr_t) aligned_buf;
}
