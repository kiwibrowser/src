/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <errno.h>

#include "native_client/src/include/build_config.h"

#if NACL_ANDROID
/* Android uses a non-canonical futex.h version that requires __user be set. */
#include <linux/compiler.h>
#endif
#include <linux/futex.h>
#include <signal.h>
#include <sys/syscall.h>

#include "native_client/src/include/concurrency_ops.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"

#if !defined(FUTEX_WAIT_PRIVATE)
# define FUTEX_WAIT_PRIVATE FUTEX_WAIT
#endif /* !defined(FUTEX_WAIT_PRIVATE) */
#if !defined(FUTEX_WAKE_PRIVATE)
# define FUTEX_WAKE_PRIVATE FUTEX_WAKE
#endif /* !defined(FUTEX_WAKE_PRIVATE) */

struct NaClAppThreadSuspendedRegisters {
  struct NaClSignalContext context;
};

/*
 * If |*addr| still contains |value|, this waits to be woken up by a
 * FutexWake(addr,...) call from another thread; otherwise, it returns
 * immediately.
 *
 * Note that if this is interrupted by a signal, the system call will
 * get restarted, but it will recheck whether |*addr| still contains
 * |value|.
 *
 * We use the *_PRIVATE variant to use process-local futexes which are
 * slightly faster than shared futexes.  (Private futexes are
 * well-known but are not covered by the Linux man page for futex(),
 * which is very out-of-date.)
 */
static void FutexWait(Atomic32 *addr, Atomic32 value) {
  if (syscall(__NR_futex, addr, FUTEX_WAIT_PRIVATE, value, 0, 0, 0) != 0) {
    /*
     * We get EWOULDBLOCK if *addr != value (EAGAIN is a synonym).
     * We get EINTR if interrupted by a signal.
     */
    if (errno != EINTR && errno != EWOULDBLOCK) {
      NaClLog(LOG_FATAL, "FutexWait: futex() failed with error %d\n", errno);
    }
  }
}

/*
 * This wakes up threads that are waiting on |addr| using FutexWait().
 * |waiters| is the maximum number of threads that will be woken up.
 */
static void FutexWake(Atomic32 *addr, int waiters) {
  if (syscall(__NR_futex, addr, FUTEX_WAKE_PRIVATE, waiters, 0, 0, 0) < 0) {
    NaClLog(LOG_FATAL, "FutexWake: futex() failed with error %d\n", errno);
  }
}

void NaClAppThreadSetSuspendState(struct NaClAppThread *natp,
                                  enum NaClSuspendState old_state,
                                  enum NaClSuspendState new_state) {
  while (1) {
    Atomic32 state = CompareAndSwap(&natp->suspend_state, old_state, new_state);
    if (NACL_LIKELY(state == (Atomic32) old_state)) {
      break;
    }
    if ((state & NACL_APP_THREAD_SUSPENDING) != 0) {
      /* We have been asked to suspend, so wait. */
      FutexWait(&natp->suspend_state, state);
    } else {
      NaClLog(LOG_FATAL, "NaClAppThreadSetSuspendState: Unexpected state: %i\n",
              state);
    }
  }
}

static void HandleSuspendSignal(struct NaClSignalContext *regs) {
  struct NaClAppThread *natp = NaClTlsGetCurrentThread();
  struct NaClSignalContext *suspended_registers =
      &natp->suspended_registers->context;

  /* Sanity check. */
  if (natp->suspend_state != (NACL_APP_THREAD_UNTRUSTED |
                              NACL_APP_THREAD_SUSPENDING)) {
    NaClSignalErrorMessage("HandleSuspendSignal: "
                           "Unexpected suspend_state\n");
    NaClAbort();
  }

  if (suspended_registers != NULL) {
    *suspended_registers = *regs;
    /*
     * Ensure that the change to natp->suspend_state does not become
     * visible before the change to natp->suspended_registers.
     */
    NaClWriteMemoryBarrier();
  }

  /*
   * Indicate that we have suspended by setting
   * NACL_APP_THREAD_SUSPENDED.  We should not need an atomic
   * operation for this since the calling thread will not be trying to
   * change suspend_state.
   */
  natp->suspend_state |= NACL_APP_THREAD_SUSPENDED;
  FutexWake(&natp->suspend_state, 1);

  /* Wait until we are asked to resume. */
  while (1) {
    Atomic32 state = natp->suspend_state;
    if ((state & NACL_APP_THREAD_SUSPENDED) != 0) {
      FutexWait(&natp->suspend_state, state);
      continue;  /* Retry */
    }
    break;
  }
  /*
   * Apply register modifications.  We need to use a snapshot of
   * natp->suspended_registers because, since we were asked to resume,
   * we could have been asked to suspend again, and
   * suspended_registers could have been allocated in the new
   * suspension request but not in the original suspension request.
   */
  if (suspended_registers != NULL) {
    *regs = *suspended_registers;
  }
}

static void FireDebugStubEvent(int pipe_fd) {
  char buf = 0;
  if (write(pipe_fd, &buf, sizeof(buf)) != sizeof(buf)) {
    NaClSignalErrorMessage("FireDebugStubEvent: Can't send debug stub event\n");
    NaClAbort();
  }
}

static void HandleUntrustedFault(int signal,
                                 struct NaClSignalContext *regs,
                                 struct NaClAppThread *natp) {
  /* Sanity check. */
  if ((natp->suspend_state & NACL_APP_THREAD_UNTRUSTED) == 0) {
    NaClSignalErrorMessage("HandleUntrustedFault: Unexpected suspend_state\n");
    NaClAbort();
  }

  /* Notify the debug stub by marking this thread as faulted. */
  natp->fault_signal = signal;
  AtomicIncrement(&natp->nap->faulted_thread_count, 1);
  FireDebugStubEvent(natp->nap->faulted_thread_fd_write);

  /*
   * We now expect the debug stub to suspend this thread via the
   * thread suspension API.  This will allow the debug stub to get the
   * register state at the point the fault happened.  The debug stub
   * will be able to modify the register state before unblocking the
   * thread using NaClAppThreadUnblockIfFaulted().
   */
  do {
    int new_signal;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, NACL_THREAD_SUSPEND_SIGNAL);
    if (sigwait(&sigset, &new_signal) != 0) {
      NaClSignalErrorMessage("HandleUntrustedFault: sigwait() failed\n");
      NaClAbort();
    }
    if (new_signal != NACL_THREAD_SUSPEND_SIGNAL) {
      NaClSignalErrorMessage("HandleUntrustedFault: "
                             "sigwait() returned unexpected result\n");
      NaClAbort();
    }
    HandleSuspendSignal(regs);
  } while (natp->fault_signal != 0);
}

int NaClThreadSuspensionSignalHandler(int signal,
                                      struct NaClSignalContext *regs,
                                      int is_untrusted,
                                      struct NaClAppThread *natp) {
  /*
   * We handle NACL_THREAD_SUSPEND_SIGNAL even if is_untrusted is
   * false, because the thread suspension signal can occur in trusted
   * code while switching from or to untrusted code while
   * suspend_state contains NACL_APP_THREAD_UNTRUSTED.
   */
  if (signal == NACL_THREAD_SUSPEND_SIGNAL) {
    HandleSuspendSignal(regs);
    return 1;
  }
  if (is_untrusted && natp->nap->enable_faulted_thread_queue) {
    HandleUntrustedFault(signal, regs, natp);
    return 1;
  }
  /* We did not handle this signal. */
  return 0;
}

/* Wait for the thread to indicate that it has suspended. */
static void WaitForUntrustedThreadToSuspend(struct NaClAppThread *natp) {
  const Atomic32 kBaseState = (NACL_APP_THREAD_UNTRUSTED |
                               NACL_APP_THREAD_SUSPENDING);
  while (1) {
    Atomic32 state = natp->suspend_state;
    if (state == kBaseState) {
      FutexWait(&natp->suspend_state, state);
      continue;  /* Retry */
    }
    if (state != (kBaseState | NACL_APP_THREAD_SUSPENDED)) {
      NaClLog(LOG_FATAL, "WaitForUntrustedThreadToSuspend: "
              "Unexpected state: %d\n", state);
    }
    break;
  }
}

void NaClUntrustedThreadSuspend(struct NaClAppThread *natp,
                                int save_registers) {
  Atomic32 old_state;
  Atomic32 suspending_state;

  /*
   * We do not want the thread to enter a NaCl syscall and start
   * taking locks when pthread_kill() takes effect, so we ask the
   * thread to suspend even if it is currently running untrusted code.
   */
  while (1) {
    old_state = natp->suspend_state;
    DCHECK((old_state & NACL_APP_THREAD_SUSPENDING) == 0);
    suspending_state = old_state | NACL_APP_THREAD_SUSPENDING;
    if (CompareAndSwap(&natp->suspend_state, old_state, suspending_state)
        != old_state) {
      continue;  /* Retry */
    }
    break;
  }
  /*
   * Once the thread has NACL_APP_THREAD_SUSPENDING set, it may not
   * change state itself, so there should be no race condition in this
   * check.
   */
  DCHECK(natp->suspend_state == suspending_state);

  if (old_state == NACL_APP_THREAD_UNTRUSTED) {
    /*
     * Allocate register state struct if needed.  This is race-free
     * when we are called by NaClUntrustedThreadsSuspendAll(), since
     * that claims nap->threads_mu.
     */
    if (save_registers && natp->suspended_registers == NULL) {
      natp->suspended_registers = malloc(sizeof(*natp->suspended_registers));
      if (natp->suspended_registers == NULL) {
        NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: malloc() failed\n");
      }
    }
    CHECK(natp->host_thread_is_defined);
    if (pthread_kill(natp->host_thread.tid, NACL_THREAD_SUSPEND_SIGNAL) != 0) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: "
              "pthread_kill() call failed\n");
    }
    WaitForUntrustedThreadToSuspend(natp);
  }
}

void NaClUntrustedThreadResume(struct NaClAppThread *natp) {
  Atomic32 old_state;
  Atomic32 new_state;
  while (1) {
    old_state = natp->suspend_state;
    new_state = old_state & ~(NACL_APP_THREAD_SUSPENDING |
                              NACL_APP_THREAD_SUSPENDED);
    DCHECK((old_state & NACL_APP_THREAD_SUSPENDING) != 0);
    if (CompareAndSwap(&natp->suspend_state, old_state, new_state)
        != old_state) {
      continue;  /* Retry */
    }
    break;
  }

  /*
   * TODO(mseaborn): A refinement would be to wake up the thread only
   * if it actually suspended during the context switch.
   */
  FutexWake(&natp->suspend_state, 1);
}

void NaClAppThreadGetSuspendedRegistersInternal(
    struct NaClAppThread *natp, struct NaClSignalContext *regs) {
  *regs = natp->suspended_registers->context;
}

void NaClAppThreadSetSuspendedRegistersInternal(
    struct NaClAppThread *natp, const struct NaClSignalContext *regs) {
  natp->suspended_registers->context = *regs;
}

int NaClAppThreadUnblockIfFaulted(struct NaClAppThread *natp, int *signal) {
  /* This function may only be called on a thread that is suspended. */
  DCHECK(natp->suspend_state == (NACL_APP_THREAD_UNTRUSTED |
                                 NACL_APP_THREAD_SUSPENDING |
                                 NACL_APP_THREAD_SUSPENDED) ||
         natp->suspend_state == (NACL_APP_THREAD_TRUSTED |
                                 NACL_APP_THREAD_SUSPENDING));

  if (natp->fault_signal == 0) {
    return 0;
  }
  *signal = natp->fault_signal;
  natp->fault_signal = 0;
  AtomicIncrement(&natp->nap->faulted_thread_count, -1);
  return 1;
}
