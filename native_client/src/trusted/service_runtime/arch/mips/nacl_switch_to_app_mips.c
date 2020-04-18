/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime, C-level context switch code.
 */

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/mips/sel_rt.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

void NaClInitSwitchToApp(struct NaClApp *nap) {
 /*
  * We don't need anything here.
  */
  UNREFERENCED_PARAMETER(nap);
}

NORETURN void NaClStartThreadInApp(struct NaClAppThread *natp,
                                   uint32_t             new_prog_ctr) {
  struct NaClThreadContext  *context;

  natp->user.trusted_stack_ptr = NaClGetStackPtr() & ~NACL_STACK_ALIGN_MASK;

  context = &natp->user;
  context->new_prog_ctr = new_prog_ctr;

  /*
   * At startup this is not the return value, but the first argument.
   * In the initial thread, it gets the pointer to the information
   * block on the stack.  Additional threads do not expect anything in
   * particular in the first argument register, so we don't bother to
   * conditionalize this.
   */
  context->sysret = context->stack_ptr;

  /*
   * context stored in $a0
   */
  NaClStartSwitch(context);
}
