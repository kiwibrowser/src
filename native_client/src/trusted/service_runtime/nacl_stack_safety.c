/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/include/build_config.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/trusted/service_runtime/nacl_stack_safety.h"

#if (NACL_WINDOWS && NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86 &&  \
     NACL_BUILD_SUBARCH == 64)

/*
 * The default should be "true" so that we can handle crashes that
 * occur on threads other than those created by NaClAppThread.
 */
THREAD uint32_t nacl_on_safe_stack = 1;

#endif
