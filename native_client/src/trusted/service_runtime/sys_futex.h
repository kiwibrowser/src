/*
 * Copyright (c) 2013 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FUTEX_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYS_FUTEX_H_ 1

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

EXTERN_C_BEGIN

struct NaClAppThread;

/* Doubly linked list node, used for the futex wait list. */
struct NaClListNode {
  struct NaClListNode *next;
  struct NaClListNode *prev;
};

int32_t NaClSysFutexWaitAbs(struct NaClAppThread *natp, uint32_t addr,
                            uint32_t value, uint32_t abstime_ptr);

int32_t NaClSysFutexWake(struct NaClAppThread *natp, uint32_t addr,
                         uint32_t nwake);

EXTERN_C_END

#endif
