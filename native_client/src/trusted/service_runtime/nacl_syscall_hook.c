/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time.
 */

#include "native_client/src/include/portability.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "native_client/src/include/nacl_compiler_annotations.h"
#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_stack_safety.h"


/*
 * HandleStackContext() fetches some of the inputs to the NaCl syscall
 * from the untrusted stack.  It updates NaClThreadContext so that the
 * saved state will be complete in case this state is read via the
 * thread suspension API.
 *
 * This is called while natp->suspend_state is set to
 * NACL_APP_THREAD_UNTRUSTED, which has two consequences:
 *
 *  1) We may read untrusted address space without calling
 *     NaClCopyTakeLock() first, because this function's execution
 *     will be suspended while any mmap hole is opened up on Windows.
 *
 *  2) We may not claim any locks.  This means we may not call
 *     NaClLog().  (An exception is that LOG_FATAL calls to NaClLog()
 *     should be okay for internal errors.)
 */
static void HandleStackContext(struct NaClAppThread *natp,
                               uint32_t             *tramp_ret_out,
                               uintptr_t            *sp_user_out) {
  struct NaClApp *nap = natp->nap;
  uintptr_t      sp_user;
  uintptr_t      sp_sys;
  uint32_t       tramp_ret;
  nacl_reg_t     user_ret;

  /*
   * sp_sys points to the top of the user stack where return addresses
   * and syscall arguments are stored.
   *
   * Note that on x86-64, NaClUserToSysStackAddr() and
   * NaClSysToUserStackAddr() do no range check.  sp_user must be okay
   * for control to have reached here, because nacl_syscall*.S writes
   * to the stack.
   */
  sp_user = NaClGetThreadCtxSp(&natp->user);
  sp_sys = NaClUserToSysStackAddr(nap, sp_user);
  /*
   * Get the trampoline return address.  This just tells us which
   * trampoline was called (and hence the syscall number); we never
   * return to the trampoline.
   */
  tramp_ret = *(volatile uint32_t *) (sp_sys + NACL_TRAMPRET_FIX);
  /*
   * Get the user return address (where we return to after the system
   * call).  We must ensure the address is properly sandboxed before
   * switching back to untrusted code.
   */
  user_ret = *(volatile uintptr_t *) (sp_sys + NACL_USERRET_FIX);
  user_ret = (nacl_reg_t) NaClSandboxCodeAddr(nap, (uintptr_t) user_ret);
  natp->user.new_prog_ctr = user_ret;

  *tramp_ret_out = tramp_ret;
  *sp_user_out = sp_user;
}

struct NaClThreadContext *NaClSyscallCSegHook(struct NaClThreadContext *ntcp) {
  struct NaClAppThread      *natp = NaClAppThreadFromThreadContext(ntcp);
  struct NaClApp            *nap;
  uint32_t                  tramp_ret;
  size_t                    sysnum;
  uintptr_t                 sp_user;
  uint32_t                  sysret;

  /*
   * Mark the thread as running on a trusted stack as soon as possible
   * so that we can report any crashes that occur after this point.
   */
  NaClStackSafetyNowOnTrustedStack();

  HandleStackContext(natp, &tramp_ret, &sp_user);

  /*
   * Before this call, the thread could be suspended, so we should not
   * lock any mutexes before this, otherwise it could cause a
   * deadlock.
   */
  NaClAppThreadSetSuspendState(natp, NACL_APP_THREAD_UNTRUSTED,
                               NACL_APP_THREAD_TRUSTED);

  nap = natp->nap;

  NaClCopyTakeLock(nap);
  /*
   * held until syscall args are copied, which occurs in the generated
   * code.
   */

  sysnum = (tramp_ret - NACL_SYSCALL_START_ADDR) >> NACL_SYSCALL_BLOCK_SHIFT;

  NaClLog(4, "Entering syscall %"NACL_PRIuS
          ": return address 0x%08"NACL_PRIxNACL_REG"\n",
          sysnum, natp->user.new_prog_ctr);

  /*
   * usr_syscall_args is used by Decoder functions in
   * nacl_syscall_handlers.c which is automatically generated file and
   * placed in the
   * scons-out/.../gen/native_client/src/trusted/service_runtime/
   * directory.  usr_syscall_args must point to the first argument of
   * a system call. System call arguments are placed on the untrusted
   * user stack.
   *
   * We save the user address for user syscall arguments fetching and
   * for VM range locking.
   */
  natp->usr_syscall_args = NaClRawUserStackAddrNormalize(sp_user +
                                                         NACL_SYSARGS_FIX);

  if (NACL_UNLIKELY(sysnum >= NACL_MAX_SYSCALLS)) {
    NaClLog(2, "INVALID system call %"NACL_PRIuS"\n", sysnum);
    sysret = (uint32_t) -NACL_ABI_EINVAL;
    NaClCopyDropLock(nap);
  } else {
    sysret = (*(nap->syscall_table[sysnum].handler))(natp);
    /* Implicitly drops lock */
  }
  NaClLog(4,
          ("Returning from syscall %"NACL_PRIuS": return value %"NACL_PRId32
           " (0x%"NACL_PRIx32")\n"),
          sysnum, sysret, sysret);
  natp->user.sysret = sysret;

  /*
   * After this NaClAppThreadSetSuspendState() call, we should not
   * claim any mutexes, otherwise we risk deadlock.
   */
  NaClAppThreadSetSuspendState(natp, NACL_APP_THREAD_TRUSTED,
                               NACL_APP_THREAD_UNTRUSTED);
  NaClStackSafetyNowOnUntrustedStack();

  /*
   * The caller switches back to untrusted code after this return.
   */
  return ntcp;
}
