/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Secure Runtime
 */
#include "native_client/src/include/portability_string.h"
#include "native_client/src/shared/platform/nacl_global_secure_random.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/arch/mips/sel_ldr_mips.h"

uint32_t nacl_guard_token;

void NaClInitGlobals(void) {
  NaClLog(2, "NaClInitGlobals\n");
  nacl_guard_token = NaClGlobalSecureRngUint32();
}


int NaClAppThreadInitArchSpecific(struct NaClAppThread *natp,
                                  nacl_reg_t           prog_ctr,
                                  nacl_reg_t           stack_ptr) {
  struct NaClThreadContext *ntcp = &natp->user;

  /*
   * We call this function so that it does not appear to be dead code,
   * although it only contains compile-time assertions.
   */
  NaClThreadContextOffsetCheck();

  memset((void *)ntcp, 0, sizeof(*ntcp));
  ntcp->stack_ptr = stack_ptr;
  ntcp->prog_ctr = prog_ctr;
  ntcp->tls_idx = NaClTlsAllocate(natp);
  if (ntcp->tls_idx == NACL_TLS_INDEX_INVALID)
    return 0;
  ntcp->t8 = (uintptr_t) &ntcp->tls_value1;
  ntcp->guard_token = nacl_guard_token;

  NaClLog(4, "user.tls_idx: 0x%08"NACL_PRIxNACL_REG"\n", ntcp->tls_idx);
  NaClLog(4, "user.stack_ptr: 0x%08"NACL_PRIxNACL_REG"\n", ntcp->stack_ptr);
  NaClLog(4, "user.prog_ctr: 0x%08"NACL_PRIxNACL_REG"\n", ntcp->prog_ctr);

  return 1;
}


void NaClThreadContextToSignalContext(const struct NaClThreadContext *th_ctx,
                                      struct NaClSignalContext *sig_ctx) {
  sig_ctx->zero        = 0;
  sig_ctx->at          = 0;
  sig_ctx->v0          = 0;
  sig_ctx->v1          = 0;
  sig_ctx->a0          = 0;
  sig_ctx->a1          = 0;
  sig_ctx->a2          = 0;
  sig_ctx->a3          = 0;
  sig_ctx->t0          = 0;
  sig_ctx->t1          = 0;
  sig_ctx->t2          = 0;
  sig_ctx->t3          = 0;
  sig_ctx->t4          = 0;
  sig_ctx->t5          = 0;
  sig_ctx->t6          = NACL_CONTROL_FLOW_MASK;
  sig_ctx->t7          = NACL_DATA_FLOW_MASK;
  sig_ctx->s0          = th_ctx->s0;
  sig_ctx->s1          = th_ctx->s1;
  sig_ctx->s2          = th_ctx->s2;
  sig_ctx->s3          = th_ctx->s3;
  sig_ctx->s4          = th_ctx->s4;
  sig_ctx->s5          = th_ctx->s5;
  sig_ctx->s6          = th_ctx->s6;
  sig_ctx->s7          = th_ctx->s7;
  sig_ctx->t8          = th_ctx->t8;
  sig_ctx->t9          = 0;
  sig_ctx->k0          = 0;
  sig_ctx->k1          = 0;
  sig_ctx->global_ptr  = 0;
  sig_ctx->stack_ptr   = th_ctx->stack_ptr;
  sig_ctx->frame_ptr   = th_ctx->frame_ptr;
  sig_ctx->prog_ctr    = th_ctx->new_prog_ctr;
  sig_ctx->return_addr = 0;
}


void NaClSignalContextUnsetClobberedRegisters(
    struct NaClSignalContext *sig_ctx) {
  sig_ctx->zero        = 0;
  sig_ctx->at          = 0;
  sig_ctx->v0          = 0;
  sig_ctx->v1          = 0;
  sig_ctx->a0          = 0;
  sig_ctx->a1          = 0;
  sig_ctx->a2          = 0;
  sig_ctx->a3          = 0;
  sig_ctx->t0          = 0;
  sig_ctx->t1          = 0;
  sig_ctx->t2          = 0;
  sig_ctx->t3          = 0;
  sig_ctx->t4          = 0;
  sig_ctx->t5          = 0;
  sig_ctx->t9          = 0;
  sig_ctx->k0          = 0;
  sig_ctx->k1          = 0;
  sig_ctx->global_ptr  = 0;
  sig_ctx->return_addr = 0;
}
