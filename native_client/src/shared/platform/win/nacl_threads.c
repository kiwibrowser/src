/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime threads implementation layer.
 */

#include <process.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

int NaClThreadCtor(struct NaClThread  *ntp,
                   void               (WINAPI *start_fn)(void *),
                   void               *state,
                   size_t             stack_size) {
  HANDLE handle;
  DWORD actual_stack_size;

  if (stack_size > MAXDWORD) {
    NaClLog(LOG_ERROR,
            ("NaClThreadCtor: "
             "_beginthreadex failed, stack request out of range\n"),
            errno);
    return 0;
  } else {
    actual_stack_size = (DWORD)stack_size;
  }

  handle = (HANDLE) _beginthreadex(NULL,      /* default security */
                                   actual_stack_size,
                                   (unsigned (WINAPI *)(void *))  start_fn,
                                   /* the argument for the thread function */
                                   state,
                                   CREATE_SUSPENDED ,  /* start suspended */
                                   NULL);
  if (0 == handle){  /* we don't need the thread id */
    NaClLog(LOG_ERROR,
            "NaClThreadCtor: _beginthreadex failed, errno %d\n",
            errno);
    return 0;
  }
  ntp->tid = handle;  /* we need the handle to kill the thread etc. */

  /* Now that the structure is filled, the thread can start */
  ResumeThread(handle);

  return 1;
}

int NaClThreadCreateJoinable(struct NaClThread  *ntp,
                             void               (WINAPI *start_fn)(void *),
                             void               *state,
                             size_t             stack_size) {
  return NaClThreadCtor(ntp, start_fn, state, stack_size);
}

void NaClThreadDtor(struct NaClThread *ntp) {
  /*
   * the handle is not closed when the thread exits because we are
   * using _beginthreadex and not _beginthread, so we must close it
   * manually
   */
  CloseHandle(ntp->tid);
}

void NaClThreadJoin(struct NaClThread *ntp) {
  DWORD result = WaitForSingleObject(ntp->tid, INFINITE);
  if (result != WAIT_OBJECT_0) {
    NaClLog(LOG_ERROR, "NaClThreadJoin: unexpected result of thread\n");
  }
  CloseHandle(ntp->tid);
}

void NaClThreadExit(void) {
  /*
   * On Windows, the exit status of a process is taken to be the exit
   * status of the last thread that exited.  If a process is killed
   * with TerminateProcess(), the threads exit in a non-deterministic
   * order.  In that case, the thread exit status we return here could
   * become the process's exit status.  We return a magic number so
   * that we can identify when that happens.
   * See https://code.google.com/p/nativeclient/issues/detail?id=2870
   */
  _endthreadex(0xdecea5ed);
}

uint32_t NaClThreadId(void) {
  return GetCurrentThreadId();
}

void NaClThreadYield(void) {
  SwitchToThread();
}
