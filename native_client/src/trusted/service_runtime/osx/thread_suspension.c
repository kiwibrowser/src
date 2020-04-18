/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <mach/mach.h>
#include <mach/thread_status.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
# include "native_client/src/trusted/service_runtime/arch/x86_32/nacl_switch_all_regs_32.h"
#endif


struct NaClAppThreadSuspendedRegisters {
  x86_thread_state_t context;
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  struct NaClSwitchRemainingRegsState switch_state;
#endif
};

static mach_port_t GetHostThreadPort(struct NaClAppThread *natp) {
  CHECK(natp->host_thread_is_defined);
  return pthread_mach_thread_np(natp->host_thread.tid);
}

void NaClAppThreadSetSuspendState(struct NaClAppThread *natp,
                                  enum NaClSuspendState old_state,
                                  enum NaClSuspendState new_state) {
  /*
   * Claiming suspend_mu here blocks a trusted/untrusted context
   * switch while the thread is suspended or a suspension is in
   * progress.
   */
  NaClXMutexLock(&natp->suspend_mu);
  DCHECK(natp->suspend_state == (Atomic32) old_state);
  natp->suspend_state = new_state;
  NaClXMutexUnlock(&natp->suspend_mu);
}

void NaClUntrustedThreadSuspend(struct NaClAppThread *natp,
                                int save_registers) {
  /*
   * We claim suspend_mu here to block trusted/untrusted context
   * switches by blocking NaClAppThreadSetSuspendState().  This blocks
   * any untrusted->trusted context switch that might happen before
   * SuspendThread() takes effect.  It blocks any trusted->untrusted
   * context switch that might happen if the syscall running in the
   * target thread returns.
   */
  NaClXMutexLock(&natp->suspend_mu);
  if (natp->suspend_state == NACL_APP_THREAD_UNTRUSTED) {
    kern_return_t result;
    mach_msg_type_number_t size;
    mach_port_t thread_port = GetHostThreadPort(natp);

    result = thread_suspend(thread_port);
    if (result != KERN_SUCCESS) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: "
              "thread_suspend() call failed: error %d\n", (int) result);
    }

    if (save_registers) {
      if (natp->suspended_registers == NULL) {
        natp->suspended_registers = malloc(sizeof(*natp->suspended_registers));
        if (natp->suspended_registers == NULL) {
          NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: malloc() failed\n");
        }
      }

      size = sizeof(natp->suspended_registers->context) / sizeof(natural_t);
      result = thread_get_state(thread_port, x86_THREAD_STATE,
                                (void *) &natp->suspended_registers->context,
                                &size);
      if (result != KERN_SUCCESS) {
        NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: "
                "thread_get_state() call failed: error %d\n", (int) result);
      }
    }
  }
  /*
   * We leave suspend_mu held so that NaClAppThreadSetSuspendState()
   * will block.
   */
}

void NaClUntrustedThreadResume(struct NaClAppThread *natp) {
  if (natp->suspend_state == NACL_APP_THREAD_UNTRUSTED) {
    kern_return_t result = thread_resume(GetHostThreadPort(natp));
    if (result != KERN_SUCCESS) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadResume: "
              "thread_resume() call failed: error %d\n", (int) result);
    }
  }
  NaClXMutexUnlock(&natp->suspend_mu);
}

void NaClAppThreadGetSuspendedRegistersInternal(
    struct NaClAppThread *natp, struct NaClSignalContext *regs) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /*
   * We might have suspended the thread while it is returning to
   * untrusted code via NaClSwitchRemainingRegsViaECX() and
   * NaCl_springboard_all_regs.  This is particularly likely for a
   * faulted thread that has been resumed and suspended again without
   * ever being unblocked by NaClAppThreadUnblockIfFaulted().
   *
   * In this situation, we must undo the register state modifications
   * made by NaClAppThreadSetSuspendedRegistersInternal().
   */
  struct NaClAppThreadSuspendedRegisters *state = natp->suspended_registers;
  struct NaClApp *nap = natp->nap;
  uint32_t eip = state->context.uts.ts32.__eip;
  if ((state->context.uts.ts32.__cs == NaClGetGlobalCs() &&
       eip >= (uintptr_t) NaClSwitchRemainingRegsViaECX &&
       eip < (uintptr_t) NaClSwitchRemainingRegsAsmEnd) ||
      (state->context.uts.ts32.__cs == natp->user.cs &&
       eip >= nap->all_regs_springboard.start_addr &&
       eip < nap->all_regs_springboard.end_addr)) {
    state->context.uts.ts32.__eip = natp->user.gs_segment.new_prog_ctr;
    state->context.uts.ts32.__ecx = natp->user.gs_segment.new_ecx;
    /*
     * It is sometimes necessary to restore the following registers
     * too, depending on how far we are through
     * NaClSwitchRemainingRegsViaECX().
     */
    state->context.uts.ts32.__cs = natp->user.cs;
    state->context.uts.ts32.__ds = natp->user.ds;
    state->context.uts.ts32.__es = natp->user.es;
    state->context.uts.ts32.__fs = natp->user.fs;
    state->context.uts.ts32.__gs = natp->user.gs;
    state->context.uts.ts32.__ss = natp->user.ss;
  }
#endif

  NaClSignalContextFromMacThreadState(regs,
                                      &natp->suspended_registers->context);
}

void NaClAppThreadSetSuspendedRegistersInternal(
    struct NaClAppThread *natp, const struct NaClSignalContext *regs) {
  kern_return_t result;
  mach_msg_type_number_t size;
  struct NaClAppThreadSuspendedRegisters *state = natp->suspended_registers;
  x86_thread_state_t context_copy;

  NaClSignalContextToMacThreadState(&state->context, regs);
  context_copy = state->context;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /*
   * thread_set_state() ignores the %cs value we supply and always
   * resets %cs back to the trusted-code value.  This means we must
   * set up the new untrusted register state via a trusted code
   * routine which returns to untrusted code via a springboard.
   *
   * We reset %cs here in case the Mac kernel is ever fixed to not
   * ignore the supplied %cs value.
   */
  context_copy.uts.ts32.__cs = NaClGetGlobalCs();
  context_copy.uts.ts32.__ds = NaClGetGlobalDs();
  /* Reset these too just in case. */
  context_copy.uts.ts32.__es = NaClGetGlobalDs();
  context_copy.uts.ts32.__ss = NaClGetGlobalDs();
  context_copy.uts.ts32.__ecx = (uintptr_t) &state->switch_state;
  context_copy.uts.ts32.__eip = (uintptr_t) NaClSwitchRemainingRegsViaECX;
  NaClSwitchRemainingRegsSetup(&state->switch_state, natp, regs);
#endif

  size = sizeof(context_copy) / sizeof(natural_t);
  result = thread_set_state(GetHostThreadPort(natp), x86_THREAD_STATE,
                            (void *) &context_copy, size);
  if (result != KERN_SUCCESS) {
    NaClLog(LOG_FATAL, "NaClAppThreadSetSuspendedRegistersInternal: "
            "thread_set_state() call failed: error %d\n", result);
  }
}

int NaClAppThreadUnblockIfFaulted(struct NaClAppThread *natp, int *signal) {
  kern_return_t result;
  if (natp->fault_signal == 0) {
    return 0;
  }
  *signal = natp->fault_signal;
  natp->fault_signal = 0;
  AtomicIncrement(&natp->nap->faulted_thread_count, -1);
  /*
   * Decrement the kernel's suspension count for the thread.  This
   * undoes the effect of mach_exception_handler.c's thread_suspend()
   * call.
   */
  result = thread_resume(GetHostThreadPort(natp));
  if (result != KERN_SUCCESS) {
    NaClLog(LOG_FATAL, "NaClAppThreadUnblockIfFaulted: "
            "thread_resume() call failed: error %d\n", (int) result);
  }
  return 1;
}
