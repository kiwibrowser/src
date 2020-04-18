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

  sig_ctx->prog_ctr = win_ctx->Eip;
  sig_ctx->stack_ptr = win_ctx->Esp;

  sig_ctx->eax = win_ctx->Eax;
  sig_ctx->ebx = win_ctx->Ebx;
  sig_ctx->ecx = win_ctx->Ecx;
  sig_ctx->edx = win_ctx->Edx;
  sig_ctx->esi = win_ctx->Esi;
  sig_ctx->edi = win_ctx->Edi;
  sig_ctx->ebp = win_ctx->Ebp;
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

  win_ctx->Eip = sig_ctx->prog_ctr;
  win_ctx->Esp = sig_ctx->stack_ptr;

  win_ctx->Eax = sig_ctx->eax;
  win_ctx->Ebx = sig_ctx->ebx;
  win_ctx->Ecx = sig_ctx->ecx;
  win_ctx->Edx = sig_ctx->edx;
  win_ctx->Esi = sig_ctx->esi;
  win_ctx->Edi = sig_ctx->edi;
  win_ctx->Ebp = sig_ctx->ebp;
  win_ctx->EFlags = sig_ctx->flags;
  win_ctx->SegCs = sig_ctx->cs;
  win_ctx->SegSs = sig_ctx->ss;
  win_ctx->SegDs = sig_ctx->ds;
  win_ctx->SegEs = sig_ctx->es;
  win_ctx->SegFs = sig_ctx->fs;
  win_ctx->SegGs = sig_ctx->gs;
}
