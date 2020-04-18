/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <string.h>
#include <sys/ucontext.h>

#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Definition of the POSIX ucontext_t for Mac OS X can be found in:
 * /usr/include/mach/i386/_structs.h
 */

static void SignalContextFromRegs(struct NaClSignalContext *dest,
                                  const x86_thread_state64_t *src) {
  dest->prog_ctr = src->__rip;
  dest->stack_ptr = src->__rsp;

  dest->rax = src->__rax;
  dest->rbx = src->__rbx;
  dest->rcx = src->__rcx;
  dest->rdx = src->__rdx;
  dest->rsi = src->__rsi;
  dest->rdi = src->__rdi;
  dest->rbp = src->__rbp;
  dest->r8  = src->__r8;
  dest->r9  = src->__r9;
  dest->r10 = src->__r10;
  dest->r11 = src->__r11;
  dest->r12 = src->__r12;
  dest->r13 = src->__r13;
  dest->r14 = src->__r14;
  dest->r15 = src->__r15;
  dest->flags = (uint32_t) src->__rflags;
  dest->cs = (uint32_t) src->__cs;
  dest->fs = (uint32_t) src->__fs;
  dest->gs = (uint32_t) src->__gs;
}

static void SignalContextToRegs(x86_thread_state64_t *dest,
                                const struct NaClSignalContext *src) {
  dest->__rip = src->prog_ctr;
  dest->__rsp = src->stack_ptr;

  dest->__rax = src->rax;
  dest->__rbx = src->rbx;
  dest->__rcx = src->rcx;
  dest->__rdx = src->rdx;
  dest->__rsi = src->rsi;
  dest->__rdi = src->rdi;
  dest->__rbp = src->rbp;
  dest->__r8  = src->r8;
  dest->__r9  = src->r9;
  dest->__r10 = src->r10;
  dest->__r11 = src->r11;
  dest->__r12 = src->r12;
  dest->__r13 = src->r13;
  dest->__r14 = src->r14;
  dest->__r15 = src->r15;
  dest->__rflags = src->flags;
  dest->__cs = src->cs;
  dest->__fs = src->fs;
  dest->__gs = src->gs;
}

void NaClSignalContextFromMacThreadState(struct NaClSignalContext *dest,
                                         const x86_thread_state_t *src) {
  SignalContextFromRegs(dest, &src->uts.ts64);
}

void NaClSignalContextToMacThreadState(x86_thread_state_t *dest,
                                       const struct NaClSignalContext *src) {
  SignalContextToRegs(&dest->uts.ts64, src);
}

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sig_ctx,
                                  const void *raw_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  memset(sig_ctx, 0, sizeof(*sig_ctx));
  SignalContextFromRegs(sig_ctx, &uctx->uc_mcontext->__ss);
}

/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  SignalContextToRegs(&uctx->uc_mcontext->__ss, sig_ctx);
}
