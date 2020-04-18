/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SWITCH_ALL_REGS_32_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_SWITCH_ALL_REGS_32_H_ 1

#include "native_client/src/include/portability.h"

struct NaClSwitchRemainingRegsState {
  /* These are adjacent because they are restored using 'lss'. */
  /* 0x00 */ uint32_t stack_ptr;
  /* 0x04 */ uint32_t ss;

  /* These are adjacent because they are restored using 'ljmp'. */
  /* 0x08 */ uint32_t spring_addr;
  /* 0x0c */ uint32_t cs;

  /* 0x10 */ uint16_t ds;
  /* 0x12 */ uint16_t es;
  /* 0x14 */ uint16_t fs;
  /* 0x16 */ uint16_t gs;
};

#endif
