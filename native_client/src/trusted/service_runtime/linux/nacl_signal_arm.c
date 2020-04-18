/*
 * Copyright (c) 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <signal.h>
#include <string.h>
#include <sys/ucontext.h>

#include "native_client/src/trusted/service_runtime/nacl_signal.h"

/*
 * Fill a signal context structure from the raw platform dependent
 * signal information.
 */
void NaClSignalContextFromHandler(struct NaClSignalContext *sig_ctx,
                                  const void *raw_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  memset(sig_ctx, 0, sizeof(*sig_ctx));

  sig_ctx->prog_ctr = mctx->arm_pc;
  sig_ctx->stack_ptr = mctx->arm_sp;

  sig_ctx->r0 = mctx->arm_r0;
  sig_ctx->r1 = mctx->arm_r1;
  sig_ctx->r2 = mctx->arm_r2;
  sig_ctx->r3 = mctx->arm_r3;
  sig_ctx->r4 = mctx->arm_r4;
  sig_ctx->r5 = mctx->arm_r5;
  sig_ctx->r6 = mctx->arm_r6;
  sig_ctx->r7 = mctx->arm_r7;
  sig_ctx->r8 = mctx->arm_r8;
  sig_ctx->r9 = mctx->arm_r9;
  sig_ctx->r10 = mctx->arm_r10;
  sig_ctx->r11 = mctx->arm_fp;
  sig_ctx->r12 = mctx->arm_ip;
  sig_ctx->lr = mctx->arm_lr;
  sig_ctx->cpsr = mctx->arm_cpsr;
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  mctx->arm_pc = sig_ctx->prog_ctr;
  mctx->arm_sp = sig_ctx->stack_ptr;

  mctx->arm_r0 = sig_ctx->r0;
  mctx->arm_r1 = sig_ctx->r1;
  mctx->arm_r2 = sig_ctx->r2;
  mctx->arm_r3 = sig_ctx->r3;
  mctx->arm_r4 = sig_ctx->r4;
  mctx->arm_r5 = sig_ctx->r5;
  mctx->arm_r6 = sig_ctx->r6;
  mctx->arm_r7 = sig_ctx->r7;
  mctx->arm_r8 = sig_ctx->r8;
  mctx->arm_r9 = sig_ctx->r9;
  mctx->arm_r10 = sig_ctx->r10;
  mctx->arm_fp = sig_ctx->r11;
  mctx->arm_ip = sig_ctx->r12;
  mctx->arm_lr = sig_ctx->lr;
  mctx->arm_cpsr = sig_ctx->cpsr;
}
