/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sys_exception.h"

#include "native_client/src/shared/platform/nacl_sync_checked.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_copy.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/win/debug_exception_handler.h"


int32_t NaClSysExceptionHandler(struct NaClAppThread *natp,
                                uint32_t             handler_addr,
                                uint32_t             old_handler) {
  struct NaClApp *nap = natp->nap;
  int32_t rv = -NACL_ABI_EINVAL;

  if (!nap->enable_exception_handling) {
    rv = -NACL_ABI_ENOSYS;
    goto no_lock_exit;
  }
  if (!NaClIsValidJumpTarget(nap, handler_addr)) {
    rv = -NACL_ABI_EFAULT;
    goto no_lock_exit;
  }
  NaClXMutexLock(&nap->exception_mu);

  /*
   * This needs to be done while holding the lock so that we don't
   * start two Windows debug exception handlers.
   */
  if (handler_addr != 0) {
    if (!NaClDebugExceptionHandlerEnsureAttached(nap)) {
      rv = -NACL_ABI_ENOSYS;
      goto unlock_exit;
    }
  }

  if (0 != old_handler &&
      !NaClCopyOutToUser(nap, (uintptr_t) old_handler,
                         &nap->exception_handler,
                         sizeof nap->exception_handler)) {
    rv = -NACL_ABI_EFAULT;
    goto unlock_exit;
  }
  nap->exception_handler = handler_addr;
  rv = 0;
 unlock_exit:
  NaClXMutexUnlock(&nap->exception_mu);
 no_lock_exit:
  return rv;
}

int32_t NaClSysExceptionStack(struct NaClAppThread *natp,
                              uint32_t             stack_addr,
                              uint32_t             stack_size) {
  if (!natp->nap->enable_exception_handling) {
    return -NACL_ABI_ENOSYS;
  }
  if (kNaClBadAddress == NaClUserToSysAddrNullOkay(natp->nap,
                                                   stack_addr + stack_size)) {
    return -NACL_ABI_EINVAL;
  }
  natp->exception_stack = stack_addr + stack_size;
  return 0;
}

int32_t NaClSysExceptionClearFlag(struct NaClAppThread *natp) {
  if (!natp->nap->enable_exception_handling) {
    return -NACL_ABI_ENOSYS;
  }
  natp->exception_flag = 0;
  return 0;
}
