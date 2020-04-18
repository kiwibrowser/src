/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Signal Context
 */

#ifndef __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_NACL_SIGNAL_64_H__
#define __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_NACL_SIGNAL_64_H__ 1

#include "native_client/src/include/portability.h"

/*
 * Architecture specific context object.  Register order matches that
 * found in src/trusted/debug_stub/abi.cc, which allows us to use an
 * abi context (GDB ordered context), and a signal context interchangably.
 * In addition, we use common names for the stack and program counter to
 * allow functions which use them to avoid conditional compilation.
 */
struct NaClSignalContext {
  uint64_t rax;
  uint64_t rbx;
  uint64_t rcx;
  uint64_t rdx;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t stack_ptr;
  uint64_t r8;
  uint64_t r9;
  uint64_t r10;
  uint64_t r11;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t prog_ctr;
  uint32_t flags;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t es;
  uint32_t fs;
  uint32_t gs;
  uint32_t padding;  /* Pad to a multiple of 8 bytes */
};

#endif /* __NATIVE_CLIENT_SERVICE_RUNTIME_ARCH_X86_64_NACL_SIGNAL_64_H__ */
