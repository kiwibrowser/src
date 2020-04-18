/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability_io.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"
#include "native_client/src/trusted/service_runtime/thread_suspension_unwind.h"
#include "native_client/src/trusted/service_runtime/win/debug_exception_handler.h"

void NaClUntrustedThreadsSuspendAll(struct NaClApp *nap, int save_registers) {
  size_t index;

  NaClXMutexLock(&nap->threads_mu);

  /*
   * TODO(mseaborn): A possible refinement here would be to use separate loops
   * for initiating and waiting for the suspension of the threads.  This might
   * be faster, since we would not be waiting for each thread to suspend one by
   * one.  It would take advantage of the asynchronous nature of thread
   * suspension.
   */
  for (index = 0; index < nap->threads.num_entries; index++) {
    struct NaClAppThread *natp = NaClGetThreadMu(nap, (int) index);
    if (natp != NULL) {
      NaClUntrustedThreadSuspend(natp, save_registers);
    }
  }
}

void NaClUntrustedThreadsResumeAll(struct NaClApp *nap) {
  size_t index;
  for (index = 0; index < nap->threads.num_entries; index++) {
    struct NaClAppThread *natp = NaClGetThreadMu(nap, (int) index);
    if (natp != NULL) {
      NaClUntrustedThreadResume(natp);
    }
  }

  NaClXMutexUnlock(&nap->threads_mu);
}

void NaClAppThreadGetSuspendedRegisters(struct NaClAppThread *natp,
                                        struct NaClSignalContext *regs) {
  if ((natp->suspend_state & NACL_APP_THREAD_UNTRUSTED) != 0) {
    NaClAppThreadGetSuspendedRegistersInternal(natp, regs);
    if (!NaClSignalContextIsUntrusted(natp, regs)) {
      enum NaClUnwindCase unwind_case;
      NaClGetRegistersForContextSwitch(natp, regs, &unwind_case);
    }
  } else {
    NaClThreadContextToSignalContext(&natp->user, regs);
  }
}

int NaClAppThreadIsSuspendedInSyscall(struct NaClAppThread *natp) {
  if ((natp->suspend_state & NACL_APP_THREAD_UNTRUSTED) != 0) {
    struct NaClSignalContext regs;
    NaClAppThreadGetSuspendedRegistersInternal(natp, &regs);
    return !NaClSignalContextIsUntrusted(natp, &regs);
  }
  return 1;
}

void NaClAppThreadSetSuspendedRegisters(struct NaClAppThread *natp,
                                        const struct NaClSignalContext *regs) {
  /*
   * We only allow modifying the register state if the thread was
   * suspended while executing untrusted code, not if the thread is
   * inside a NaCl syscall.
   */
  if ((natp->suspend_state & NACL_APP_THREAD_UNTRUSTED) != 0) {
    struct NaClSignalContext current_regs;
    NaClAppThreadGetSuspendedRegistersInternal(natp, &current_regs);
    if (NaClSignalContextIsUntrusted(natp, &current_regs)) {
      NaClAppThreadSetSuspendedRegistersInternal(natp, regs);
    } else {
      NaClLog(LOG_WARNING,
              "NaClAppThreadSetSuspendedRegisters: Registers not modified "
              "(thread suspended during trusted/untrusted context switch)\n");
    }
  } else {
    NaClLog(LOG_WARNING,
            "NaClAppThreadSetSuspendedRegisters: Registers not modified "
            "(thread suspended inside syscall)\n");
  }
}

int NaClFaultedThreadQueueEnable(struct NaClApp *nap) {
#if NACL_WINDOWS
  nap->faulted_thread_event = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (nap->faulted_thread_event == NULL) {
    NaClLog(LOG_FATAL,
            "NaClFaultedThreadQueueEnable: Failed to create event object for "
            "faulted thread events\n");
  }
#else
  int fds[2];
#if NACL_LINUX
  int ret = pipe2(fds, O_CLOEXEC);
#else
  int ret = pipe(fds);
#endif
  if (ret < 0) {
    NaClLog(LOG_FATAL,
            "NaClFaultedThreadQueueEnable: Failed to allocate pipe for "
            "faulted thread events\n");
  }
  nap->faulted_thread_fd_read = fds[0];
  nap->faulted_thread_fd_write = fds[1];
#endif
  nap->enable_faulted_thread_queue = 1;
  return NaClDebugExceptionHandlerEnsureAttached(nap);
}
