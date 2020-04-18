/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime, C-level context switch code.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/arch/x86/sel_rt.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"

void NaClInitSwitchToApp(struct NaClApp *nap) {
  /* TODO(jfb) Use a safe cast here. */
  NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) nap->cpu_features;
  if (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_AVX)) {
    NaClSwitch = NaClSwitchAVX;
  } else if (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_SSE)) {
    NaClSwitch = NaClSwitchSSE;
  } else {
    NaClSwitch = NaClSwitchNoSSE;
  }
}

NORETURN void NaClStartThreadInApp(struct NaClAppThread *natp,
                                   nacl_reg_t           new_prog_ctr) {
  struct NaClApp            *nap;
  struct NaClThreadContext  *context;
  /*
   * Save service runtime segment registers; fs/gs is used for TLS
   * on Windows and Linux respectively, so will change.  The others
   * should be global, but we save them from the thread anyway.
   *
   * %cs and %ds are restored by trampoline code, so not saved here.
   */
  natp->user.trusted_es = NaClGetEs();
  natp->user.trusted_fs = NaClGetFs();
#if NACL_WINDOWS
  /*
   * Win32 leaks %gs values on return from a windows syscall if the
   * previously running thread had a non-zero %gs, e.g., an untrusted
   * thread was interrupted by the scheduler.  If we used NaClGetGs()
   * here, then in the trampoline context switch code, we will try to
   * restore %gs to this leaked value, possibly generating a fault
   * since that segment selector may not be valid (e.g., if that
   * earlier thread had exited and its selector had been deallocated).
   */
  natp->user.trusted_gs = 0;
#else
  natp->user.trusted_gs = NaClGetGs();
#endif
  natp->user.trusted_ss = NaClGetSs();
  /*
   * Preserves stack alignment.  The trampoline code loads this value
   * to %esp, then pushes the thread ID (LDT index) onto the stack as
   * argument to NaClSyscallCSegHook.  See nacl_syscall.S.
   */
  natp->user.trusted_stack_ptr = (NaClGetStackPtr() & ~0xf) + 4;

  nap = natp->nap;
  context = &natp->user;
  context->spring_addr = nap->syscall_return_springboard.start_addr;
  context->new_prog_ctr = new_prog_ctr;
  context->sysret = 0; /* %eax not used to return */

  NaClSwitch(context);

#if NACL_WINDOWS && defined(__clang__)
  /*
   * NaClSwitch is a function pointer in x86_32 and there's no way to mark
   * the function pointed to by a function pointer as noreturn in clang-cl mode.
   */
  __builtin_unreachable();
#endif
}
