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

  sig_ctx->prog_ctr = mctx->gregs[REG_EIP];
  sig_ctx->stack_ptr = mctx->gregs[REG_ESP];

  sig_ctx->eax = mctx->gregs[REG_EAX];
  sig_ctx->ebx = mctx->gregs[REG_EBX];
  sig_ctx->ecx = mctx->gregs[REG_ECX];
  sig_ctx->edx = mctx->gregs[REG_EDX];
  sig_ctx->esi = mctx->gregs[REG_ESI];
  sig_ctx->edi = mctx->gregs[REG_EDI];
  sig_ctx->ebp = mctx->gregs[REG_EBP];
  sig_ctx->flags = mctx->gregs[REG_EFL];
   /*
    * We need to drop the top 16 bits with the casts below.  In some
    * situations, Linux does not assign to the top 2 bytes of the
    * REG_CS array entry when writing %cs to the stack (and similarly
    * for the other segment registers).  Therefore we need to drop the
    * undefined top 2 bytes.
    *
    * This happens in 32-bit processes running on the 64-bit kernel
    * from Ubuntu Hardy, but not on 32-bit kernels.  The kernel
    * version in Ubuntu Lucid also does not have this problem.
    *
    * See http://code.google.com/p/nativeclient/issues/detail?id=1486
    */
  sig_ctx->cs = (uint16_t) mctx->gregs[REG_CS];
  sig_ctx->ss = (uint16_t) mctx->gregs[REG_SS];
  sig_ctx->ds = (uint16_t) mctx->gregs[REG_DS];
  sig_ctx->es = (uint16_t) mctx->gregs[REG_ES];
  sig_ctx->fs = (uint16_t) mctx->gregs[REG_FS];
  sig_ctx->gs = (uint16_t) mctx->gregs[REG_GS];
}


/*
 * Update the raw platform dependent signal information from the
 * signal context structure.
 */
void NaClSignalContextToHandler(void *raw_ctx,
                                const struct NaClSignalContext *sig_ctx) {
  ucontext_t *uctx = (ucontext_t *) raw_ctx;
  mcontext_t *mctx = &uctx->uc_mcontext;

  mctx->gregs[REG_EIP] = sig_ctx->prog_ctr;
  mctx->gregs[REG_ESP] = sig_ctx->stack_ptr;

  mctx->gregs[REG_EAX] = sig_ctx->eax;
  mctx->gregs[REG_EBX] = sig_ctx->ebx;
  mctx->gregs[REG_ECX] = sig_ctx->ecx;
  mctx->gregs[REG_EDX] = sig_ctx->edx;
  mctx->gregs[REG_ESI] = sig_ctx->esi;
  mctx->gregs[REG_EDI] = sig_ctx->edi;
  mctx->gregs[REG_EBP] = sig_ctx->ebp;
  mctx->gregs[REG_EFL] = sig_ctx->flags;
  mctx->gregs[REG_CS] = sig_ctx->cs;
  mctx->gregs[REG_SS] = sig_ctx->ss;
  mctx->gregs[REG_DS] = sig_ctx->ds;
  mctx->gregs[REG_ES] = sig_ctx->es;
  mctx->gregs[REG_FS] = sig_ctx->fs;
  mctx->gregs[REG_GS] = sig_ctx->gs;
}



