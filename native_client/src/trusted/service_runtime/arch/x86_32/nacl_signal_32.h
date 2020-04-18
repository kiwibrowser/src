/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Signal Context
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_NACL_SIGNAL_32_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_32_NACL_SIGNAL_32_H__ 1

#include "native_client/src/include/portability.h"

/*
 * Architecture specific context object.  Register order matches that
 * found in src/trusted/debug_stub/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 */
struct NaClSignalContext {
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t stack_ptr;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t prog_ctr;
  uint32_t flags;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t es;
  uint32_t fs;
  uint32_t gs;
};

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_NACL_SIGNAL_64_H__ */
