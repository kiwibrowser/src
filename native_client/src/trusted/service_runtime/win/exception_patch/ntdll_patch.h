/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_WIN_EXCEPTION_PATCH_NTDLL_PATCH_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_WIN_EXCEPTION_PATCH_NTDLL_PATCH_H_ 1

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"

#if (NACL_WINDOWS && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 && \
     NACL_BUILD_SUBARCH == 64)

void NaClPatchWindowsExceptionDispatcher(void);

uint8_t *NaClGetKiUserExceptionDispatcher(void);

extern uint8_t NaCl_exception_dispatcher_exit_fast[];
extern uint8_t NaCl_exception_dispatcher_exit_fast_end[];

extern uint8_t NaCl_exception_dispatcher_intercept[];
extern uint8_t NaCl_exception_dispatcher_intercept_tls_index[];
extern uint8_t NaCl_exception_dispatcher_intercept_end[];

#endif


#endif
