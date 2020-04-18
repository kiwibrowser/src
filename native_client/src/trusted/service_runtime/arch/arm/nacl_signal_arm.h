/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Signal Context
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_NACL_SIGNAL_ARM_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_NACL_SIGNAL_ARM_H__ 1

#include "native_client/src/include/portability.h"

/*
 * Architecture specific context object.  Register order matches that
 * found in src/trusted/debug_stub/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 */
struct NaClSignalContext {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r4;
  uint32_t r5;
  uint32_t r6;
  uint32_t r7;
  uint32_t r8;
  uint32_t r9;
  uint32_t r10;
  uint32_t r11;
  uint32_t r12;
  uint32_t stack_ptr;
  uint32_t lr;
  uint32_t prog_ctr;
  uint32_t cpsr;
};


#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_ARM_NACL_SIGNAL_ARM_H__ */
