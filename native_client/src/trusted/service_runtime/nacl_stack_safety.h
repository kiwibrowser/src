/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_STACK_SAFETY_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_STACK_SAFETY_H_

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

/*
 * NB: Relying code should open-code -- not use a function call -- in
 * assembly the necessary code to read nacl_thread_on_safe_stack.
 */

#if (NACL_WINDOWS && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 &&  \
     NACL_BUILD_SUBARCH == 64)

extern THREAD uint32_t nacl_on_safe_stack;

static INLINE void NaClStackSafetyNowOnUntrustedStack(void) {
  nacl_on_safe_stack = 0;
}

static INLINE void NaClStackSafetyNowOnTrustedStack(void) {
  nacl_on_safe_stack = 1;
}

#else

static INLINE void NaClStackSafetyNowOnUntrustedStack(void) {
}

static INLINE void NaClStackSafetyNowOnTrustedStack(void) {
}

#endif

EXTERN_C_END

#endif
