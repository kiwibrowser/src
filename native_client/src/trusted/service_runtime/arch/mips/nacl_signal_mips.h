/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Signal Context
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_NACL_SIGNAL_MIPS_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_NACL_SIGNAL_MIPS_H__ 1

#include "native_client/src/include/portability.h"

/*
 * Architecture specific context object.  Register order matches that
 * found in src/trusted/gdb_rsp/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use a common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 */
struct NaClSignalContext {
  uint32_t zero;
  uint32_t at;
  uint32_t v0;
  uint32_t v1;
  uint32_t a0;
  uint32_t a1;
  uint32_t a2;
  uint32_t a3;
  uint32_t t0;
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;
  uint32_t t4;
  uint32_t t5;
  uint32_t t6;
  uint32_t t7;
  uint32_t s0;
  uint32_t s1;
  uint32_t s2;
  uint32_t s3;
  uint32_t s4;
  uint32_t s5;
  uint32_t s6;
  uint32_t s7;
  uint32_t t8;
  uint32_t t9;
  uint32_t k0;
  uint32_t k1;
  uint32_t global_ptr;
  uint32_t stack_ptr;
  uint32_t frame_ptr;
  uint32_t return_addr;
  uint32_t prog_ctr;
};


#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_MIPS_NACL_SIGNAL_MIPS_H__ */
