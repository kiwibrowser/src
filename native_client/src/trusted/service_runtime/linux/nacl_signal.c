/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_exception.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"


/*
 * This module is based on the Posix signal model.  See:
 * http://www.opengroup.org/onlinepubs/009695399/functions/sigaction.html
 */

/*
 * The signals listed here should either be handled by NaCl (or otherwise
 * trusted code).
 */
static int s_Signals[] = {
#if NACL_ARCH(NACL_BUILD_ARCH) != NACL_mips
  /* This signal does not exist on MIPS. */
  SIGSTKFLT,
#endif
  SIGSYS, /* Used to support a seccomp-bpf sandbox. */
  NACL_THREAD_SUSPEND_SIGNAL,
  SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGBUS, SIGFPE, SIGSEGV, SIGTERM,
  /* Handle SIGABRT in case someone sends it asynchronously using kill(). */
  SIGABRT
};

static struct sigaction s_OldActions[NACL_ARRAY_SIZE_UNSAFE(s_Signals)];

static NaClSignalHandler g_handler_func;

void NaClSignalHandlerSet(NaClSignalHandler func) {
  g_handler_func = func;
}

/*
 * Returns, via is_untrusted, whether the signal happened while
 * executing untrusted code.
 *
 * Returns, via result_thread, the NaClAppThread that untrusted code
 * was running in.
 *
 * Note that this should only be called from the thread in which the
 * signal occurred, because on x86-64 it reads a thread-local variable
 * (nacl_current_thread).
 */
static void GetCurrentThread(const struct NaClSignalContext *sig_ctx,
                             int *is_untrusted,
                             struct NaClAppThread **result_thread) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /*
   * For x86-32, if %cs does not match, it is untrusted code.
   *
   * Note that this check might not be valid on Mac OS X, because
   * thread_get_state() does not return the value of NaClGetGlobalCs()
   * for a thread suspended inside a syscall.  However, this code is
   * not used on Mac OS X.
   */
  *is_untrusted = (NaClGetGlobalCs() != sig_ctx->cs);
  *result_thread = NaClAppThreadGetFromIndex(sig_ctx->gs >> 3);
#elif (NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64) || \
      NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm || \
      NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  struct NaClAppThread *natp = NaClTlsGetCurrentThread();
  if (natp == NULL) {
    *is_untrusted = 0;
    *result_thread = NULL;
  } else {
    /*
     * Get the address of an arbitrary local, stack-allocated variable,
     * just for the purpose of doing a sanity check.
     */
    void *pointer_into_stack = &natp;
    /*
     * Sanity check: Make sure the stack we are running on is not
     * allocated in untrusted memory.  This checks that the alternate
     * signal stack is correctly set up, because otherwise, if it is
     * not set up, the test case would not detect that.
     *
     * There is little point in doing a CHECK instead of a DCHECK,
     * because if we are running off an untrusted stack, we have already
     * lost.
     */
    DCHECK(!NaClIsUserAddr(natp->nap, (uintptr_t) pointer_into_stack));
    *is_untrusted = NaClIsUserAddr(natp->nap, sig_ctx->prog_ctr);
    *result_thread = natp;
  }
#else
# error Unsupported architecture
#endif

  /*
   * Trusted code could accidentally jump into sandbox address space,
   * so don't rely on prog_ctr on its own for determining whether a
   * crash comes from untrusted code.  We don't want to restore
   * control to an untrusted exception handler if trusted code
   * crashes.
   */
  if (*is_untrusted &&
      ((*result_thread)->suspend_state & NACL_APP_THREAD_UNTRUSTED) == 0) {
    *is_untrusted = 0;
  }
}

static void FindAndRunHandler(int sig, siginfo_t *info, void *uc) {
  unsigned int a;

  /* If we need to keep searching, try the old signal handler. */
  for (a = 0; a < NACL_ARRAY_SIZE(s_Signals); a++) {
    /* If we handle this signal */
    if (s_Signals[a] == sig) {
      /* If this is a real sigaction pointer... */
      if ((s_OldActions[a].sa_flags & SA_SIGINFO) != 0) {
        /*
         * On Mac OS X, sigaction() can return a "struct sigaction"
         * with SA_SIGINFO set but with a NULL sa_sigaction if no
         * signal handler was previously registered.  This is allowed
         * by POSIX, which does not require a struct returned by
         * sigaction() to be intelligible.  We check for NULL here to
         * avoid a crash.
         */
        if (s_OldActions[a].sa_sigaction != NULL) {
          /* then call the old handler. */
          s_OldActions[a].sa_sigaction(sig, info, uc);
          break;
        }
      } else {
        /* otherwise check if it is a real signal pointer */
        if ((s_OldActions[a].sa_handler != SIG_DFL) &&
            (s_OldActions[a].sa_handler != SIG_IGN)) {
          /* and call the old signal. */
          s_OldActions[a].sa_handler(sig);
          break;
        }
      }
      /*
       * We matched the signal, but didn't handle it, so we emulate
       * the default behavior which is to exit the app with the signal
       * number as the error code.
       */
      NaClExit(-sig);
    }
  }
}

/*
 * This function checks whether we can dispatch the signal to an
 * untrusted exception handler.  If we can, it modifies the register
 * state to call the handler and writes a stack frame into into
 * untrusted address space, and returns true.  Otherwise, it returns
 * false.
 */
static int DispatchToUntrustedHandler(struct NaClAppThread *natp,
                                      struct NaClSignalContext *regs) {
  struct NaClApp *nap = natp->nap;
  uintptr_t frame_addr;
  volatile struct NaClExceptionFrame *frame;
  uint32_t new_stack_ptr;
  uintptr_t context_user_addr;

  if (!NaClSignalCheckSandboxInvariants(regs, natp)) {
    return 0;
  }
  if (nap->exception_handler == 0) {
    return 0;
  }
  if (natp->exception_flag) {
    return 0;
  }

  natp->exception_flag = 1;

  if (natp->exception_stack == 0) {
    new_stack_ptr = regs->stack_ptr - NACL_STACK_RED_ZONE;
  } else {
    new_stack_ptr = natp->exception_stack;
  }
  /* Allocate space for the stack frame, and ensure its alignment. */
  new_stack_ptr -=
      sizeof(struct NaClExceptionFrame) - NACL_STACK_PAD_BELOW_ALIGN;
  new_stack_ptr = new_stack_ptr & ~NACL_STACK_ALIGN_MASK;
  new_stack_ptr -= NACL_STACK_ARGS_SIZE;
  new_stack_ptr -= NACL_STACK_PAD_BELOW_ALIGN;
  frame_addr = NaClUserToSysAddrRange(nap, new_stack_ptr,
                                      sizeof(struct NaClExceptionFrame));
  if (frame_addr == kNaClBadAddress) {
    /* We cannot write the stack frame. */
    return 0;
  }
  context_user_addr = new_stack_ptr + offsetof(struct NaClExceptionFrame,
                                               context);

  frame = (struct NaClExceptionFrame *) frame_addr;
  NaClSignalSetUpExceptionFrame(frame, regs, context_user_addr);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  regs->prog_ctr = nap->exception_handler;
  regs->stack_ptr = new_stack_ptr;
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 64
  regs->rdi = context_user_addr; /* Argument 1 */
  regs->prog_ctr = NaClUserToSys(nap, nap->exception_handler);
  regs->stack_ptr = NaClUserToSys(nap, new_stack_ptr);
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_arm
  /*
   * Returning from the exception handler is not possible, so to avoid
   * any confusion that might arise from jumping to an uninitialised
   * address, we set the return address to zero.
   */
  regs->lr = 0;
  regs->r0 = context_user_addr;  /* Argument 1 */
  regs->prog_ctr = NaClUserToSys(nap, nap->exception_handler);
  regs->stack_ptr = NaClUserToSys(nap, new_stack_ptr);
#elif NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips
  regs->return_addr = 0;
  regs->a0 = context_user_addr;
  regs->prog_ctr = NaClUserToSys(nap, nap->exception_handler);
  regs->stack_ptr = NaClUserToSys(nap, new_stack_ptr);
  /*
   * Per Linux/MIPS convention, PIC functions assume that t9 holds
   * the function's address on entry.
   */
  regs->t9 = regs->prog_ctr;
#else
# error Unsupported architecture
#endif

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  regs->flags &= ~NACL_X86_DIRECTION_FLAG;
#endif

  return 1;
}

static void SignalCatch(int sig, siginfo_t *info, void *uc) {
  struct NaClSignalContext sig_ctx;
  int is_untrusted;
  struct NaClAppThread *natp;

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /*
   * Reset the x86 direction flag.  New versions of gcc and libc
   * assume that the direction flag is clear on entry to a function,
   * as the x86 ABI requires.  However, untrusted code can set this
   * flag, and versions of Linux before 2.6.25 do not clear the flag
   * before running the signal handler, so we clear it here for safety.
   * See http://code.google.com/p/nativeclient/issues/detail?id=1495
   */
  __asm__("cld");
#endif

  NaClSignalContextFromHandler(&sig_ctx, uc);
  GetCurrentThread(&sig_ctx, &is_untrusted, &natp);

#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && NACL_BUILD_SUBARCH == 32
  /*
   * On Linux, the kernel does not restore %gs when entering the
   * signal handler, so we must do that here.  We need to do this for
   * TLS to work and for glibc's syscall wrappers to work, because
   * some builds of glibc fetch a syscall function pointer from the
   * static TLS area.  There is the potential for vulnerabilities if
   * we call glibc without restoring %gs (such as
   * http://code.google.com/p/nativeclient/issues/detail?id=1607),
   * although the risk is reduced because the untrusted %gs segment
   * has an extent of only 4 bytes (see
   * http://code.google.com/p/nativeclient/issues/detail?id=2176).
   *
   * Note that, in comparison, Breakpad tries to avoid using libc
   * calls at all when a crash occurs.
   *
   * For comparison, on Mac OS X, the kernel *does* restore the
   * original %gs when entering the signal handler.  On Mac, our
   * assignment to %gs here wouldn't be necessary, but it wouldn't be
   * harmful either.  However, this code is not currently used on Mac
   * OS X.
   *
   * Both Linux and Mac OS X necessarily restore %cs, %ds, and %ss
   * otherwise we would have a hard time handling signals generated by
   * untrusted code at all.
   *
   * Note that we check natp (which is based on %gs) rather than
   * is_untrusted (which is based on %cs) because we need to handle
   * the case where %gs is set to the untrusted-code value but %cs is
   * not.
   *
   * GCC's stack protector (-fstack-protector) will make use of %gs even before
   * we have a chance to restore it. It is important that this function is not
   * compiled with -fstack-protector.
   */
  if (natp != NULL) {
    NaClSetGs(natp->user.trusted_gs);
  }
#endif

  if (sig != SIGINT && sig != SIGQUIT) {
    if (NaClThreadSuspensionSignalHandler(sig, &sig_ctx, is_untrusted, natp)) {
      NaClSignalContextToHandler(uc, &sig_ctx);
      /* Resume untrusted code using possibly modified register state. */
      return;
    }
  }

  if (is_untrusted &&
      (sig == SIGSEGV || sig == SIGILL || sig == SIGFPE ||
       (NACL_ARCH(NACL_BUILD_ARCH) == NACL_mips && sig == SIGTRAP))) {
    if (DispatchToUntrustedHandler(natp, &sig_ctx)) {
      NaClSignalContextToHandler(uc, &sig_ctx);
      /* Resume untrusted code using the modified register state. */
      return;
    }
  }

  if (g_handler_func != NULL) {
    g_handler_func(sig, &sig_ctx, is_untrusted);
    return;
  }

  NaClSignalHandleUntrusted(sig, &sig_ctx, is_untrusted);

  FindAndRunHandler(sig, info, uc);
}


/*
 * Check that the current process has no signal handlers registered
 * that we won't override with safe handlers.
 *
 * We want to discourage Chrome or libraries from registering signal
 * handlers themselves, because those signal handlers are often not
 * safe when triggered from untrusted code.  For background, see:
 * http://code.google.com/p/nativeclient/issues/detail?id=1607
 */
static void AssertNoOtherSignalHandlers(void) {
  unsigned int index;
  int signum;
  char handled_by_nacl[NSIG];

  /* 0 is not a valid signal number. */
  for (signum = 1; signum < NSIG; signum++) {
    handled_by_nacl[signum] = 0;
  }
  for (index = 0; index < NACL_ARRAY_SIZE(s_Signals); index++) {
    signum = s_Signals[index];
    CHECK(signum > 0);
    CHECK(signum < NSIG);
    handled_by_nacl[signum] = 1;
  }
  for (signum = 1; signum < NSIG; signum++) {
    struct sigaction sa;

    if (handled_by_nacl[signum])
      continue;

    if (sigaction(signum, NULL, &sa) != 0) {
      /*
       * Don't complain if the kernel does not consider signum to be a
       * valid signal number, which produces EINVAL.
       */
      if (errno != EINVAL) {
        NaClLog(LOG_FATAL, "AssertNoOtherSignalHandlers: "
                "sigaction() call failed for signal %d: errno=%d\n",
                signum, errno);
      }
    } else {
      if ((sa.sa_flags & SA_SIGINFO) == 0) {
        if (sa.sa_handler == SIG_DFL || sa.sa_handler == SIG_IGN)
          continue;
      } else {
        /*
         * It is not strictly legal for sa_sigaction to contain NULL
         * or SIG_IGN, but Valgrind reports SIG_IGN for signal 64, so
         * we allow it here.
         */
        if (sa.sa_sigaction == NULL ||
            sa.sa_sigaction == (void (*)(int, siginfo_t *, void *)) SIG_IGN)
          continue;
      }
      NaClLog(LOG_FATAL, "AssertNoOtherSignalHandlers: "
              "A signal handler is registered for signal %d\n", signum);
    }
  }
}

void NaClSignalHandlerInit(void) {
  struct sigaction sa;
  unsigned int a;

  /*
   * Android adds a handler for SIGPIPE in the dynamic linker.
   */
  if (NACL_ANDROID)
    CHECK(signal(SIGPIPE, SIG_IGN) != SIG_ERR);

  AssertNoOtherSignalHandlers();

  memset(&sa, 0, sizeof(sa));
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = SignalCatch;
  sa.sa_flags = SA_ONSTACK | SA_SIGINFO;

  /*
   * Mask all signals we catch to prevent re-entry.
   *
   * In particular, NACL_THREAD_SUSPEND_SIGNAL must be masked while we
   * are handling a fault from untrusted code, otherwise the
   * suspension signal will interrupt the trusted fault handler.  That
   * would cause NaClAppThreadGetSuspendedRegisters() to report
   * trusted-code register state rather than untrusted-code register
   * state from the point where the fault occurred.
   */
  for (a = 0; a < NACL_ARRAY_SIZE(s_Signals); a++) {
    sigaddset(&sa.sa_mask, s_Signals[a]);
  }

  /* Install all handlers */
  for (a = 0; a < NACL_ARRAY_SIZE(s_Signals); a++) {
    if (sigaction(s_Signals[a], &sa, &s_OldActions[a]) != 0) {
      NaClLog(LOG_FATAL, "Failed to install handler for %d.\n\tERR:%s\n",
                          s_Signals[a], strerror(errno));
    }
  }
}

void NaClSignalHandlerFini(void) {
  unsigned int a;

  /* Remove all handlers */
  for (a = 0; a < NACL_ARRAY_SIZE(s_Signals); a++) {
    if (sigaction(s_Signals[a], &s_OldActions[a], NULL) != 0) {
      NaClLog(LOG_FATAL, "Failed to unregister handler for %d.\n\tERR:%s\n",
                          s_Signals[a], strerror(errno));
    }
  }
}
