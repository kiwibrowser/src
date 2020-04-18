/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <float.h>

#include "native_client/src/include/build_config.h"

#if NACL_WINDOWS
/*
 * This header declares the _mm_getcsr function.
 */
#include <intrin.h>
#endif

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"


uint16_t  nacl_global_cs = 0;
uint16_t  nacl_global_ds = 0;


void NaClInitGlobals(void) {
  nacl_global_cs = NaClGetCs();
  nacl_global_ds = NaClGetDs();
}

uint16_t NaClGetGlobalDs(void) {
  return nacl_global_ds;
}

uint16_t NaClGetGlobalCs(void) {
  return nacl_global_cs;
}

int NaClAppThreadInitArchSpecific(struct NaClAppThread *natp,
                                  nacl_reg_t           prog_ctr,
                                  nacl_reg_t           stack_ptr) {
  struct NaClThreadContext *ntcp = &natp->user;
  struct NaClApp *nap = natp->nap;
  /* TODO(mcgrathr): Use a safe cast here. */
  NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) nap->cpu_features;

  NaClThreadContextOffsetCheck();

  NaClLog(4, "&nap->code_seg_sel = 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) &nap->code_seg_sel);
  NaClLog(4, "&nap->data_seg_sel = 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) &nap->data_seg_sel);
  NaClLog(4, "nap->code_seg_sel = 0x%02x\n", nap->code_seg_sel);
  NaClLog(4, "nap->data_seg_sel = 0x%02x\n", nap->data_seg_sel);

  /*
   * initialize registers to appropriate values.  most registers just
   * get zero, but for the segment register we allocate segment
   * selectors for the NaCl app, based on its address space.
   */
  ntcp->ebx = 0;
  ntcp->esi = 0;
  ntcp->edi = 0;
  ntcp->frame_ptr = 0;
  ntcp->stack_ptr = stack_ptr;
  ntcp->prog_ctr = prog_ctr;
  ntcp->new_prog_ctr = 0;
  ntcp->sysret = (nacl_reg_t) -NACL_ABI_EINVAL;

  ntcp->cs = nap->code_seg_sel;
  ntcp->ds = nap->data_seg_sel;

  ntcp->es = nap->data_seg_sel;
  ntcp->fs = 0;  /* windows use this for TLS and SEH; linux does not */
  ntcp->gs = NaClTlsAllocate(natp);
  if (ntcp->gs == NACL_TLS_INDEX_INVALID)
    return 0;
  ntcp->ss = nap->data_seg_sel;

  ntcp->fcw = NACL_X87_FCW_DEFAULT;
  ntcp->mxcsr = NACL_MXCSR_DEFAULT;

  /*
   * Save the system's state of the x87 FPU control word so we can restore
   * the same state when returning to trusted code.
   */
#if NACL_WINDOWS
  {
    uint16_t sys_fcw;
    __asm {
      fnstcw sys_fcw;
    }
    ntcp->sys_fcw = sys_fcw;
  }
#else
  __asm__ __volatile__("fnstcw %0" : "=m" (ntcp->sys_fcw));
#endif

  /*
   * Likewise for the SSE control word, if SSE is supported.
   */
  if (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_SSE)) {
#if NACL_WINDOWS
    ntcp->sys_mxcsr = _mm_getcsr();
#else
    /*
     * GCC actually defines _mm_getcsr too, in its <xmmintrin.h>.
     * But that (and the __builtin_ia32_stmxcsr it uses) are only
     * available when compiling with -msse, which also makes the
     * compiler generate SSE instructions itself, which would be
     * incompatible with systems that don't support SSE.
     */
    __asm__("stmxcsr %0" : "=m" (ntcp->sys_mxcsr));
#endif
  }

  NaClLog(4, "user.cs: 0x%02x\n", ntcp->cs);
  NaClLog(4, "user.fs: 0x%02x\n", ntcp->fs);
  NaClLog(4, "user.gs: 0x%02x\n", ntcp->gs);
  NaClLog(4, "user.ss: 0x%02x\n", ntcp->ss);

  return 1;
}


void NaClThreadContextToSignalContext(const struct NaClThreadContext *th_ctx,
                                      struct NaClSignalContext *sig_ctx) {
  sig_ctx->eax       = 0;
  sig_ctx->ecx       = 0;
  sig_ctx->edx       = 0;
  sig_ctx->ebx       = th_ctx->ebx;
  sig_ctx->stack_ptr = th_ctx->stack_ptr;
  sig_ctx->ebp       = th_ctx->frame_ptr;
  sig_ctx->esi       = th_ctx->esi;
  sig_ctx->edi       = th_ctx->edi;
  sig_ctx->prog_ctr  = th_ctx->new_prog_ctr;
  sig_ctx->flags     = 0;
  sig_ctx->cs        = th_ctx->cs;
  sig_ctx->ss        = th_ctx->ss;
  sig_ctx->ds        = th_ctx->ds;
  sig_ctx->es        = th_ctx->es;
  sig_ctx->fs        = th_ctx->fs;
  sig_ctx->gs        = th_ctx->gs;
}


void NaClSignalContextUnsetClobberedRegisters(
    struct NaClSignalContext *sig_ctx) {
  sig_ctx->eax       = 0;
  sig_ctx->ecx       = 0;
  sig_ctx->edx       = 0;
  sig_ctx->flags     = 0;
}
