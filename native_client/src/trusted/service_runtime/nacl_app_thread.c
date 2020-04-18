/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime user thread state.
 */

#include <string.h>

#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/aligned_malloc.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_desc_effector_ldr.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/nacl_stack_safety.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"
#include "native_client/src/trusted/service_runtime/osx/mach_thread_map.h"


void WINAPI NaClAppThreadLauncher(void *state) {
  struct NaClAppThread *natp = (struct NaClAppThread *) state;
  uint32_t thread_idx;
  NaClLog(4, "NaClAppThreadLauncher: entered\n");

  NaClSignalStackRegister(natp->signal_stack);

  NaClLog(4, "      natp = 0x%016"NACL_PRIxPTR"\n", (uintptr_t) natp);
  NaClLog(4, " prog_ctr  = 0x%016"NACL_PRIxNACL_REG"\n", natp->user.prog_ctr);
  NaClLog(4, "stack_ptr  = 0x%016"NACL_PRIxPTR"\n",
          NaClGetThreadCtxSp(&natp->user));

  thread_idx = NaClGetThreadIdx(natp);
  CHECK(0 < thread_idx);
  CHECK(thread_idx < NACL_THREAD_MAX);
  NaClTlsSetCurrentThread(natp);
  nacl_user[thread_idx] = &natp->user;
#if NACL_WINDOWS
  nacl_thread_ids[thread_idx] = GetCurrentThreadId();
#elif NACL_OSX
  NaClSetCurrentMachThreadForThreadIndex(thread_idx);
#endif

  /*
   * We have to hold the threads_mu lock until after thread_num field
   * in this thread has been initialized.  All other threads can only
   * find and examine this natp through the threads table, so the fact
   * that natp is not consistent (no thread_num) will not be visible.
   */
  NaClXMutexLock(&natp->nap->threads_mu);
  natp->thread_num = NaClAddThreadMu(natp->nap, natp);
  NaClXMutexUnlock(&natp->nap->threads_mu);

  NaClVmHoleThreadStackIsSafe(natp->nap);

  NaClStackSafetyNowOnUntrustedStack();

  /*
   * Notify the debug stub, that a new thread is availible.
   */
  if (NULL != natp->nap->debug_stub_callbacks) {
    natp->nap->debug_stub_callbacks->thread_create_hook(natp);
  }

  /*
   * After this NaClAppThreadSetSuspendState() call, we should not
   * claim any mutexes, otherwise we risk deadlock.
   */
  NaClAppThreadSetSuspendState(natp, NACL_APP_THREAD_TRUSTED,
                               NACL_APP_THREAD_UNTRUSTED);

  NaClStartThreadInApp(natp, natp->user.prog_ctr);
}


/*
 * natp should be thread_self(), called while holding no locks.
 */
void NaClAppThreadTeardown(struct NaClAppThread *natp) {
  struct NaClApp  *nap;
  size_t          thread_idx;

  /*
   * mark this thread as dead; doesn't matter if some other thread is
   * asking us to commit suicide.
   */
  NaClLog(3, "NaClAppThreadTeardown(0x%08"NACL_PRIxPTR")\n",
          (uintptr_t) natp);
  nap = natp->nap;
  if (NULL != nap->debug_stub_callbacks) {
    NaClLog(3, " notifying the debug stub of the thread exit\n");
    /*
     * This must happen before deallocating the ID natp->thread_num.
     * We have the invariant that debug stub lock should be acquired before
     * nap->threads_mu lock. Hence we must not hold threads_mu lock while
     * calling debug stub hooks.
     */
    nap->debug_stub_callbacks->thread_exit_hook(natp);
  }

  NaClLog(3, " getting thread table lock\n");
  NaClXMutexLock(&nap->threads_mu);
  NaClLog(3, " getting thread lock\n");
  NaClXMutexLock(&natp->mu);
  /*
   * Remove ourselves from the ldt-indexed global tables.  The ldt
   * entry is released as part of NaClAppThreadDelete(), and if
   * another thread is immediately created (from some other running
   * thread) we want to be sure that any ldt-based lookups will not
   * reach this dying thread's data.
   */
  thread_idx = NaClGetThreadIdx(natp);
  /*
   * On x86-64 and ARM, clearing nacl_user entry ensures that we will
   * fault if another syscall is made with this thread_idx.  In
   * particular, thread_idx 0 is never used.
   */
  nacl_user[thread_idx] = NULL;
#if NACL_WINDOWS
  nacl_thread_ids[thread_idx] = 0;
#elif NACL_OSX
  NaClClearMachThreadForThreadIndex(thread_idx);
#endif
  /*
   * Unset the TLS variable so that if a crash occurs during thread
   * teardown, the signal handler does not dereference a dangling
   * NaClAppThread pointer.
   */
  NaClTlsSetCurrentThread(NULL);

  NaClLog(3, " removing thread from thread table\n");
  /* Deallocate the ID natp->thread_num. */
  NaClRemoveThreadMu(nap, natp->thread_num);
  NaClLog(3, " unlocking thread\n");
  NaClXMutexUnlock(&natp->mu);
  NaClLog(3, " unlocking thread table\n");
  NaClXMutexUnlock(&nap->threads_mu);
  NaClLog(3, " unregistering signal stack\n");
  NaClSignalStackUnregister();
  NaClLog(3, " freeing thread object\n");
  NaClAppThreadDelete(natp);
  NaClLog(3, " NaClThreadExit\n");
  NaClThreadExit();
  NaClLog(LOG_FATAL,
          "NaClAppThreadTeardown: NaClThreadExit() should not return\n");
  /* NOTREACHED */
}


struct NaClAppThread *NaClAppThreadMake(struct NaClApp *nap,
                                        uintptr_t      usr_entry,
                                        uintptr_t      usr_stack_ptr,
                                        uint32_t       user_tls1,
                                        uint32_t       user_tls2) {
  struct NaClAppThread *natp;

  natp = NaClAlignedMalloc(sizeof *natp, __alignof(struct NaClAppThread));
  if (natp == NULL) {
    return NULL;
  }

  NaClLog(4, "         natp = 0x%016"NACL_PRIxPTR"\n", (uintptr_t) natp);
  NaClLog(4, "          nap = 0x%016"NACL_PRIxPTR"\n", (uintptr_t) nap);
  NaClLog(4, "usr_stack_ptr = 0x%016"NACL_PRIxPTR"\n", usr_stack_ptr);

  /*
   * Set these early, in case NaClTlsAllocate() wants to examine them.
   */
  natp->nap = nap;
  natp->thread_num = -1;  /* illegal index */
  natp->host_thread_is_defined = 0;
  memset(&natp->host_thread, 0, sizeof(natp->host_thread));

  if (!NaClAppThreadInitArchSpecific(natp, usr_entry, usr_stack_ptr)) {
    goto cleanup_free;
  }

  NaClTlsSetTlsValue1(natp, user_tls1);
  NaClTlsSetTlsValue2(natp, user_tls2);

  natp->signal_stack = NULL;
  natp->exception_stack = 0;
  natp->exception_flag = 0;

  if (!NaClMutexCtor(&natp->mu)) {
    goto cleanup_free;
  }

  if (!NaClSignalStackAllocate(&natp->signal_stack)) {
    goto cleanup_mu;
  }

  if (!NaClMutexCtor(&natp->suspend_mu)) {
    goto cleanup_mu;
  }
  natp->suspend_state = NACL_APP_THREAD_TRUSTED;
  natp->suspended_registers = NULL;
  natp->fault_signal = 0;

  natp->dynamic_delete_generation = 0;

  if (!NaClCondVarCtor(&natp->futex_condvar)) {
    goto cleanup_suspend_mu;
  }
  return natp;

 cleanup_suspend_mu:
  NaClMutexDtor(&natp->suspend_mu);
 cleanup_mu:
  NaClMutexDtor(&natp->mu);
  if (NULL != natp->signal_stack) {
    NaClSignalStackFree(&natp->signal_stack);
    natp->signal_stack = NULL;
  }
 cleanup_free:
  NaClAlignedFree(natp);
  return NULL;
}


int NaClAppThreadSpawn(struct NaClApp *nap,
                       uintptr_t      usr_entry,
                       uintptr_t      usr_stack_ptr,
                       uint32_t       user_tls1,
                       uint32_t       user_tls2) {
  struct NaClAppThread *natp = NaClAppThreadMake(nap, usr_entry, usr_stack_ptr,
                                                 user_tls1, user_tls2);
  if (natp == NULL) {
    return 0;
  }
  /*
   * We set host_thread_is_defined assuming, for now, that
   * NaClThreadCtor() will succeed.
   */
  natp->host_thread_is_defined = 1;
  if (!NaClThreadCtor(&natp->host_thread, NaClAppThreadLauncher, (void *) natp,
                      NACL_KERN_STACK_SIZE)) {
    /*
     * No other thread saw the NaClAppThread, so it is OK that
     * host_thread was not initialized despite host_thread_is_defined
     * being set.
     */
    natp->host_thread_is_defined = 0;
    NaClAppThreadDelete(natp);
    return 0;
  }
  return 1;
}


void NaClAppThreadDelete(struct NaClAppThread *natp) {
  /*
   * the thread must not be still running, else this crashes the system
   */

  if (natp->host_thread_is_defined) {
    NaClThreadDtor(&natp->host_thread);
  }
  free(natp->suspended_registers);
  NaClMutexDtor(&natp->suspend_mu);
  NaClSignalStackFree(natp->signal_stack);
  natp->signal_stack = NULL;
  NaClCondVarDtor(&natp->futex_condvar);
  NaClTlsFree(natp);
  NaClMutexDtor(&natp->mu);
  NaClAlignedFree(natp);
}
