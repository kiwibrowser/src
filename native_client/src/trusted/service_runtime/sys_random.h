/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_RANDOM_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_RANDOM_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClAppThread;

int32_t NaClSysGetRandomBytes(struct NaClAppThread *natp,
                              uint32_t buf_addr, uint32_t buf_size);

EXTERN_C_END

#endif
