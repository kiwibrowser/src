/*
 * Copyright 2015 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SYS_CLOCK_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SYS_CLOCK_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClAppThread;

int32_t NaClSysClock(struct NaClAppThread *natp);

EXTERN_C_END

#endif
