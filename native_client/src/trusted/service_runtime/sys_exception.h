/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_EXCEPTION_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_EXCEPTION_H_ 1

#include "native_client/src/include/portability.h"

struct NaClAppThread;

int32_t NaClSysExceptionHandler(struct NaClAppThread *natp,
                                uint32_t             handler_addr,
                                uint32_t             old_handler);

int32_t NaClSysExceptionStack(struct NaClAppThread *natp,
                              uint32_t             stack_addr,
                              uint32_t             stack_size);

int32_t NaClSysExceptionClearFlag(struct NaClAppThread *natp);

#endif
