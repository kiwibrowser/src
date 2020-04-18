/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <windows.h>

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/thread_suspension.h"


struct NaClAppThreadSuspendedRegisters {
  CONTEXT context;
};

static HANDLE GetHostThreadHandle(struct NaClAppThread *natp) {
  CHECK(natp->host_thread_is_defined);
  return natp->host_thread.tid;
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
  DCHECK(natp->suspend_state == old_state);
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
    CONTEXT temp_context;
    CONTEXT *context;
    HANDLE thread_handle = GetHostThreadHandle(natp);

    if (SuspendThread(thread_handle) == (DWORD) -1) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: "
              "SuspendThread() call failed, error %u\n",
              GetLastError());
    }

    if (save_registers) {
      if (natp->suspended_registers == NULL) {
        natp->suspended_registers = malloc(sizeof(*natp->suspended_registers));
        if (natp->suspended_registers == NULL) {
          NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: malloc() failed\n");
        }
      }
      context = &natp->suspended_registers->context;
      context->ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    } else {
      /*
       * SuspendThread() can return before the thread has been
       * suspended, because internally it only sends a message asking
       * for the thread to be suspended.
       * See http://code.google.com/p/nativeclient/issues/detail?id=2557
       *
       * As a workaround for that, we call GetThreadContext() even
       * when save_registers=0.  GetThreadContext() should only be
       * able to return a snapshot of the register state once the
       * thread has actually suspended.
       *
       * If save_registers=0, the set of registers we request via
       * ContextFlags is unimportant as long as it is non-empty.
       */
      context = &temp_context;
      context->ContextFlags = CONTEXT_CONTROL;
    }

    if (!GetThreadContext(thread_handle, context)) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadSuspend: "
              "GetThreadContext() failed, error %u\n",
              GetLastError());
    }
  }
  /*
   * We leave suspend_mu held so that NaClAppThreadSetSuspendState()
   * will block.
   */
}

void NaClUntrustedThreadResume(struct NaClAppThread *natp) {
  if (natp->suspend_state == NACL_APP_THREAD_UNTRUSTED) {
    if (ResumeThread(GetHostThreadHandle(natp)) == (DWORD) -1) {
      NaClLog(LOG_FATAL, "NaClUntrustedThreadResume: "
              "ResumeThread() call failed, error %u\n",
              GetLastError());
    }
  }
  NaClXMutexUnlock(&natp->suspend_mu);
}

void NaClAppThreadGetSuspendedRegistersInternal(
    struct NaClAppThread *natp, struct NaClSignalContext *regs) {
  NaClSignalContextFromHandler(regs, natp->suspended_registers);
}

void NaClAppThreadSetSuspendedRegistersInternal(
    struct NaClAppThread *natp, const struct NaClSignalContext *regs) {
  NaClSignalContextToHandler(natp->suspended_registers, regs);
  if (!SetThreadContext(GetHostThreadHandle(natp),
                        &natp->suspended_registers->context)) {
    NaClLog(LOG_FATAL, "NaClAppThreadSetSuspendedRegistersInternal: "
            "SetThreadContext() failed, error %u\n",
            GetLastError());
  }
}

int NaClAppThreadUnblockIfFaulted(struct NaClAppThread *natp, int *signal) {
  DWORD previous_suspend_count;

  if (natp->fault_signal == 0) {
    return 0;
  }
  *signal = natp->fault_signal;
  natp->fault_signal = 0;
  AtomicIncrement(&natp->nap->faulted_thread_count, -1);
  /*
   * Decrement Windows' suspension count for the thread.  This undoes
   * the effect of debug_exception_handler.c's SuspendThread() call.
   */
  previous_suspend_count = ResumeThread(GetHostThreadHandle(natp));
  if (previous_suspend_count == (DWORD) -1) {
    NaClLog(LOG_FATAL, "NaClAppThreadUnblockIfFaulted: "
            "ResumeThread() call failed\n");
  }
  /*
   * This thread should already have been suspended using
   * NaClUntrustedThreadSuspend(), so the thread will not actually
   * resume until NaClUntrustedThreadResume() is called.
   */
  DCHECK(previous_suspend_count >= 2);
  return 1;
}
