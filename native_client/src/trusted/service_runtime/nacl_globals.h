/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime globals.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/trusted/service_runtime/arch/sel_ldr_arch.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"

EXTERN_C_BEGIN
struct NaClThreadContext;
struct NaClAppThread;
struct NaClMutex;
struct NaClApp;

#if NACL_WINDOWS
__declspec(dllexport)
/*
 * This array is exported so that it can be used by a debugger.  However, it is
 * not a stable interface and it may change or be removed in the future.  A
 * debugger using this interface could break.
 */
#endif
extern struct NaClThreadContext *nacl_user[NACL_THREAD_MAX];
#if NACL_WINDOWS
/*
 * NaCl Idx -> Thread ID mapping. Gdb scans this array to find NaCl index
 * by Thread ID.
 *
 * This is not a stable interface and it may change or be removed in
 * the future.  A debugger using this interface could break.
 */
__declspec(dllexport) extern uint32_t nacl_thread_ids[NACL_THREAD_MAX];
#endif
/*
 * nacl_user is accessed without holding any locks.  once a thread is
 * live, only that thread itself may read/write the register context
 * contents (based on its %gs), and this allows a thread to context
 * switch from the application to the runtime, since we must have a
 * secure stack before calling any code, including lock acquisition
 * code.
 */

void  NaClGlobalModuleInit(void);
void  NaClGlobalModuleFini(void);

/* this is defined in src/trusted/service_runtime/arch/<arch>/ sel_rt.h */
void NaClInitGlobals(void);

static INLINE struct NaClAppThread *NaClAppThreadGetFromIndex(
    uint32_t thread_index) {
  DCHECK(thread_index < NACL_THREAD_MAX);
  return NaClAppThreadFromThreadContext(nacl_user[thread_index]);
}

/* hack for gdb */
#if NACL_WINDOWS
__declspec(dllexport)
#endif
extern uintptr_t nacl_global_xlate_base;

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_GLOBALS_H__ */
