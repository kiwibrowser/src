/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime user thread state.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__ 1

#include <stddef.h>

#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/include/build_config.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/service_runtime/nacl_signal.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/sys_futex.h"


EXTERN_C_BEGIN

struct NaClApp;
struct NaClAppThreadSuspendedRegisters;

/*
 * The thread hosting the NaClAppThread may change suspend_state
 * between NACL_APP_THREAD_TRUSTED and NACL_APP_THREAD_UNTRUSTED using
 * NaClAppThreadSetSuspendState().
 *
 * On Linux, a controlling thread may change this from:
 *   NACL_APP_THREAD_UNTRUSTED
 *   -> NACL_APP_THREAD_UNTRUSTED | NACL_APP_THREAD_SUSPENDING
 * or
 *   NACL_APP_THREAD_TRUSTED
 *   -> NACL_APP_THREAD_TRUSTED | NACL_APP_THREAD_SUSPENDING
 * and back again.
 *
 * Furthermore, the signal handler in the thread being suspended will
 * change suspend_state from:
 *   NACL_APP_THREAD_UNTRUSTED | NACL_APP_THREAD_SUSPENDING
 *   -> (NACL_APP_THREAD_UNTRUSTED | NACL_APP_THREAD_SUSPENDING
 *       | NACL_APP_THREAD_SUSPENDED)
 * The controlling thread will later change suspend_thread back to:
 *   NACL_APP_THREAD_UNTRUSTED
 * This tells the signal handler to resume execution.
 */
enum NaClSuspendState {
  NACL_APP_THREAD_UNTRUSTED = 1,
  NACL_APP_THREAD_TRUSTED = 2
#if NACL_LINUX
  , NACL_APP_THREAD_SUSPENDING = 4,
  NACL_APP_THREAD_SUSPENDED = 8
#endif
};

/*
 * Generally, only the thread itself will need to manipulate this
 * structure, but occasionally we may need to blow away a thread for
 * some reason, and look inside.  While a thread is in the NaCl
 * application running untrusted code, the lock must *not* be held.
 */
struct NaClAppThread {
  /*
   * NaClAppThread 'inherits' from NaClThreadContext; the 'user' field
   * is first so that NaClAppThreadFromThreadContext() can convert by
   * casting.
   *
   * 'user' contains all the architecture-specific state for this thread.
   * TODO(mseaborn): Rename it to a more descriptive name.
   */
  struct NaClThreadContext  user;

  struct NaClMutex          mu;

  /*
   * The NaCl app that contains this thread.  The app must exist as
   * long as a thread is still alive.
   */
  struct NaClApp            *nap;

  int                       thread_num;  /* index into nap->threads */

  /*
   * If host_thread_is_defined is true, host_thread is initialized and
   * owned by the NaClAppThread such that it will be freed by
   * NaClAppThreadDelete().
   *
   * host_thread_is_defined may be false when running untrusted code
   * on a borrowed host thread that was not created by
   * NaClAppThreadSpawn().
   */
  int                       host_thread_is_defined;
  struct NaClThread         host_thread;  /* low level thread representation */

  struct NaClMutex          suspend_mu;
  Atomic32                  suspend_state; /* enum NaClSuspendState */
  /*
   * suspended_registers contains the register state of the thread if
   * it has been suspended with save_registers=1 and if suspend_state
   * contains NACL_APP_THREAD_UNTRUSTED.  This is for use by the debug
   * stub.
   *
   * To save space, suspended_registers is allocated on demand.  It
   * may be left allocated after the thread is resumed.
   *
   * suspended_registers will usually contain untrusted-code register
   * state.  However, it may contain trusted-code register state if
   * the thread suspension kicked in during a trusted/untrusted
   * context switch (e.g. while executing the trampoline or
   * nacl_syscall_hook.c).
   */
  struct NaClAppThreadSuspendedRegisters *suspended_registers;
  /*
   * If fault_signal is non-zero, the thread has faulted and so has
   * been suspended.  fault_signal indicates the type of fault (it is
   * a signal number on Linux and an exception code on Windows).
   */
  int fault_signal;

  uintptr_t                 usr_syscall_args;
  /*
   * user's syscall argument address, relative to untrusted user
   * address.  the syscall arglist is on the untrusted stack.
   */

  /* Stack for signal handling, registered with sigaltstack(). */
  void                      *signal_stack;

  /*
   * exception_stack is the address of the top of the untrusted
   * exception handler stack, or 0 if no such stack is registered for
   * this thread.
   */
  uint32_t                  exception_stack;
  /*
   * exception_flag is a boolean.  When it is 1, untrusted exception
   * handling is disabled for this thread.  It is set to 1 when the
   * exception handler is called, in order to prevent the exception
   * handler from being re-entered.
   */
  uint32_t                  exception_flag;

  /*
   * The last generation this thread reported into the service runtime
   * Protected by mu
   */
  int                       dynamic_delete_generation;

  /*
   * If this thread is waiting on a futex, futex_wait_list_node is
   * linked into the doubly linked list NaClApp::futex_wait_list_head.
   */
  struct NaClListNode       futex_wait_list_node;
  /*
   * If this thread is waiting on a futex, futex_wait_addr contains
   * the untrusted address that the thread is waiting on.
   */
  uint32_t                  futex_wait_addr;
  struct NaClCondVar        futex_condvar;
};

void WINAPI NaClAppThreadLauncher(void *state);

void NaClAppThreadTeardown(struct NaClAppThread *natp);

/*
 * NaClAppThreadMake() creates a NaClAppThread object without invoking
 * untrusted code or creating a host thread.
 *
 * The usr_entry and usr_stack_ptr values are directly used to
 * initialize the user register values.  The caller is responsible for
 * error checking: usr_entry must be a valid entry point (0 mod N).
 */
struct NaClAppThread *NaClAppThreadMake(struct NaClApp *nap,
                                        uintptr_t      usr_entry,
                                        uintptr_t      usr_stack_ptr,
                                        uint32_t       user_tls1,
                                        uint32_t       user_tls2) NACL_WUR;

/*
 * NaClAppThreadSpawn() creates a NaClAppThread and launches a host
 * thread that invokes the given entry point in untrusted code.  This
 * returns true on success, false on failure.
 */
int NaClAppThreadSpawn(struct NaClApp *nap,
                       uintptr_t      usr_entry,
                       uintptr_t      usr_stack_ptr,
                       uint32_t       user_tls1,
                       uint32_t       user_tls2) NACL_WUR;

void NaClAppThreadDelete(struct NaClAppThread *natp);

/*
 * This function can be called from the thread hosting the
 * NaClAppThread.  It can be used to switch between the states
 * NACL_APP_THREAD_TRUSTED and NACL_APP_THREAD_UNTRUSTED.
 */
void NaClAppThreadSetSuspendState(struct NaClAppThread *natp,
                                  enum NaClSuspendState old_state,
                                  enum NaClSuspendState new_state);

static INLINE struct NaClAppThread *NaClAppThreadFromThreadContext(
    struct NaClThreadContext *ntcp) {
  NACL_COMPILE_TIME_ASSERT(offsetof(struct NaClAppThread, user) == 0);
  return (struct NaClAppThread *) ntcp;
}

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_APP_THREAD_H__ */
