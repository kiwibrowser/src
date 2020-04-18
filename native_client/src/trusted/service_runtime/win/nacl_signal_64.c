/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sig_ctx,
                                  const void *raw_ctx) {
  const CONTEXT *win_ctx = raw_ctx;

  memset(sig_ctx, 0, sizeof(*sig_ctx));

  sig_ctx->prog_ctr = win_ctx->Rip;
  sig_ctx->stack_ptr = win_ctx->Rsp;

  sig_ctx->rax = win_ctx->Rax;
  sig_ctx->rbx = win_ctx->Rbx;
  sig_ctx->rcx = win_ctx->Rcx;
  sig_ctx->rdx = win_ctx->Rdx;
  sig_ctx->rsi = win_ctx->Rsi;
  sig_ctx->rdi = win_ctx->Rdi;
  sig_ctx->rbp = win_ctx->Rbp;
  sig_ctx->r8 = win_ctx->R8;
  sig_ctx->r9 = win_ctx->R9;
  sig_ctx->r10 = win_ctx->R10;
  sig_ctx->r11 = win_ctx->R11;
  sig_ctx->r12 = win_ctx->R12;
  sig_ctx->r13 = win_ctx->R13;
  sig_ctx->r14 = win_ctx->R14;
  sig_ctx->r15 = win_ctx->R15;
  sig_ctx->flags = win_ctx->EFlags;
  sig_ctx->cs = win_ctx->SegCs;
  sig_ctx->ss = win_ctx->SegSs;
  sig_ctx->ds = win_ctx->SegDs;
  sig_ctx->es = win_ctx->SegEs;
  sig_ctx->fs = win_ctx->SegFs;
  sig_ctx->gs = win_ctx->SegGs;
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  CONTEXT *win_ctx = raw_ctx;

  win_ctx->Rip = sig_ctx->prog_ctr;
  win_ctx->Rsp = sig_ctx->stack_ptr;

  win_ctx->Rax = sig_ctx->rax;
  win_ctx->Rbx = sig_ctx->rbx;
  win_ctx->Rcx = sig_ctx->rcx;
  win_ctx->Rdx = sig_ctx->rdx;
  win_ctx->Rsi = sig_ctx->rsi;
  win_ctx->Rdi = sig_ctx->rdi;
  win_ctx->Rbp = sig_ctx->rbp;
  win_ctx->R8 = sig_ctx->r8;
  win_ctx->R9 = sig_ctx->r9;
  win_ctx->R10 = sig_ctx->r10;
  win_ctx->R11 = sig_ctx->r11;
  win_ctx->R12 = sig_ctx->r12;
  win_ctx->R13 = sig_ctx->r13;
  win_ctx->R14 = sig_ctx->r14;
  win_ctx->R15 = sig_ctx->r15;
  win_ctx->EFlags = sig_ctx->flags;
  win_ctx->SegCs = sig_ctx->cs;
  win_ctx->SegSs = sig_ctx->ss;
  win_ctx->SegDs = sig_ctx->ds;
  win_ctx->SegEs = sig_ctx->es;
  win_ctx->SegFs = sig_ctx->fs;
  win_ctx->SegGs = sig_ctx->gs;
}

