/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/trusted/service_runtime/thread_suspension_unwind.h"

#include "native_client/src/trusted/cpu_features/arch/x86/cpu_x86.h"
#include "native_client/src/trusted/service_runtime/arch/arm/tramp_arm.h"
#include "native_client/src/trusted/service_runtime/arch/x86_64/tramp_64.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"


static void GetNaClSyscallSeg(struct NaClApp *nap,
                              uintptr_t *nacl_syscall_seg,
                              uintptr_t *nacl_syscall_seg_regs_saved) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  NaClCPUFeaturesX86 *features = (NaClCPUFeaturesX86 *) nap->cpu_features;
  if (NaClGetCPUFeatureX86(features, NaClCPUFeatureX86_SSE)) {
    *nacl_syscall_seg = (uintptr_t) &NaClSyscallSegSSE;
    *nacl_syscall_seg_regs_saved = (uintptr_t) &NaClSyscallSegRegsSavedSSE;
  } else {
    *nacl_syscall_seg = (uintptr_t) &NaClSyscallSegNoSSE;
    *nacl_syscall_seg_regs_saved = (uintptr_t) &NaClSyscallSegRegsSavedNoSSE;
  }
#else
  UNREFERENCED_PARAMETER(nap);

  *nacl_syscall_seg = (uintptr_t) &NaClSyscallSeg;
  *nacl_syscall_seg_regs_saved = (uintptr_t) &NaClSyscallSegRegsSaved;
#endif
}

/*
 * This returns 0 if |regs| indicates that the thread's register state
 * has already been saved in NaClThreadContext.  Otherwise, it adjusts
 * |regs| to undo the effects of calling the syscall trampoline and
 * returns 1.
 */
static int Unwind(struct NaClAppThread *natp, struct NaClSignalContext *regs,
                  enum NaClUnwindCase *unwind_case) {
  struct NaClApp *nap = natp->nap;
  uintptr_t nacl_syscall_seg;
  uintptr_t nacl_syscall_seg_regs_saved;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  if (regs->cs == natp->user.cs &&
      regs->prog_ctr >= nap->syscall_return_springboard.start_addr &&
      regs->prog_ctr < nap->syscall_return_springboard.end_addr) {
    *unwind_case = NACL_UNWIND_in_springboard;
    return 0;
  }
  if (regs->cs == natp->user.cs &&
      regs->prog_ctr >= NACL_TRAMPOLINE_START &&
      regs->prog_ctr < NACL_TRAMPOLINE_END) {
    *unwind_case = NACL_UNWIND_in_trampoline;
    regs->stack_ptr += 4;  /* Pop user return address */
    return 1;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  if (regs->prog_ctr >= NaClUserToSys(nap, NACL_TRAMPOLINE_START) &&
      regs->prog_ctr < NaClUserToSys(nap, NACL_TRAMPOLINE_END)) {
    *unwind_case = NACL_UNWIND_in_trampoline;
    regs->stack_ptr += 8;  /* Pop user return address */
    return 1;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  if (regs->prog_ctr >= NACL_TRAMPOLINE_START &&
      regs->prog_ctr < NACL_TRAMPOLINE_END) {
    *unwind_case = NACL_UNWIND_in_trampoline;
    regs->prog_ctr = NaClSandboxCodeAddr(nap, regs->lr);
    return 1;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  if (regs->prog_ctr >= NACL_TRAMPOLINE_START &&
      regs->prog_ctr < NACL_TRAMPOLINE_END) {
    *unwind_case = NACL_UNWIND_in_trampoline;
    regs->prog_ctr = NaClSandboxCodeAddr(nap, regs->return_addr);
    return 1;
  }
#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  if (regs->cs == NaClGetGlobalCs() &&
      regs->prog_ctr >= nap->pcrel_thunk &&
      regs->prog_ctr < nap->pcrel_thunk_end) {
    *unwind_case = NACL_UNWIND_in_pcrel_thunk;
    regs->stack_ptr += 4 + 8;  /* Pop user + trampoline return addresses */
    return 1;
  }
#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  if (regs->prog_ctr >= (uintptr_t) &NaClGetTlsFastPath1 &&
      regs->prog_ctr < (uintptr_t) &NaClGetTlsFastPath1End) {
    *unwind_case = NACL_UNWIND_in_tls_fast_path;
    if (regs->prog_ctr < (uintptr_t) &NaClGetTlsFastPath1RspRestored) {
      regs->stack_ptr += 8;  /* Pop user return address */
    }
    return 1;
  }
  if (regs->prog_ctr >= (uintptr_t) &NaClGetTlsFastPath2 &&
      regs->prog_ctr < (uintptr_t) &NaClGetTlsFastPath2End) {
    *unwind_case = NACL_UNWIND_in_tls_fast_path;
    if (regs->prog_ctr < (uintptr_t) &NaClGetTlsFastPath2RspRestored) {
      regs->stack_ptr += 8;  /* Pop user return address */
    }
    return 1;
  }
#endif

  GetNaClSyscallSeg(nap, &nacl_syscall_seg, &nacl_syscall_seg_regs_saved);
  if (regs->prog_ctr >= nacl_syscall_seg &&
      regs->prog_ctr < nacl_syscall_seg_regs_saved) {
    *unwind_case = NACL_UNWIND_in_syscallseg;
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
    /* Pop user + trampoline return addresses */
    regs->stack_ptr += 4 + 8;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
    /* Pop user return address. */
    regs->stack_ptr += 8;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
    /*
     * On MIPS, $ra is not modified from the start of the trampoline to the
     * point where registers are saved.
     */
    regs->prog_ctr = NaClSandboxCodeAddr(nap, regs->return_addr);
#endif
    return 1;
  }

  *unwind_case = NACL_UNWIND_after_saving_regs;
  return 0;
}

/*
 * Given that thread |natp| has been suspended during a
 * trusted/untrusted context switch and has trusted register state
 * |regs|, this modifies |regs| to contain untrusted register state
 * (that is, the state the syscall will return with).
 */
void NaClGetRegistersForContextSwitch(struct NaClAppThread *natp,
                                      struct NaClSignalContext *regs,
                                      enum NaClUnwindCase *unwind_case) {
  if (Unwind(natp, regs, unwind_case)) {
    NaClSignalContextUnsetClobberedRegisters(regs);
  } else {
    NaClThreadContextToSignalContext(&natp->user, regs);
  }

  if (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 ||
      (NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm &&
       *unwind_case != NACL_UNWIND_in_trampoline) ||
      (NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips &&
       *unwind_case != NACL_UNWIND_in_trampoline &&
       *unwind_case != NACL_UNWIND_in_syscallseg)) {
    /*
     * Read the return address from the untrusted stack.
     * NaClCopyInFromUser() can fault or return an error here, but only
     * if the thread was suspended just before it was about to crash.
     * This can happen if untrusted code JMP'd to the trampoline with a
     * bad stack pointer or if the thread was racing with an munmap().
     */
    nacl_reg_t user_ret = 0;
    if (!NaClCopyInFromUser(natp->nap, &user_ret,
                            (uint32_t) (regs->stack_ptr + NACL_USERRET_FIX),
                            sizeof(user_ret))) {
      NaClLog(LOG_WARNING, "NaClGetRegistersForContextSwitch: "
              "Failed to read return address; using dummy value\n");
    }
    regs->prog_ctr = NaClSandboxCodeAddr(natp->nap, user_ret);
  }
}
