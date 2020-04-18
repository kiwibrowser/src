// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_LINUX_SYSTEM_HEADERS_ARM64_LINUX_UCONTEXT_H_
#define SANDBOX_LINUX_SYSTEM_HEADERS_ARM64_LINUX_UCONTEXT_H_

#if !defined(__BIONIC_HAVE_UCONTEXT_T)
#include <asm/sigcontext.h>
#include <signal.h>
#include <stdint.h>
// We also need greg_t for the sandbox, include it in this header as well.
typedef uint64_t greg_t;

struct ucontext_t {
  unsigned long uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  sigset_t uc_sigmask;
  /* glibc uses a 1024-bit sigset_t */
  uint8_t unused[1024 / 8 - sizeof(sigset_t)];
  /* last for future expansion */
  struct sigcontext uc_mcontext;
};

#else
#include <sys/ucontext.h>
#endif  // __BIONIC_HAVE_UCONTEXT_T

#endif  // SANDBOX_LINUX_SYSTEM_HEADERS_ARM64_LINUX_UCONTEXT_H_
