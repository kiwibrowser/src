/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <string.h>
#include <sys/ucontext.h>

#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Definition of the POSIX ucontext_t for Linux can be found in:
 * /usr/include/sys/ucontext.h
 */

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sig_ctx,
                                  const void *raw_ctx) {
  const ucontext_t *uctx = (const ucontext_t *) raw_ctx;
  const mcontext_t *mctx = &uctx->uc_mcontext;

  memset(sig_ctx, 0, sizeof(*sig_ctx));

  sig_ctx->prog_ctr = mctx->gregs[REG_RIP];
  sig_ctx->stack_ptr = mctx->gregs[REG_RSP];

  sig_ctx->rax = mctx->gregs[REG_RAX];
  sig_ctx->rbx = mctx->gregs[REG_RBX];
  sig_ctx->rcx = mctx->gregs[REG_RCX];
  sig_ctx->rdx = mctx->gregs[REG_RDX];
  sig_ctx->rsi = mctx->gregs[REG_RSI];
  sig_ctx->rdi = mctx->gregs[REG_RDI];
  sig_ctx->rbp = mctx->gregs[REG_RBP];
  sig_ctx->r8  = mctx->gregs[REG_R8];
  sig_ctx->r9  = mctx->gregs[REG_R9];
  sig_ctx->r10 = mctx->gregs[REG_R10];
  sig_ctx->r11 = mctx->gregs[REG_R11];
  sig_ctx->r12 = mctx->gregs[REG_R12];
  sig_ctx->r13 = mctx->gregs[REG_R13];
  sig_ctx->r14 = mctx->gregs[REG_R14];
  sig_ctx->r15 = mctx->gregs[REG_R15];
  sig_ctx->flags = mctx->gregs[REG_EFL];

  /* Record placeholder values. */
  sig_ctx->cs = 0;
  sig_ctx->gs = 0;
  sig_ctx->fs = 0;
  sig_ctx->ds = 0;
  sig_ctx->ss = 0;
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  mctx->gregs[REG_RIP] = sig_ctx->prog_ctr;
  mctx->gregs[REG_RSP] = sig_ctx->stack_ptr;

  mctx->gregs[REG_RAX] = sig_ctx->rax;
  mctx->gregs[REG_RBX] = sig_ctx->rbx;
  mctx->gregs[REG_RCX] = sig_ctx->rcx;
  mctx->gregs[REG_RDX] = sig_ctx->rdx;
  mctx->gregs[REG_RSI] = sig_ctx->rsi;
  mctx->gregs[REG_RDI] = sig_ctx->rdi;
  mctx->gregs[REG_RBP] = sig_ctx->rbp;
  mctx->gregs[REG_R8]  = sig_ctx->r8;
  mctx->gregs[REG_R9]  = sig_ctx->r9;
  mctx->gregs[REG_R10] = sig_ctx->r10;
  mctx->gregs[REG_R11] = sig_ctx->r11;
  mctx->gregs[REG_R12] = sig_ctx->r12;
  mctx->gregs[REG_R13] = sig_ctx->r13;
  mctx->gregs[REG_R14] = sig_ctx->r14;
  mctx->gregs[REG_R15] = sig_ctx->r15;
  mctx->gregs[REG_EFL] = sig_ctx->flags;

  /*
   * We do not support modifying any of %cs, %ds, %fs, %gs, and %ss in 64b, so
   * we do not push them back into the context.
   */
}



