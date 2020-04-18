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
                                  const x86_thread_state32_t *src) {
  dest->prog_ctr = src->__eip;
  dest->stack_ptr = src->__esp;

  dest->eax = src->__eax;
  dest->ebx = src->__ebx;
  dest->ecx = src->__ecx;
  dest->edx = src->__edx;
  dest->esi = src->__esi;
  dest->edi = src->__edi;
  dest->ebp = src->__ebp;
  dest->flags = src->__eflags;
  dest->cs = src->__cs;
  dest->ss = src->__ss;
  dest->ds = src->__ds;
  dest->es = src->__es;
  dest->fs = src->__fs;
  dest->gs = src->__gs;
}

static void SignalContextToRegs(x86_thread_state32_t *dest,
                                const struct NaClSignalContext *src) {
  dest->__eip = src->prog_ctr;
  dest->__esp = src->stack_ptr;

  dest->__eax = src->eax;
  dest->__ebx = src->ebx;
  dest->__ecx = src->ecx;
  dest->__edx = src->edx;
  dest->__esi = src->esi;
  dest->__edi = src->edi;
  dest->__ebp = src->ebp;
  dest->__eflags = src->flags;
  dest->__cs = src->cs;
  dest->__ss = src->ss;
  dest->__ds = src->ds;
  dest->__es = src->es;
  dest->__fs = src->fs;
  dest->__gs = src->gs;
}

void NaClSignalContextFromMacThreadState(struct NaClSignalContext *dest,
                                         const x86_thread_state_t *src) {
  SignalContextFromRegs(dest, &src->uts.ts32);
}

void NaClSignalContextToMacThreadState(x86_thread_state_t *dest,
                                       const struct NaClSignalContext *src) {
  SignalContextToRegs(&dest->uts.ts32, src);
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
