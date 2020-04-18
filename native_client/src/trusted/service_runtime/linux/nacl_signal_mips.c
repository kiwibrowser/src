/*
 * Copyright 2012 The Native Client Authors. All rights reserved.
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
  /*
   * TODO(petarj): Check whether float registers should be added here.
   */
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  memset(sig_ctx, 0, sizeof(*sig_ctx));

  sig_ctx->prog_ctr = mctx->pc;

  sig_ctx->zero = mctx->gregs[0];
  sig_ctx->at = mctx->gregs[1];
  sig_ctx->v0 = mctx->gregs[2];
  sig_ctx->v1 = mctx->gregs[3];
  sig_ctx->a0 = mctx->gregs[4];
  sig_ctx->a1 = mctx->gregs[5];
  sig_ctx->a2 = mctx->gregs[6];
  sig_ctx->a3 = mctx->gregs[7];
  sig_ctx->t0 = mctx->gregs[8];
  sig_ctx->t1 = mctx->gregs[9];
  sig_ctx->t2 = mctx->gregs[10];
  sig_ctx->t3 = mctx->gregs[11];
  sig_ctx->t4 = mctx->gregs[12];
  sig_ctx->t5 = mctx->gregs[13];
  sig_ctx->t6 = mctx->gregs[14];
  sig_ctx->t7 = mctx->gregs[15];
  sig_ctx->s0 = mctx->gregs[16];
  sig_ctx->s1 = mctx->gregs[17];
  sig_ctx->s2 = mctx->gregs[18];
  sig_ctx->s3 = mctx->gregs[19];
  sig_ctx->s4 = mctx->gregs[20];
  sig_ctx->s5 = mctx->gregs[21];
  sig_ctx->s6 = mctx->gregs[22];
  sig_ctx->s7 = mctx->gregs[23];
  sig_ctx->t8 = mctx->gregs[24];
  sig_ctx->t9 = mctx->gregs[25];
  sig_ctx->k0 = mctx->gregs[26];
  sig_ctx->k1 = mctx->gregs[27];
  sig_ctx->global_ptr = mctx->gregs[28];
  sig_ctx->stack_ptr = mctx->gregs[29];
  sig_ctx->frame_ptr = mctx->gregs[30];
  sig_ctx->return_addr = mctx->gregs[31];
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  mctx->pc = sig_ctx->prog_ctr;

  mctx->gregs[0] = sig_ctx->zero;
  mctx->gregs[1] = sig_ctx->at;
  mctx->gregs[2] = sig_ctx->v0;
  mctx->gregs[3] = sig_ctx->v1;
  mctx->gregs[4] = sig_ctx->a0;
  mctx->gregs[5] = sig_ctx->a1;
  mctx->gregs[6] = sig_ctx->a2;
  mctx->gregs[7] = sig_ctx->a3;
  mctx->gregs[8] = sig_ctx->t0;
  mctx->gregs[9] = sig_ctx->t1;
  mctx->gregs[10] = sig_ctx->t2;
  mctx->gregs[11] = sig_ctx->t3;
  mctx->gregs[12] = sig_ctx->t4;
  mctx->gregs[13] = sig_ctx->t5;
  mctx->gregs[14] = sig_ctx->t6;
  mctx->gregs[15] = sig_ctx->t7;
  mctx->gregs[16] = sig_ctx->s0;
  mctx->gregs[17] = sig_ctx->s1;
  mctx->gregs[18] = sig_ctx->s2;
  mctx->gregs[19] = sig_ctx->s3;
  mctx->gregs[20] = sig_ctx->s4;
  mctx->gregs[21] = sig_ctx->s5;
  mctx->gregs[22] = sig_ctx->s6;
  mctx->gregs[23] = sig_ctx->s7;
  mctx->gregs[24] = sig_ctx->t8;
  mctx->gregs[25] = sig_ctx->t9;
  mctx->gregs[26] = sig_ctx->k0;
  mctx->gregs[27] = sig_ctx->k1;
  mctx->gregs[28] = sig_ctx->global_ptr;
  mctx->gregs[29] = sig_ctx->stack_ptr;
  mctx->gregs[30] = sig_ctx->frame_ptr;
  mctx->gregs[31] = sig_ctx->return_addr;
}
