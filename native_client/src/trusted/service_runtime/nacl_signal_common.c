/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

#if NACL_WINDOWS
#include <io.h>
#define write _write
#else
#include <unistd.h>
#endif


ssize_t NaClSignalErrorMessage(const char *msg) {
  /*
   * We cannot use NaClLog() in the context of a signal handler: it is
   * too complex.  However, write() is signal-safe.
   */
  size_t len_t = strlen(msg);
  int len = (int) len_t;

  /*
   * Write uses int not size_t, so we may wrap the length and/or
   * generate a negative value.  Only print if it matches.
   */
  if ((len > 0) && (len_t == (size_t) len)) {
    return (ssize_t) write(2, msg, len);
  }

  return 0;
}

/*
 * This function takes the register state (sig_ctx) for a thread
 * (natp) that has been suspended and returns whether the thread was
 * suspended while executing untrusted code.
 */
int NaClSignalContextIsUntrusted(struct NaClAppThread *natp,
                                 const struct NaClSignalContext *sig_ctx) {
  uint32_t prog_ctr;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /*
   * Note that we do not check "sig_ctx != NaClGetGlobalCs()".  On Mac
   * OS X, if a thread is suspended while in a syscall,
   * thread_get_state() returns cs=0x7 rather than cs=0x17 (the normal
   * cs value for trusted code).
   */
  if (sig_ctx->cs != natp->user.cs)
    return 0;
#elif (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64) || \
      NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || \
      NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  if (!NaClIsUserAddr(natp->nap, sig_ctx->prog_ctr))
    return 0;
#else
# error Unsupported architecture
#endif

  prog_ctr = (uint32_t) sig_ctx->prog_ctr;
  return (prog_ctr < NACL_TRAMPOLINE_START ||
          prog_ctr >= NACL_TRAMPOLINE_END);
}

/*
 * Sanity checks: Reject unsafe register state that untrusted code
 * should not be able to set unless there is a sandbox hole.  We do
 * this in an attempt to prevent such a hole from being exploitable.
 */
int NaClSignalCheckSandboxInvariants(const struct NaClSignalContext *regs,
                                     struct NaClAppThread *natp) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  if (regs->cs != natp->user.cs ||
      regs->ds != natp->user.ds ||
      regs->es != natp->user.es ||
      regs->fs != natp->user.fs ||
      regs->gs != natp->user.gs ||
      regs->ss != natp->user.ss) {
    return 0;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  /*
   * Untrusted code can temporarily set %rsp/%rbp to be in the 0..4GB
   * range but it should not be able to generate a fault while in that
   * state.
   */
  if (regs->r15 != natp->user.r15 ||
      !NaClIsUserAddr(natp->nap, regs->stack_ptr) ||
      !NaClIsUserAddr(natp->nap, regs->rbp)) {
    return 0;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  if (!NaClIsUserAddr(natp->nap, regs->stack_ptr) ||
      regs->r9 != (uintptr_t) &natp->user.tls_value1) {
    return 0;
  }
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  if (!NaClIsUserAddr(natp->nap, regs->stack_ptr) ||
      regs->t6 != NACL_CONTROL_FLOW_MASK ||
      regs->t7 != NACL_DATA_FLOW_MASK) {
    return 0;
  }
#else
# error Unsupported architecture
#endif
  return 1;
}

void NaClSignalHandleUntrusted(int signal,
                               const struct NaClSignalContext *regs,
                               int is_untrusted) {
  char tmp[128];
  /*
   * Return an 8 bit error code which is -signal to
   * simulate normal OS behavior
   */
  if (is_untrusted) {
    SNPRINTF(tmp, sizeof(tmp), "\n** Signal %d from untrusted code: "
             "pc=%" NACL_PRIxNACL_REG "\n", signal, regs->prog_ctr);
    NaClSignalErrorMessage(tmp);
    NaClExit((-signal) & 0xFF);
  } else {
    SNPRINTF(tmp, sizeof(tmp), "\n** Signal %d from trusted code: "
             "pc=%" NACL_PRIxNACL_REG "\n", signal, regs->prog_ctr);
    NaClSignalErrorMessage(tmp);
    /*
     * Continue the search for another handler so that trusted crashes
     * can be handled by the Breakpad crash reporter.
     */
  }
}


/*
 * This is a separate function to make it obvious from the crash
 * reports that this crash is deliberate and for testing purposes.
 */
void NaClSignalTestCrashOnStartup(void) {
  if (getenv("NACL_CRASH_TEST") != NULL) {
    NaClSignalErrorMessage("[CRASH_TEST] Causing crash in NaCl "
                           "trusted code...\n");
    /*
     * Clang transmutes a NULL pointer reference into a generic
     * "undefined" case.  That code crashes with a different signal
     * than an actual bad pointer reference, violating the tests'
     * expectations.  A pointer that is known bad but is not literally
     * NULL does not get this treatment.
     */
    *(volatile int *) 1 = 0;
  }
}

static void NaClUserRegisterStateFromSignalContext(
    volatile NaClUserRegisterState *dest,
    const struct NaClSignalContext *src) {
#define COPY_REG(reg) dest->reg = src->reg
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  COPY_REG(eax);
  COPY_REG(ecx);
  COPY_REG(edx);
  COPY_REG(ebx);
  COPY_REG(stack_ptr);
  COPY_REG(ebp);
  COPY_REG(esi);
  COPY_REG(edi);
  COPY_REG(prog_ctr);
  COPY_REG(flags);
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  COPY_REG(rax);
  COPY_REG(rcx);
  COPY_REG(rdx);
  COPY_REG(rbx);
  COPY_REG(stack_ptr);
  COPY_REG(rbp);
  COPY_REG(rsi);
  COPY_REG(rdi);
  COPY_REG(r8);
  COPY_REG(r9);
  COPY_REG(r10);
  COPY_REG(r11);
  COPY_REG(r12);
  COPY_REG(r13);
  COPY_REG(r14);
  COPY_REG(r15);
  COPY_REG(prog_ctr);
  COPY_REG(flags);
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  COPY_REG(r0);
  COPY_REG(r1);
  COPY_REG(r2);
  COPY_REG(r3);
  COPY_REG(r4);
  COPY_REG(r5);
  COPY_REG(r6);
  COPY_REG(r7);
  COPY_REG(r8);
  /* Don't leak the address of NaClAppThread by reporting r9's value here. */
  dest->r9 = -1;
  COPY_REG(r10);
  COPY_REG(r11);
  COPY_REG(r12);
  COPY_REG(stack_ptr);
  COPY_REG(lr);
  COPY_REG(prog_ctr);
  COPY_REG(cpsr);
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  COPY_REG(zero);
  COPY_REG(at);
  COPY_REG(v0);
  COPY_REG(v1);
  COPY_REG(a0);
  COPY_REG(a1);
  COPY_REG(a2);
  COPY_REG(a3);
  COPY_REG(t0);
  COPY_REG(t1);
  COPY_REG(t2);
  COPY_REG(t3);
  COPY_REG(t4);
  COPY_REG(t5);
  COPY_REG(t6);
  COPY_REG(t7);
  COPY_REG(s0);
  COPY_REG(s1);
  COPY_REG(s2);
  COPY_REG(s3);
  COPY_REG(s4);
  COPY_REG(s5);
  COPY_REG(s6);
  COPY_REG(s7);
  COPY_REG(t8);
  COPY_REG(t9);
  COPY_REG(k0);
  COPY_REG(k1);
  COPY_REG(global_ptr);
  COPY_REG(stack_ptr);
  COPY_REG(frame_ptr);
  COPY_REG(return_addr);
  COPY_REG(prog_ctr);
#else
# error Unsupported architecture
#endif
#undef COPY_REG
}

/*
 * The |frame| argument is volatile because this function writes
 * directly into untrusted address space on Linux.
 */
void NaClSignalSetUpExceptionFrame(volatile struct NaClExceptionFrame *frame,
                                   const struct NaClSignalContext *regs,
                                   uint32_t context_user_addr) {
  unsigned i;

  /*
   * Use the end of frame->portable for the size, avoiding padding
   * added after it within NaClExceptionFrame.
   */
  frame->context.size =
      (uint32_t) ((uintptr_t) (&frame->portable + 1) -
                  (uintptr_t) &frame->context);
  frame->context.portable_context_offset =
      (uint32_t) ((uintptr_t) &frame->portable -
                  (uintptr_t) &frame->context);
  frame->context.portable_context_size = sizeof(frame->portable);
  frame->context.arch = NACL_ELF_E_MACHINE;
  frame->context.regs_size = sizeof(frame->context.regs);

  /* Avoid memset() here because |frame| is volatile. */
  for (i = 0; i < NACL_ARRAY_SIZE(frame->context.reserved); i++) {
    frame->context.reserved[i] = 0;
  }

  NaClUserRegisterStateFromSignalContext(&frame->context.regs, regs);

  frame->portable.prog_ctr = (uint32_t) regs->prog_ctr;
  frame->portable.stack_ptr = (uint32_t) regs->stack_ptr;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  frame->context_ptr = context_user_addr;
  frame->portable.frame_ptr = (uint32_t) regs->ebp;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  UNREFERENCED_PARAMETER(context_user_addr);
  frame->portable.frame_ptr = (uint32_t) regs->rbp;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  UNREFERENCED_PARAMETER(context_user_addr);
  frame->portable.frame_ptr = regs->r11;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  UNREFERENCED_PARAMETER(context_user_addr);
  frame->portable.frame_ptr = regs->frame_ptr;
#else
# error Unsupported architecture
#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /*
   * Returning from the exception handler is not possible, so to avoid
   * any confusion that might arise from jumping to an uninitialised
   * address, we set the return address to zero.
   */
  frame->return_addr = 0;
#endif
}
