/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <float.h>
#include <string.h>

#include "native_client/src/include/build_config.h"

/*
 * This header declares the _mm_getcsr function.
 */
#if NACL_WINDOWS
#include <intrin.h>
#else
#include <xmmintrin.h>
#endif

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"


void NaClInitGlobals(void) {
  /* no need to save segment registers */
  ;
}

int NaClAppThreadInitArchSpecific(struct NaClAppThread *natp,
                                  nacl_reg_t           prog_ctr,
                                  nacl_reg_t           stack_ptr) {
  struct NaClThreadContext *ntcp = &natp->user;
  struct NaClApp *nap = natp->nap;

  NaClThreadContextOffsetCheck();

  memset(ntcp, 0, sizeof(*ntcp));

  ntcp->rax = 0;
  ntcp->rbx = 0;
  ntcp->rcx = 0;
  ntcp->rdx = 0;

  ntcp->rbp = stack_ptr;  /* must be a valid stack addr! */
  ntcp->rsi = 0;
  ntcp->rdi = 0;
  ntcp->rsp = stack_ptr;

  ntcp->r8 = 0;
  ntcp->r9 = 0;
  ntcp->r10 = 0;
  ntcp->r11 = 0;

  ntcp->r12 = 0;
  ntcp->r13 = 0;
  ntcp->r14 = 0;
  ntcp->r15 = nap->mem_start;

  ntcp->prog_ctr = NaClUserToSys(nap, prog_ctr);
  ntcp->new_prog_ctr = 0;
  ntcp->sysret = (nacl_reg_t) -NACL_ABI_EINVAL;

  ntcp->tls_idx = NaClTlsAllocate(natp);
  if (ntcp->tls_idx == NACL_TLS_INDEX_INVALID)
    return 0;

  ntcp->fcw = NACL_X87_FCW_DEFAULT;
  ntcp->mxcsr = NACL_MXCSR_DEFAULT;

  /*
   * Save the system's state of the x87 FPU control word so we can restore
   * the same state when returning to trusted code.
   */
#if NACL_WINDOWS
  NaClDoFnstcw(&ntcp->sys_fcw);
#else
  __asm__ __volatile__("fnstcw %0" : "=m" (ntcp->sys_fcw));
#endif

  /*
   * Likewise for the SSE control word.
   */
  ntcp->sys_mxcsr = _mm_getcsr();

  return 1;
}

void NaClThreadContextToSignalContext(const struct NaClThreadContext *th_ctx,
                                      struct NaClSignalContext *sig_ctx) {
  sig_ctx->rax       = 0;
  sig_ctx->rbx       = th_ctx->rbx;
  sig_ctx->rcx       = 0;
  sig_ctx->rdx       = 0;
  sig_ctx->rsi       = 0;
  sig_ctx->rdi       = 0;
  sig_ctx->rbp       = th_ctx->rbp;
  sig_ctx->stack_ptr = th_ctx->rsp;
  sig_ctx->r8        = 0;
  sig_ctx->r9        = 0;
  sig_ctx->r10       = 0;
  sig_ctx->r11       = 0;
  sig_ctx->r12       = th_ctx->r12;
  sig_ctx->r13       = th_ctx->r13;
  sig_ctx->r14       = th_ctx->r14;
  sig_ctx->r15       = th_ctx->r15;
  sig_ctx->prog_ctr  = th_ctx->new_prog_ctr;
  sig_ctx->flags     = 0;
  sig_ctx->cs        = 0;
  sig_ctx->ss        = 0;
  sig_ctx->ds        = 0;
  sig_ctx->es        = 0;
  sig_ctx->fs        = 0;
  sig_ctx->gs        = 0;
}


void NaClSignalContextUnsetClobberedRegisters(
    struct NaClSignalContext *sig_ctx) {
  sig_ctx->rax       = 0;

  sig_ctx->rcx       = 0;
  sig_ctx->rdx       = 0;
  sig_ctx->rsi       = 0;
  sig_ctx->rdi       = 0;

  sig_ctx->r8        = 0;
  sig_ctx->r9        = 0;
  sig_ctx->r10       = 0;
  sig_ctx->r11       = 0;

  sig_ctx->flags     = 0;
}
