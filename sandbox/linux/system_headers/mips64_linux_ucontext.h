// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSTEM_HEADERS_MIPS64_LINUX_UCONTEXT_H_
#define SANDBOX_LINUX_SYSTEM_HEADERS_MIPS64_LINUX_UCONTEXT_H_

#include <stdint.h>

// This is mostly copied from breakpad (common/android/include/sys/ucontext.h),
// except we do use sigset_t for uc_sigmask instead of a custom type.
#if !defined(__BIONIC_HAVE_UCONTEXT_T)
// Ensure that 'stack_t' is defined.
#include <asm/signal.h>

// We also need greg_t for the sandbox, include it in this header as well.
typedef unsigned long greg_t;

typedef struct {
  uint64_t gregs[32];
  uint64_t fpregs[32];
  uint64_t mdhi;
  uint64_t hi1;
  uint64_t hi2;
  uint64_t hi3;
  uint64_t mdlo;
  uint64_t lo1;
  uint64_t lo2;
  uint64_t lo3;
  uint64_t pc;
  uint32_t fpc_csr;
  uint32_t used_math;
  uint32_t dsp;
  uint32_t reserved;
} mcontext_t;

typedef struct ucontext {
  uint32_t uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
  // Other fields are not used by Google Breakpad. Don't define them.
} ucontext_t;

#else
#include <sys/ucontext.h>
#endif  // __BIONIC_HAVE_UCONTEXT_T

#endif  // SANDBOX_LINUX_SYSTEM_HEADERS_MIPS64_LINUX_UCONTEXT_H_
