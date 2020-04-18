/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Gather ye all module initializations and finalizations here.
 */
#include "native_client/src/trusted/debug_stub/debug_stub.h"
#include "native_client/src/trusted/desc/nrd_all_modules.h"
#include "native_client/src/trusted/fault_injection/fault_injection.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/nacl_thread_nice.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/nacl_stack_safety.h"

void  NaClAllModulesInit(void) {
  NaClNrdAllModulesInit();
  NaClFaultInjectionModuleInit();
  NaClGlobalModuleInit();  /* various global variables */
  NaClTlsInit();
  NaClThreadNiceInit();
}


void NaClAllModulesFini(void) {
  NaClTlsFini();
  NaClGlobalModuleFini();
  NaClNrdAllModulesFini();
}
